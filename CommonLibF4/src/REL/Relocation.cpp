#include "REL/Relocation.h"
#include "InstructionDecoder.h"
#include "RuntimeDatabase.h"

#define WIN32_LEAN_AND_MEAN

#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define OEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NOCTLMGR
#define NODRAWTEXT
#define NOGDI
#define NOKERNEL
#define NOUSER
#define NONLS
#define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOMINMAX
#define NOMSG
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX

#include <Windows.h>

#include <chrono>
#include <fstream>
#include <unordered_set>

namespace REL
{
	std::string_view id_resolve_status_text(IDResolveStatus a_status) noexcept
	{
		switch (a_status) {
		case IDResolveStatus::kResolvedLocalCache:
			return "local_cache";
		case IDResolveStatus::kResolvedKnownRVA:
			return "known_rva";
		case IDResolveStatus::kResolvedSharedCache:
			return "shared_cache";
		case IDResolveStatus::kResolvedPattern:
			return "pattern";
		case IDResolveStatus::kResolvedLegacy:
			return "legacy";
		case IDResolveStatus::kOGBridgeFailed:
			return "og_bridge_failed";
		case IDResolveStatus::kNGBridgeFailed:
			return "ng_bridge_failed";
		case IDResolveStatus::kUnknownID:
			return "unknown_id";
		case IDResolveStatus::kNoCompatiblePattern:
			return "no_compatible_pattern";
		case IDResolveStatus::kPatternNotFound:
			return "pattern_not_found";
		case IDResolveStatus::kPatternAmbiguous:
			return "pattern_ambiguous";
		case IDResolveStatus::kInvalidPattern:
			return "invalid_pattern";
		case IDResolveStatus::kRuntimeUnavailable:
			return "runtime_unavailable";
		case IDResolveStatus::kSemanticValidationFailed:
			return "semantic_validation_failed";
		case IDResolveStatus::kCallsiteNotFound:
			return "callsite_not_found";
		case IDResolveStatus::kCallsiteAmbiguous:
			return "callsite_ambiguous";
		case IDResolveStatus::kInvalidOffset:
			return "invalid_offset";
		case IDResolveStatus::kInvalidCallsite:
			return "invalid_callsite";
		case IDResolveStatus::kUnresolved:
			return "unresolved";
		}
		return "unresolved";
	}

	namespace
	{
		struct RuntimeFunction
		{
			std::uint32_t begin;
			std::uint32_t end;
			std::uint32_t unwind;
		};

		constexpr std::size_t MAX_AUTO_CALLSITE_FUNCTION_BYTES = 256u * 1024u;

		[[nodiscard]] std::string_view runtime_family_text(const Version& a_version) noexcept
		{
			switch (runtime_family(a_version)) {
			case RuntimeFamily::kOG:
				return "og";
			case RuntimeFamily::kNG:
				return "ng";
			case RuntimeFamily::kAE:
			default:
				return "ae";
			}
		}

		struct DiagnosticPaths
		{
			std::filesystem::path trace;
			std::filesystem::path mapping;
			std::filesystem::path mappingFail;
		};

		[[nodiscard]] const DiagnosticPaths& diagnostic_paths()
		{
			static const DiagnosticPaths paths = [] {
				static const std::uint8_t moduleAnchor{};
				HMODULE module{};
				if (!GetModuleHandleExW(
						GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
							GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
						reinterpret_cast<LPCWSTR>(std::addressof(moduleAnchor)),
						std::addressof(module))) {
					return DiagnosticPaths{};
				}
				std::wstring buffer(32768, L'\0');
				const auto length = GetModuleFileNameW(
					module,
					buffer.data(),
					static_cast<DWORD>(buffer.size()));
				if (length == 0 || length >= buffer.size()) {
					return DiagnosticPaths{};
				}
				buffer.resize(length);
				DiagnosticPaths result;
				result.trace = buffer;
				result.trace.replace_extension(".trace");
				result.mapping = buffer;
				result.mapping.replace_extension(".mapping");
				result.mappingFail = result.mapping;
				result.mappingFail += ".fail";
				return result;
			}();
			return paths;
		}

		[[nodiscard]] bool module_readable(
			const Module& a_module,
			std::uint64_t a_rva,
			std::size_t a_size) noexcept
		{
			for (std::size_t index = 0; index < Segment::total; ++index) {
				const auto segment = a_module.segment(static_cast<Segment::Name>(index));
				if (a_rva >= segment.offset() && a_rva - segment.offset() <= segment.size() &&
					a_size <= segment.size() - static_cast<std::size_t>(a_rva - segment.offset())) {
					return true;
				}
			}
			return false;
		}

		[[nodiscard]] std::vector<std::pair<std::uint32_t, std::uint32_t>>
		logical_function_scopes(const Module& a_module, std::uint32_t a_ownerRVA) noexcept
		{
			const auto pdata = a_module.segment(Segment::pdata);
			if (pdata.size() < sizeof(RuntimeFunction) ||
				pdata.size() % sizeof(RuntimeFunction) != 0) {
				return {};
			}
			const auto functions = std::span(
				pdata.pointer<const RuntimeFunction>(), pdata.size() / sizeof(RuntimeFunction));
			const auto rootOf = [&](RuntimeFunction a_function) -> std::optional<RuntimeFunction> {
				if (a_function.begin >= a_function.end || a_function.end > a_module.image_size()) {
					return std::nullopt;
				}
				std::unordered_set<std::uint32_t> followed;
				while (a_function.unwind != 0) {
					if (!followed.insert(a_function.unwind).second ||
						!module_readable(a_module, a_function.unwind, 4)) {
						return std::nullopt;
					}
					const auto* header = reinterpret_cast<const std::uint8_t*>(
						a_module.base() + a_function.unwind);
					if (((header[0] >> 3) & 0x4) == 0) {
						break;
					}
					const auto codeCount = static_cast<std::size_t>(header[2]);
					const auto chainedOffset = static_cast<std::size_t>(4) +
					                           ((codeCount + 1) & ~std::size_t{ 1 }) * 2;
					if (a_function.unwind > UINT32_MAX - chainedOffset ||
						!module_readable(
							a_module, a_function.unwind + chainedOffset, sizeof(RuntimeFunction))) {
						return std::nullopt;
					}
					RuntimeFunction chained{};
					std::memcpy(
						std::addressof(chained),
						reinterpret_cast<const void*>(
							a_module.base() + a_function.unwind + chainedOffset),
						sizeof(chained));
					if (chained.begin >= chained.end || chained.end > a_module.image_size()) {
						return std::nullopt;
					}
					a_function = chained;
				}
				return a_function;
			};

			std::optional<RuntimeFunction> root;
			for (const auto& function : functions) {
				if (function.begin <= a_ownerRVA && a_ownerRVA < function.end) {
					const auto candidate = rootOf(function);
					if (!candidate) {
						return {};
					}
					if (!root) {
						root = *candidate;
					} else if (candidate->begin != root->begin || candidate->end != root->end) {
						return {};
					}
				}
			}
			if (!root) {
				return {};
			}

			std::vector<std::pair<std::uint32_t, std::uint32_t>> scopes;
			scopes.emplace_back(root->begin, root->end);
			for (const auto& function : functions) {
				const auto candidate = rootOf(function);
				if (candidate && candidate->begin == root->begin && candidate->end == root->end) {
					scopes.emplace_back(function.begin, function.end);
				}
			}
			std::ranges::sort(scopes);
			std::vector<std::pair<std::uint32_t, std::uint32_t>> merged;
			for (const auto& scope : scopes) {
				if (!merged.empty() && scope.first <= merged.back().second) {
					merged.back().second = std::max(merged.back().second, scope.second);
				} else {
					merged.push_back(scope);
				}
			}

			const auto text = a_module.segment(Segment::text);
			std::size_t totalBytes{};
			bool containsOwner{};
			for (const auto& [begin, end] : merged) {
				if (begin >= end || begin < text.offset() ||
					static_cast<std::uint64_t>(end - text.offset()) > text.size() ||
					!module_readable(a_module, begin, end - begin)) {
					return {};
				}
				const auto size = static_cast<std::size_t>(end - begin);
				if (size > MAX_AUTO_CALLSITE_FUNCTION_BYTES - totalBytes) {
					return {};
				}
				totalBytes += size;
				containsOwner = containsOwner || (begin <= a_ownerRVA && a_ownerRVA < end);
			}
			return containsOwner ? merged : std::vector<std::pair<std::uint32_t, std::uint32_t>>{};
		}

		[[nodiscard]] IDResolveStatus runtime_failure_status(RuntimeDatabase::ResolveFailure a_failure) noexcept
		{
			switch (a_failure) {
			case RuntimeDatabase::ResolveFailure::kUnknownID:
				return IDResolveStatus::kUnknownID;
			case RuntimeDatabase::ResolveFailure::kNoCompatiblePattern:
				return IDResolveStatus::kNoCompatiblePattern;
			case RuntimeDatabase::ResolveFailure::kPatternNotFound:
				return IDResolveStatus::kPatternNotFound;
			case RuntimeDatabase::ResolveFailure::kPatternAmbiguous:
				return IDResolveStatus::kPatternAmbiguous;
			case RuntimeDatabase::ResolveFailure::kInvalidPattern:
				return IDResolveStatus::kInvalidPattern;
			case RuntimeDatabase::ResolveFailure::kRuntimeUnavailable:
				return IDResolveStatus::kRuntimeUnavailable;
			case RuntimeDatabase::ResolveFailure::kSemanticValidationFailed:
				return IDResolveStatus::kSemanticValidationFailed;
			case RuntimeDatabase::ResolveFailure::kNone:
			default:
				return IDResolveStatus::kUnresolved;
			}
		}

		void trace_resolution(const IDResolveResult& a_result)
		{
			static std::mutex lock;
			static std::unordered_set<std::uint64_t> traced;
			static std::unordered_set<std::string> tracedOffsets;
			static bool compactSuccessLogged{ false };
			static bool traceChecked{ false };
			static bool traceWriteFailed{ false };
			static std::ofstream trace;
			const std::scoped_lock guard{ lock };
			if (!traceChecked) {
				traceChecked = true;
				const auto& path = diagnostic_paths().trace;
				std::error_code error;
				if (!path.empty() && std::filesystem::exists(path, error) && !error) {
					trace.open(path, std::ios::out | std::ios::trunc);
					if (!trace.is_open()) {
						spdlog::error(
							"F4RD FAIL trace reason=open_failed file={}",
							path.filename().string());
					}
				}
			}
			if (!a_result) {
				spdlog::error(
					"F4RD FAIL id={} reason={} elapsed_us={}",
					a_result.id,
					id_resolve_status_text(a_result.status),
					a_result.elapsedMicroseconds);
				return;
			}

			if (trace.is_open()) {
				bool wrote{};
				if (a_result.selectedOffset && a_result.finalRva) {
					const auto key = fmt::format(
						"{}:{}:{}:{}",
						a_result.id,
						*a_result.selectedOffset,
						a_result.automaticOffset,
						runtime_family_text(Module::get().version()));
					if (tracedOffsets.insert(key).second) {
						trace << fmt::format(
							"{} 0x{:X} offset=0x{:X} result=0x{:X} slot={} source={}\n",
							a_result.id,
							*a_result.rva,
							*a_result.selectedOffset,
							*a_result.finalRva,
							runtime_family_text(Module::get().version()),
							a_result.automaticOffset ? "auto" : "variant");
						wrote = true;
					}
				} else if (traced.insert(a_result.id).second) {
					trace << fmt::format("{} 0x{:X}\n", a_result.id, *a_result.rva);
					wrote = true;
				}
				if (wrote) {
					trace.flush();
					if (!trace && !traceWriteFailed) {
						traceWriteFailed = true;
						spdlog::error(
							"F4RD FAIL trace reason=write_failed file={}",
							diagnostic_paths().trace.filename().string());
					}
				}
			}

			if (!compactSuccessLogged) {
				compactSuccessLogged = true;
				const auto version = Module::get().version();
				spdlog::info(
					"F4RD OK runtime={} mode={} source={}",
					version.string(),
					runtime_family(version) == RuntimeFamily::kOG ? "hybrid" : "patterns",
					id_resolve_status_text(a_result.status));
			}
		}
	}

	namespace detail
	{
		CallsiteScanResult find_relative_callsite(
			std::span<const std::uint8_t> a_function,
			std::uint32_t a_functionRVA,
			std::uint32_t a_ownerRVA,
			std::uint32_t a_targetRVA,
			AutoCallsiteBranch a_branch,
			std::uint16_t a_occurrence) noexcept
		{
			CallsiteScanResult result;
			if (a_branch != AutoCallsiteBranch::kCall &&
				a_branch != AutoCallsiteBranch::kJump &&
				a_branch != AutoCallsiteBranch::kCallOrJump) {
				return result;
			}
			if (a_function.size() < 5 || a_functionRVA > a_ownerRVA ||
				static_cast<std::uint64_t>(a_ownerRVA - a_functionRVA) >= a_function.size()) {
				return result;
			}

			std::optional<std::ptrdiff_t> selected;
			const auto decoded = for_each_direct_relative_branch(
				a_function,
				[&](std::size_t offset, std::uint8_t opcode) {
					const auto accepted =
						a_branch == AutoCallsiteBranch::kCall ? opcode == 0xE8 :
						a_branch == AutoCallsiteBranch::kJump ? opcode == 0xE9 :
						(opcode == 0xE8 || opcode == 0xE9);
					if (!accepted) {
						return;
					}
					std::int32_t displacement{};
					std::memcpy(
						std::addressof(displacement), a_function.data() + offset + 1, sizeof(displacement));
					const auto target = static_cast<std::int64_t>(a_functionRVA) +
					                    static_cast<std::int64_t>(offset) + 5 + displacement;
					if (target != a_targetRVA) {
						return;
					}
					const auto source = static_cast<std::int64_t>(a_functionRVA) +
					                    static_cast<std::int64_t>(offset);
					const auto relative = source - a_ownerRVA;
					if (a_occurrence == AutoCallsite::LAST ||
						(a_occurrence != AutoCallsite::UNIQUE && result.matches == a_occurrence)) {
						selected = static_cast<std::ptrdiff_t>(relative);
					}
					if (a_occurrence == AutoCallsite::UNIQUE && result.matches == 0) {
						selected = static_cast<std::ptrdiff_t>(relative);
					}
					++result.matches;
				});
			if (!decoded) {
				return result;
			}

			if (a_occurrence != AutoCallsite::UNIQUE) {
				if (!selected) {
					result.status = CallsiteScanStatus::kNotFound;
					return result;
				}
				result.offset = selected;
				result.status = CallsiteScanStatus::kResolved;
				return result;
			}
			if (result.matches == 0) {
				result.status = CallsiteScanStatus::kNotFound;
				return result;
			}
			if (result.matches != 1) {
				result.status = CallsiteScanStatus::kAmbiguous;
				return result;
			}
			result.offset = selected;
			result.status = CallsiteScanStatus::kResolved;
			return result;
		}

		std::vector<std::ptrdiff_t> find_relative_callsites(
			std::span<const std::uint8_t> a_function,
			std::uint32_t a_functionRVA,
			std::uint32_t a_ownerRVA,
			std::uint32_t a_targetRVA,
			AutoCallsiteBranch a_branch)
		{
			std::vector<std::ptrdiff_t> result;
			if (a_function.size() < 5 ||
				a_branch != AutoCallsiteBranch::kCall &&
				a_branch != AutoCallsiteBranch::kJump &&
				a_branch != AutoCallsiteBranch::kCallOrJump) {
				return result;
			}
			const auto decoded = for_each_direct_relative_branch(
				a_function,
				[&](std::size_t offset, std::uint8_t opcode) {
					const auto accepted =
						a_branch == AutoCallsiteBranch::kCall ? opcode == 0xE8 :
						a_branch == AutoCallsiteBranch::kJump ? opcode == 0xE9 :
						(opcode == 0xE8 || opcode == 0xE9);
					if (!accepted) {
						return;
					}
					std::int32_t displacement{};
					std::memcpy(
						std::addressof(displacement), a_function.data() + offset + 1, sizeof(displacement));
					const auto target = static_cast<std::int64_t>(a_functionRVA) +
					                    static_cast<std::int64_t>(offset) + 5 + displacement;
					if (target == a_targetRVA) {
						const auto relative = static_cast<std::int64_t>(a_functionRVA) +
						                      static_cast<std::int64_t>(offset) - a_ownerRVA;
						result.push_back(static_cast<std::ptrdiff_t>(relative));
					}
				});
			if (!decoded) {
				return {};
			}
			return result;
		}
	}

	CallsiteResolveResult resolve_callsites(
		const ID& a_owner,
		const ID& a_target,
		AutoCallsiteBranch a_branch)
	{
		CallsiteResolveResult result;
		if (a_branch != AutoCallsiteBranch::kCall &&
			a_branch != AutoCallsiteBranch::kJump &&
			a_branch != AutoCallsiteBranch::kCallOrJump) {
			return result;
		}
		const auto owner = IDDatabase::get().resolve(a_owner);
		const auto target = IDDatabase::get().resolve(a_target);
		if (!owner || !target || *owner.rva > UINT32_MAX || *target.rva > UINT32_MAX) {
			result.status = IDResolveStatus::kInvalidCallsite;
			return result;
		}
		const auto ownerRVA = static_cast<std::uint32_t>(*owner.rva);
		const auto scopes = logical_function_scopes(Module::get(), ownerRVA);
		if (scopes.empty()) {
			result.status = IDResolveStatus::kInvalidCallsite;
			return result;
		}
		for (const auto& [functionBegin, functionEnd] : scopes) {
			const auto bytes = std::span(
				reinterpret_cast<const std::uint8_t*>(Module::get().base() + functionBegin),
				static_cast<std::size_t>(functionEnd - functionBegin));
			auto offsets = detail::find_relative_callsites(
				bytes,
				functionBegin,
				ownerRVA,
				static_cast<std::uint32_t>(*target.rva),
				a_branch);
			result.offsets.insert(result.offsets.end(), offsets.begin(), offsets.end());
		}
		std::ranges::sort(result.offsets);
		result.offsets.erase(std::unique(result.offsets.begin(), result.offsets.end()), result.offsets.end());
		result.rvas.reserve(result.offsets.size());
		for (const auto offset : result.offsets) {
			const auto source = static_cast<std::int64_t>(ownerRVA) + offset;
			if (source < 0 || static_cast<std::uint64_t>(source) >= Module::get().image_size()) {
				result.offsets.clear();
				result.rvas.clear();
				result.status = IDResolveStatus::kInvalidCallsite;
				return result;
			}
			result.rvas.push_back(static_cast<std::size_t>(source));
		}
		result.status = result.rvas.empty() ?
		                    IDResolveStatus::kCallsiteNotFound :
		                    IDResolveStatus::kResolvedPattern;
		return result;
	}

	IDDatabase::IDDatabase()
	{
		load();
		dump_requested_mapping();
	}

	IDDatabase::~IDDatabase() = default;

	void IDDatabase::dump_requested_mapping() const
	{
		const auto& paths = diagnostic_paths();
		if (paths.mapping.empty()) {
			return;
		}
		std::error_code error;
		if (!std::filesystem::exists(paths.mapping, error) || error) {
			return;
		}
		std::filesystem::remove(paths.mappingFail, error);
		if (error) {
			spdlog::error(
				"F4RD FAIL mapping reason=fail_cleanup_failed file={}",
				paths.mappingFail.filename().string());
			return;
		}

		std::ofstream mapping(paths.mapping, std::ios::out | std::ios::trunc);
		if (!mapping) {
			spdlog::error(
				"F4RD FAIL mapping reason=open_failed file={}",
				paths.mapping.filename().string());
			return;
		}

		const auto version = Module::get().version();
		const auto isOG = runtime_family(version) == RuntimeFamily::kOG;
		std::size_t total{};
		std::size_t resolved{};
		std::size_t failed{};
		std::ofstream failures;
		bool failureOpenReported{};
		const auto recordFailure = [&](std::uint64_t a_id, IDResolveStatus a_status) {
			++failed;
			if (!failures.is_open()) {
				failures.open(paths.mappingFail, std::ios::out | std::ios::trunc);
			}
			if (failures.is_open()) {
				failures << fmt::format("{} FAIL {}\n", a_id, id_resolve_status_text(a_status));
			} else if (!failureOpenReported) {
				failureOpenReported = true;
				spdlog::error(
					"F4RD FAIL mapping reason=fail_open_failed file={}",
					paths.mappingFail.filename().string());
			}
		};

		if (isOG) {
			total = _id2offset.size();
			for (const auto& entry : _id2offset) {
				mapping << fmt::format("{} 0x{:X}\n", entry.id, entry.offset);
				++resolved;
			}
		} else if (_runtime) {
			const auto results = _runtime->resolve_all_patterns(version, Module::get());
			total = results.size();
			for (const auto& entry : results) {
				if (entry.result) {
					mapping << fmt::format("{} 0x{:X}\n", entry.id, *entry.result.rva);
					++resolved;
				} else {
					recordFailure(entry.id, runtime_failure_status(entry.result.failure));
				}
			}
		} else {
			total = 1;
			recordFailure(0, IDResolveStatus::kUnknownID);
		}

		mapping.flush();
		if (!mapping) {
			spdlog::error(
				"F4RD FAIL mapping reason=write_failed file={}",
				paths.mapping.filename().string());
			if (!failures.is_open()) {
				failures.open(paths.mappingFail, std::ios::out | std::ios::trunc);
			}
			if (failures.is_open()) {
				failures << "0 FAIL mapping_write_failed\n";
				failures.flush();
			}
			return;
		}
		if (failures.is_open()) {
			failures.flush();
			if (!failures) {
				spdlog::error(
					"F4RD FAIL mapping reason=fail_write_failed file={}",
					paths.mappingFail.filename().string());
				return;
			}
		}
		if (failed == 0) {
			spdlog::info(
				"F4RD OK mapping runtime={} total={} resolved={} failed=0 file={}",
				version.string(),
				total,
				resolved,
				paths.mapping.filename().string());
		} else {
			spdlog::error(
				"F4RD FAIL mapping runtime={} total={} resolved={} failed={} file={} failures={}",
				version.string(),
				total,
				resolved,
				failed,
				paths.mapping.filename().string(),
				paths.mappingFail.filename().string());
		}
	}

	std::size_t IDDatabase::id2offset(std::uint64_t a_id) const
	{
		const auto result = resolve(a_id);
		if (result.rva) {
			return *result.rva;
		}
		stl::report_and_fail(fmt::format(
			"REL::ID {} could not be resolved ({})",
			a_id,
			id_resolve_status_text(result.status)));
	}

	std::size_t IDDatabase::id2offset(const ID& a_id) const
	{
		const auto result = resolve(a_id);
		if (result.rva) {
			return *result.rva;
		}
		if (a_id.has_ng_id()) {
			stl::report_and_fail(fmt::format(
				"REL::ID({}, {}, {}) could not be resolved ({})",
				a_id.og_id(),
				a_id.ng_id(),
				a_id.ae_id(),
				id_resolve_status_text(result.status)));
		}
		if (a_id.has_og_id()) {
			stl::report_and_fail(fmt::format(
				"REL::ID({}, {}) could not be resolved ({})",
				a_id.og_id(),
				a_id.ae_id(),
				id_resolve_status_text(result.status)));
		}
		stl::report_and_fail(fmt::format(
			"REL::ID {} could not be resolved ({})",
			a_id.ae_id(),
			id_resolve_status_text(result.status)));
	}

	std::size_t IDDatabase::id2offset(const ID& a_id, const VariantOffset& a_offset) const
	{
		const auto version = Module::get().version();
		auto result = resolve(a_id);
		if (!a_offset.valid(version)) {
			result.rva.reset();
			result.status = IDResolveStatus::kInvalidOffset;
			trace_resolution(result);
			stl::report_and_fail(fmt::format(
				"REL::ID {} received an offset that cannot be represented safely",
				a_id.id(version)));
		}
		if (!result.rva) {
			stl::report_and_fail(fmt::format(
				"REL::ID {} could not be resolved ({})",
				a_id.id(version),
				id_resolve_status_text(result.status)));
		}

		std::ptrdiff_t selectedOffset{};
		bool automaticOffset{};
		if (a_offset.is_auto(version)) {
			const auto* specification = a_offset.auto_callsite(version);
			if (!specification) {
				result.rva.reset();
				result.status = IDResolveStatus::kInvalidCallsite;
				trace_resolution(result);
				stl::report_and_fail(fmt::format(
					"REL::ID {} requested AUTO_OFFSET for {} without an AUTO_CALLSITE target",
					a_id.id(version),
					runtime_family_text(version)));
			}

			const auto targetResult = resolve(specification->target());
			if (!targetResult.rva) {
				result.rva.reset();
				result.status = IDResolveStatus::kInvalidCallsite;
				trace_resolution(result);
				stl::report_and_fail(fmt::format(
					"REL::ID {} automatic callsite target {} could not be resolved ({})",
					a_id.id(version),
					specification->target().id(version),
					id_resolve_status_text(targetResult.status)));
			}

			if (*result.rva > UINT32_MAX || *targetResult.rva > UINT32_MAX) {
				result.rva.reset();
				result.status = IDResolveStatus::kInvalidCallsite;
				trace_resolution(result);
				stl::report_and_fail(fmt::format(
					"REL::ID {} automatic callsite has an out-of-range RVA",
					a_id.id(version)));
			}
			const auto ownerRVA = static_cast<std::uint32_t>(*result.rva);
			const auto targetRVA = static_cast<std::uint32_t>(*targetResult.rva);
			const auto scopes = logical_function_scopes(Module::get(), ownerRVA);
			if (scopes.empty()) {
				result.rva.reset();
				result.status = IDResolveStatus::kInvalidCallsite;
				trace_resolution(result);
				stl::report_and_fail(fmt::format(
					"REL::ID {} automatic callsite owner is not a valid runtime function",
					a_id.id(version)));
			}
			std::vector<std::ptrdiff_t> matches;
			for (const auto& [functionBegin, functionEnd] : scopes) {
				const auto bytes = std::span(
					reinterpret_cast<const std::uint8_t*>(Module::get().base() + functionBegin),
					static_cast<std::size_t>(functionEnd - functionBegin));
				auto offsets = detail::find_relative_callsites(
					bytes,
					functionBegin,
					ownerRVA,
					targetRVA,
					specification->branch());
				matches.insert(matches.end(), offsets.begin(), offsets.end());
			}
			std::ranges::sort(matches);
			matches.erase(std::unique(matches.begin(), matches.end()), matches.end());
			detail::CallsiteScanResult scan;
			scan.matches = matches.size();
			const auto occurrence = specification->occurrence();
			if (matches.empty()) {
				scan.status = detail::CallsiteScanStatus::kNotFound;
			} else if (occurrence == AutoCallsite::UNIQUE) {
				if (matches.size() == 1) {
					scan.offset = matches.front();
					scan.status = detail::CallsiteScanStatus::kResolved;
				} else {
					scan.status = detail::CallsiteScanStatus::kAmbiguous;
				}
			} else {
				const auto index = occurrence == AutoCallsite::LAST ?
				                       matches.size() - 1 :
				                       static_cast<std::size_t>(occurrence);
				if (index < matches.size()) {
					scan.offset = matches[index];
					scan.status = detail::CallsiteScanStatus::kResolved;
				} else {
					scan.status = detail::CallsiteScanStatus::kNotFound;
				}
			}
			if (!scan.offset) {
				result.rva.reset();
				result.status = scan.status == detail::CallsiteScanStatus::kAmbiguous ?
				                    IDResolveStatus::kCallsiteAmbiguous :
				                    scan.status == detail::CallsiteScanStatus::kNotFound ?
				                    IDResolveStatus::kCallsiteNotFound :
				                    IDResolveStatus::kInvalidCallsite;
				trace_resolution(result);
				stl::report_and_fail(fmt::format(
					"REL::ID {} automatic callsite to ID {} could not be resolved ({}, matches={})",
					a_id.id(version),
					specification->target().id(version),
					id_resolve_status_text(result.status),
					scan.matches));
			}
			selectedOffset = *scan.offset;
			automaticOffset = true;
		} else {
			selectedOffset = a_offset.offset(version);
		}

		result.selectedOffset = selectedOffset;
		result.automaticOffset = automaticOffset;
		std::optional<std::size_t> finalRva;
		const auto baseRva = *result.rva;
		if (selectedOffset >= 0) {
			const auto delta = static_cast<std::size_t>(selectedOffset);
			if (delta <= std::numeric_limits<std::size_t>::max() - baseRva) {
				finalRva = baseRva + delta;
			}
		} else {
			const auto magnitude = static_cast<std::size_t>(
				-(selectedOffset + 1)) + 1;
			if (magnitude <= baseRva) {
				finalRva = baseRva - magnitude;
			}
		}
		if (!finalRva || *finalRva >= Module::get().image_size()) {
			result.rva.reset();
			result.finalRva.reset();
			result.status = IDResolveStatus::kInvalidOffset;
			trace_resolution(result);
			stl::report_and_fail(fmt::format(
				"REL::ID {} offset {} resolves outside the loaded module",
				a_id.id(version),
				selectedOffset));
		}
		result.finalRva = *finalRva;
		trace_resolution(result);
		return *finalRva;
	}

	IDResolveResult IDDatabase::resolve(std::uint64_t a_id, IDResolveMode a_mode) const
	{
		return resolve_impl(a_id, a_mode, true);
	}

	IDResolveResult IDDatabase::resolve(const ID& a_id, IDResolveMode a_mode) const
	{
		const auto version = Module::get().version();
		const auto family = runtime_family(version);
		if (family == RuntimeFamily::kAE) {
			return resolve_impl(a_id.ae_id(), a_mode, true);
		}
		if (family == RuntimeFamily::kNG) {
			auto result = resolve_impl(a_id.ng_id(), a_mode, false);
			if (!result && !a_id.has_ng_id()) {
				result.status = IDResolveStatus::kNGBridgeFailed;
			}
			trace_resolution(result);
			return result;
		}
		if (a_mode != IDResolveMode::kNormal) {
			return resolve_impl(a_id.ae_id(), a_mode, true);
		}

		auto pattern = resolve_impl(a_id.ae_id(), IDResolveMode::kRuntimeOnly, false);
		if (pattern) {
			trace_resolution(pattern);
			return pattern;
		}
		if (a_id.has_og_id()) {
			auto legacy = resolve_impl(a_id.og_id(), IDResolveMode::kNormal, false);
			if (legacy) {
				trace_resolution(legacy);
				return legacy;
			}
		}
		if (!a_id.has_og_id()) {
			pattern.status = IDResolveStatus::kOGBridgeFailed;
		}
		trace_resolution(pattern);
		return pattern;
	}

	IDResolveResult IDDatabase::resolve_impl(
		std::uint64_t a_id,
		IDResolveMode a_mode,
		bool a_trace) const
	{
		const auto started = std::chrono::steady_clock::now();
		IDResolveResult result;
		result.id = a_id;
		const auto finish = [&](IDResolveStatus a_status, std::optional<std::size_t> a_rva = std::nullopt) {
			result.status = a_status;
			result.rva = a_rva;
			result.elapsedMicroseconds = static_cast<std::uint64_t>(
				std::chrono::duration_cast<std::chrono::microseconds>(
					std::chrono::steady_clock::now() - started)
					.count());
			if (a_trace) {
				trace_resolution(result);
			}
			return result;
		};

		const auto legacy = [&]() -> std::optional<std::size_t> {
			if (_id2offset.empty()) {
				return std::nullopt;
			}
			const mapping_t elem{ a_id, 0 };
			const auto it = std::lower_bound(
				_id2offset.begin(),
				_id2offset.end(),
				elem,
				[](const mapping_t& a_left, const mapping_t& a_right) {
					return a_left.id < a_right.id;
				});
			return it != _id2offset.end() && it->id == a_id ?
				       std::optional<std::size_t>{ static_cast<std::size_t>(it->offset) } :
				       std::nullopt;
		};
		const auto version = Module::get().version();
		const auto isOG = runtime_family(version) == RuntimeFamily::kOG;
		if (a_mode == IDResolveMode::kNormal && isOG) {
			if (const auto rva = legacy()) {
				return finish(IDResolveStatus::kResolvedLegacy, *rva);
			}
		}

		IDResolveStatus runtimeFailure{ IDResolveStatus::kUnknownID };
		if (_runtime) {
			RuntimeDatabase::ResolveMode mode;
			switch (a_mode) {
			case IDResolveMode::kNormal:
				mode = isOG ? RuntimeDatabase::ResolveMode::kKnownOnly :
				              RuntimeDatabase::ResolveMode::kNormal;
				break;
			case IDResolveMode::kRuntimeOnly:
				mode = RuntimeDatabase::ResolveMode::kNormal;
				break;
			case IDResolveMode::kPatternsOnly:
				mode = RuntimeDatabase::ResolveMode::kPatternsOnly;
				break;
			case IDResolveMode::kPatternsShared:
				mode = RuntimeDatabase::ResolveMode::kPatternsShared;
				break;
			default:
				mode = RuntimeDatabase::ResolveMode::kNormal;
				break;
			}
			const auto resolved = _runtime->resolve_detailed(a_id, version, Module::get(), mode);
			if (resolved) {
				IDResolveStatus status;
				switch (resolved.source) {
				case RuntimeDatabase::ResolveSource::kLocalCache:
					status = IDResolveStatus::kResolvedLocalCache;
					break;
				case RuntimeDatabase::ResolveSource::kKnownRVA:
					status = IDResolveStatus::kResolvedKnownRVA;
					break;
				case RuntimeDatabase::ResolveSource::kSharedCache:
					status = IDResolveStatus::kResolvedSharedCache;
					break;
				case RuntimeDatabase::ResolveSource::kPattern:
					status = IDResolveStatus::kResolvedPattern;
					break;
				default:
					status = IDResolveStatus::kUnresolved;
					break;
				}
				return finish(status, *resolved.rva);
			}
			switch (resolved.failure) {
			case RuntimeDatabase::ResolveFailure::kUnknownID:
				runtimeFailure = IDResolveStatus::kUnknownID;
				break;
			case RuntimeDatabase::ResolveFailure::kNoCompatiblePattern:
				runtimeFailure = IDResolveStatus::kNoCompatiblePattern;
				break;
			case RuntimeDatabase::ResolveFailure::kPatternNotFound:
				runtimeFailure = IDResolveStatus::kPatternNotFound;
				break;
			case RuntimeDatabase::ResolveFailure::kPatternAmbiguous:
				runtimeFailure = IDResolveStatus::kPatternAmbiguous;
				break;
			case RuntimeDatabase::ResolveFailure::kInvalidPattern:
				runtimeFailure = IDResolveStatus::kInvalidPattern;
				break;
			case RuntimeDatabase::ResolveFailure::kRuntimeUnavailable:
				runtimeFailure = IDResolveStatus::kRuntimeUnavailable;
				break;
			case RuntimeDatabase::ResolveFailure::kSemanticValidationFailed:
				runtimeFailure = IDResolveStatus::kSemanticValidationFailed;
				break;
			case RuntimeDatabase::ResolveFailure::kNone:
			default:
				runtimeFailure = IDResolveStatus::kUnresolved;
				break;
			}
		}

		if (a_mode == IDResolveMode::kNormal && isOG) {
			if (const auto rva = legacy()) {
				return finish(IDResolveStatus::kResolvedLegacy, *rva);
			}
		}
		return finish(runtimeFailure);
	}

	IDValidationSummary IDDatabase::validate(
		std::span<const std::uint64_t> a_ids,
		const std::filesystem::path& a_logPath,
		IDResolveMode a_mode) const
	{
		const auto started = std::chrono::steady_clock::now();
		IDValidationSummary summary;
		summary.entries.reserve(a_ids.size());
		std::ofstream log;
		if (!a_logPath.empty()) {
			log.open(a_logPath, std::ios::out | std::ios::trunc);
		}
		if (log.is_open()) {
			log << fmt::format(
				"runtime={} mode={} ids={}\n",
				Module::get().version().string(),
				a_mode == IDResolveMode::kNormal       ? "normal" :
				a_mode == IDResolveMode::kRuntimeOnly  ? "runtime_only" :
				a_mode == IDResolveMode::kPatternsOnly ? "patterns_only" :
														 "patterns_shared",
				a_ids.size());
		}
		for (const auto id : a_ids) {
			auto result = resolve(id, a_mode);
			if (result) {
				++summary.resolved;
			} else {
				++summary.failed;
			}
			if (log.is_open()) {
				const auto rva = result.rva ? fmt::format("0x{:X}", *result.rva) : "-";
				log << fmt::format(
					"id={} rva={} status={} elapsed_us={}\n",
					result.id,
					rva,
					id_resolve_status_text(result.status),
					result.elapsedMicroseconds);
			}
			summary.entries.push_back(std::move(result));
		}
		summary.elapsedMicroseconds = static_cast<std::uint64_t>(
			std::chrono::duration_cast<std::chrono::microseconds>(
				std::chrono::steady_clock::now() - started)
				.count());
		if (log.is_open()) {
			log << fmt::format(
				"summary total={} resolved={} failed={} elapsed_us={}\n",
				summary.entries.size(),
				summary.resolved,
				summary.failed,
				summary.elapsedMicroseconds);
		}
		return summary;
	}

	std::span<const IDDatabase::mapping_t> IDDatabase::get_id2offset()
	{
		const std::scoped_lock lock(_mappingLock);
		if (_id2offset.empty() && _runtime) {
			const auto known = _runtime->known_mappings(Module::get().version());
			_runtimeMappings.reserve(known.size());
			for (const auto& [id, offset] : known) {
				_runtimeMappings.push_back({ id, offset });
			}
			_id2offset = _runtimeMappings;
		}
		return _id2offset;
	}

	void IDDatabase::load()
	{
		const auto& module = Module::get();
		const auto version = module.version();
		const auto isOG = runtime_family(version) == RuntimeFamily::kOG;
		const auto versionedRuntimePath = fmt::format(
			"Data/F4SE/Plugins/f4rd-runtime-{}.bin",
			version.string());
		const std::filesystem::path runtimePath = "Data/F4SE/Plugins/f4rd-runtime.bin";
		std::filesystem::path loadedRuntimePath;
		for (const auto& candidate :
			std::array<std::filesystem::path, 2>{ runtimePath, versionedRuntimePath }) {
			std::error_code error;
			if (std::filesystem::exists(candidate, error) && !error) {
				try {
					_runtime = RuntimeDatabase::load(candidate);
					loadedRuntimePath = candidate;
				} catch (const std::exception& exception) {
					stl::report_and_fail(fmt::format(
						"failed to load {}: {}",
						candidate.string(),
						exception.what()));
				}
				break;
			}
		}

		const auto loadEmbeddedOGTable = [&](const std::filesystem::path& a_path) {
			_id2offset = {};
			_mmap.close();
			if (!_mmap.open(a_path)) {
				return false;
			}
			if (_mmap.size() < sizeof(std::uint64_t)) {
				_mmap.close();
				return false;
			}
			constexpr std::array<std::uint8_t, 8> runtimeMagic{ 'F', '4', 'R', 'D', 'B', 'I', 'N', 0 };
			if (std::equal(runtimeMagic.begin(), runtimeMagic.end(),
					reinterpret_cast<const std::uint8_t*>(_mmap.data()))) {
				_mmap.close();
				return false;
			}
			std::uint64_t count{};
			std::memcpy(std::addressof(count), _mmap.data(), sizeof(count));
			const auto available = _mmap.size() - sizeof(std::uint64_t);
			if (count > available / sizeof(mapping_t)) {
				_mmap.close();
				return false;
			}
			const auto entries = std::span{
				reinterpret_cast<const mapping_t*>(_mmap.data() + sizeof(std::uint64_t)),
				static_cast<std::size_t>(count)
			};
			for (std::size_t index = 0; index < entries.size(); ++index) {
				if (entries[index].offset >= module.image_size() ||
					(index != 0 && entries[index - 1].id >= entries[index].id)) {
					_mmap.close();
					return false;
				}
			}
			_id2offset = entries;
			return true;
		};

		const auto loadedOGTable = isOG && !loadedRuntimePath.empty() &&
			loadEmbeddedOGTable(loadedRuntimePath);
		if (!loadedOGTable && !_runtimeMappings.empty()) {
			_id2offset = _runtimeMappings;
		} else if (!loadedOGTable && !_runtime) {
			stl::report_and_fail(fmt::format(
				"failed to open {} or {}",
				runtimePath.string(),
				versionedRuntimePath));
		}
	}

	void Module::load_segments()
	{
		auto dosHeader = reinterpret_cast<const ::IMAGE_DOS_HEADER*>(_base);
		auto ntHeader = stl::adjust_pointer<::IMAGE_NT_HEADERS64>(dosHeader, dosHeader->e_lfanew);
		_imageSize = ntHeader->OptionalHeader.SizeOfImage;
		_preferredBase = ntHeader->OptionalHeader.ImageBase;
		const auto* sections = IMAGE_FIRST_SECTION(ntHeader);
		const auto size = static_cast<std::size_t>(ntHeader->FileHeader.NumberOfSections);
		for (std::size_t i = 0; i < size; ++i) {
			const auto& section = sections[i];
			const auto sectionNameEnd = std::find(
				std::begin(section.Name), std::end(section.Name), std::uint8_t{});
			const auto sectionNameLength = static_cast<std::size_t>(
				std::distance(std::begin(section.Name), sectionNameEnd));
			const auto it = std::find_if(
				SEGMENTS.begin(),
				SEGMENTS.end(),
				[&](auto&& a_elem) {
					return a_elem.size() == sectionNameLength &&
					       std::memcmp(a_elem.data(), section.Name, sectionNameLength) == 0;
				});
			if (it != SEGMENTS.end()) {
				const auto idx = static_cast<std::size_t>(std::distance(SEGMENTS.begin(), it));
				_segments[idx] = Segment{ _base, _base + section.VirtualAddress, section.Misc.VirtualSize };
			}
		}
	}
}
