#include "RuntimeDatabase.h"
#include "InstructionDecoder.h"

#include <fstream>
#include <future>
#include <thread>

namespace
{
	constexpr std::array<std::uint8_t, 8> MAGIC{ 'F', '4', 'R', 'D', 'B', 'I', 'N', 0 };
	constexpr std::uint32_t ENDIAN_MARKER = 0x01020304;
	constexpr std::size_t HEADER_SIZE = 112;
	constexpr std::size_t RECORD_SIZE = 40;
	constexpr std::size_t ALIAS_SIZE = 24;
	constexpr std::size_t KNOWN_RVA_SIZE = 16;
	constexpr std::size_t CANDIDATE_SIZE_V1 = 32;
	constexpr std::size_t CANDIDATE_SIZE = 40;
	constexpr std::size_t FRAGMENT_SIZE = 24;
	constexpr std::size_t CONSTRAINT_SIZE = 24;
	constexpr std::size_t MAX_INTERMEDIATE_MATCHES = 4096;
	constexpr std::size_t MAX_RESOLUTION_DEPTH = 64;
	constexpr std::uint32_t FRAGMENT_REVERSE = 1u;
	constexpr std::uint16_t CANDIDATE_VERSION_MAJOR_MINOR = 1u;
	constexpr std::uint16_t CANDIDATE_RESULT_TEXT = 8u;
	constexpr std::uint16_t CANDIDATE_RESULT_RDATA = 16u;
	constexpr std::uint16_t CANDIDATE_RESULT_DATA = 32u;
	constexpr std::uint16_t CANDIDATE_RESULT_IDATA = 64u;
	constexpr std::uint16_t CANDIDATE_RESULT_ALIGN8 = 128u;
	constexpr std::uint16_t CANDIDATE_RESULT_SECTION_MASK =
		CANDIDATE_RESULT_TEXT | CANDIDATE_RESULT_RDATA |
		CANDIDATE_RESULT_DATA | CANDIDATE_RESULT_IDATA;
	constexpr std::uint16_t CANDIDATE_RIP_TRAILING_BYTES_MASK = 0xFF00u;
	constexpr unsigned CANDIDATE_RIP_TRAILING_BYTES_SHIFT = 8u;
	constexpr std::uint32_t ALIAS_VERSION_MAJOR_MINOR = 1u;
	constexpr std::uint32_t RECORD_FUNCTION = 1u;
	constexpr std::uint32_t RECORD_MSVC_RTTI = 2u;
	constexpr std::uint32_t RECORD_VTABLE = 4u;
	constexpr std::uint32_t RECORD_NIRTTI = 8u;
	constexpr std::uint32_t RECORD_SINGLETON = 16u;
	constexpr std::uint32_t RECORD_EVENT = 32u;
	constexpr std::uint32_t RECORD_GLOBAL = 64u;
	constexpr std::uint32_t RECORD_REQUIRE_FUNCTION_START = 128u;
	constexpr std::uint32_t RECORD_SEMANTIC_MASK =
		RECORD_FUNCTION | RECORD_MSVC_RTTI | RECORD_VTABLE | RECORD_NIRTTI |
		RECORD_SINGLETON | RECORD_EVENT | RECORD_GLOBAL;
	constexpr std::uint32_t RECORD_ALL_FLAGS =
		RECORD_SEMANTIC_MASK | RECORD_REQUIRE_FUNCTION_START;
	constexpr std::uint8_t RESOLUTION_RELATIVE_TO_ID = 20;
	constexpr std::uint8_t RESOLUTION_PDATA_ORDINAL_FROM_ID = 21;
	constexpr std::uint8_t RESOLUTION_RDATA_VTABLE_FROM_IDS = 22;
	constexpr std::uint8_t RESOLUTION_RIP_RELATIVE_TARGET_IN_ID = 23;
	constexpr std::uint8_t RESOLUTION_RDATA_VTABLE_FROM_SLOT_PATTERNS = 24;
	constexpr std::uint8_t RESOLUTION_MSVC_TYPE_DESCRIPTOR_FROM_VTABLE_ID = 25;
	constexpr std::uint8_t RESOLUTION_IMPORT_THUNK_FROM_NAME = 26;
	constexpr std::uint8_t RESOLUTION_RUNTIME_UNAVAILABLE = 27;
	constexpr std::uint8_t RESOLUTION_FUNCTION_FROM_RIP_RELATIVE_STRING = 28;
	constexpr std::size_t MAX_SCOPED_FUNCTION_BYTES = 256u * 1024u;
	constexpr std::uint8_t CONSTRAINT_RELATIVE32_TARGET = 1;
	constexpr std::uint8_t CONSTRAINT_RESOLVED_ID = 2;
	constexpr std::uint8_t CONSTRAINT_POINTED_PATTERN = 3;

	enum class RuntimeFamilyKey : std::uint8_t
	{
		kOG,
		kNG,
		kAE
	};

	[[nodiscard]] constexpr RuntimeFamilyKey runtime_family_key(
		const std::array<std::uint16_t, 4>& a_version) noexcept
	{
		if (a_version >= std::array<std::uint16_t, 4>{ 1, 11, 0, 0 }) {
			return RuntimeFamilyKey::kAE;
		}
		if (a_version >= std::array<std::uint16_t, 4>{ 1, 10, 980, 0 }) {
			return RuntimeFamilyKey::kNG;
		}
		return RuntimeFamilyKey::kOG;
	}

	[[nodiscard]] constexpr bool version_scope_matches(
		const std::array<std::uint16_t, 4>& a_source,
		const std::array<std::uint16_t, 4>& a_target,
		bool a_majorMinor) noexcept
	{
		if (!a_majorMinor) {
			return a_source == a_target;
		}
		return a_source[0] == a_target[0] && a_source[1] == a_target[1] &&
		       runtime_family_key(a_source) == runtime_family_key(a_target);
	}

	template <class AliasRange>
	void validate_alias_scopes(const AliasRange& a_aliases)
	{
		for (std::size_t begin = 0; begin < a_aliases.size();) {
			auto end = begin + 1;
			while (end < a_aliases.size() && a_aliases[end].id == a_aliases[begin].id) {
				++end;
			}
			for (auto left = begin; left < end; ++left) {
				for (auto right = left + 1; right < end; ++right) {
					if (a_aliases[left].recordIndex == a_aliases[right].recordIndex) {
						continue;
					}
					const auto leftScoped =
						(a_aliases[left].flags & ALIAS_VERSION_MAJOR_MINOR) != 0;
					const auto rightScoped =
						(a_aliases[right].flags & ALIAS_VERSION_MAJOR_MINOR) != 0;
					const auto exactConflict = !leftScoped && !rightScoped &&
						a_aliases[left].version == a_aliases[right].version;
					const auto scopedConflict = leftScoped && rightScoped &&
						version_scope_matches(
							a_aliases[left].version.parts,
							a_aliases[right].version.parts,
							true);
					if (exactConflict || scopedConflict) {
						throw std::runtime_error(
							"runtime database alias maps to multiple canonical IDs in one scope");
					}
				}
			}
			begin = end;
		}
	}

	[[nodiscard]] constexpr bool candidate_scope_matches(
		const std::array<std::uint16_t, 4>& a_source,
		const std::array<std::uint16_t, 4>& a_target,
		std::uint16_t a_flags) noexcept
	{
		return a_source == std::array<std::uint16_t, 4>{} ||
		       version_scope_matches(a_source, a_target,
			   (a_flags & CANDIDATE_VERSION_MAJOR_MINOR) != 0);
	}

	[[nodiscard]] constexpr bool is_id_only_dependency(std::uint8_t a_resolution) noexcept
	{
		return a_resolution == RESOLUTION_RELATIVE_TO_ID ||
		       a_resolution == RESOLUTION_PDATA_ORDINAL_FROM_ID ||
		       a_resolution == RESOLUTION_RDATA_VTABLE_FROM_IDS ||
		       a_resolution == RESOLUTION_MSVC_TYPE_DESCRIPTOR_FROM_VTABLE_ID;
	}

	[[nodiscard]] constexpr bool is_scoped_rip_dependency(std::uint8_t a_resolution) noexcept
	{
		return a_resolution == RESOLUTION_RIP_RELATIVE_TARGET_IN_ID;
	}

	[[nodiscard]] constexpr bool is_slot_pattern_dependency(std::uint8_t a_resolution) noexcept
	{
		return a_resolution == RESOLUTION_RDATA_VTABLE_FROM_SLOT_PATTERNS;
	}

	[[nodiscard]] constexpr bool is_dependent_resolution(std::uint8_t a_resolution) noexcept
	{
		return is_id_only_dependency(a_resolution) ||
		       is_scoped_rip_dependency(a_resolution) ||
		       is_slot_pattern_dependency(a_resolution);
	}

	[[nodiscard]] constexpr bool is_unavailable_resolution(std::uint8_t a_resolution) noexcept
	{
		return a_resolution == RESOLUTION_RUNTIME_UNAVAILABLE;
	}

	[[nodiscard]] std::size_t bulk_worker_count(
		std::size_t a_workItems,
		std::size_t a_minimumItemsPerWorker,
		std::size_t a_maxWorkers = 16)
	{
		const auto hardwareWorkers = std::max<std::size_t>(
			1, static_cast<std::size_t>(std::thread::hardware_concurrency()));
		const auto usefulWorkers = a_workItems == 0 ? 1 :
			1 + (a_workItems - 1) / a_minimumItemsPerWorker;
		return std::min({ hardwareWorkers, usefulWorkers, a_maxWorkers });
	}

	template <class T>
	[[nodiscard]] T read_le(std::span<const std::uint8_t> a_input, std::size_t a_offset)
	{
		static_assert(std::is_integral_v<T>);
		if (a_offset > a_input.size() || sizeof(T) > a_input.size() - a_offset) {
			throw std::runtime_error("runtime database read is out of range");
		}
		using U = std::make_unsigned_t<T>;
		U value{};
		for (std::size_t i = 0; i < sizeof(T); ++i) {
			value |= static_cast<U>(a_input[a_offset + i]) << (i * 8);
		}
		return static_cast<T>(value);
	}

	struct PackedPatternLayout
	{
		std::size_t bitmapSize{};
		std::size_t valueCount{};
		std::size_t partialOffset{};
		std::size_t partialCount{};
	};

	[[nodiscard]] PackedPatternLayout packed_pattern_layout(
		std::span<const std::uint8_t> a_bytes,
		std::size_t a_data,
		std::uint32_t a_dataSize,
		std::uint32_t a_length)
	{
		const auto bitmapSize = (static_cast<std::size_t>(a_length) + 7) / 8;
		if (a_length == 0 || a_data > a_bytes.size() ||
			a_dataSize > a_bytes.size() - a_data || a_dataSize < bitmapSize) {
			throw std::runtime_error("runtime database packed pattern is truncated");
		}
		std::size_t valueCount{};
		for (std::size_t index = 0; index < a_length; ++index) {
			valueCount += (a_bytes[a_data + index / 8] >> (index % 8)) & 1u;
		}
		if (a_length % 8 != 0) {
			const auto validBits = static_cast<std::uint8_t>((1u << (a_length % 8)) - 1u);
			if ((a_bytes[a_data + bitmapSize - 1] & ~validBits) != 0) {
				throw std::runtime_error("runtime database packed pattern padding is invalid");
			}
		}
		if (valueCount > a_dataSize - bitmapSize) {
			throw std::runtime_error("runtime database packed pattern values are truncated");
		}
		const auto partialOffset = a_data + bitmapSize + valueCount;
		const auto partialBytes = a_data + a_dataSize - partialOffset;
		if (partialBytes % 5 != 0) {
			throw std::runtime_error("runtime database packed pattern exceptions are invalid");
		}
		const auto partialCount = partialBytes / 5;
		for (std::size_t partial = 0; partial < partialCount; ++partial) {
			const auto offset = partialOffset + partial * 5;
			const auto index = read_le<std::uint32_t>(a_bytes, offset);
			const auto mask = a_bytes[offset + 4];
			if (index >= a_length || mask == 0 || mask == 0xFF ||
				((a_bytes[a_data + index / 8] >> (index % 8)) & 1u) == 0) {
				throw std::runtime_error("runtime database packed pattern exception is invalid");
			}
			for (std::size_t previous = 0; previous < partial; ++previous) {
				if (read_le<std::uint32_t>(a_bytes, partialOffset + previous * 5) == index) {
					throw std::runtime_error("runtime database packed pattern exception is duplicated");
				}
			}
		}
		return { bitmapSize, valueCount, partialOffset, partialCount };
	}

	struct DecodedPattern
	{
		std::vector<std::uint8_t> values;
		std::vector<std::uint8_t> masks;
	};

	void decode_packed_pattern_into(
		std::span<const std::uint8_t> a_bytes,
		std::size_t a_data,
		std::uint32_t a_dataSize,
		std::uint32_t a_length,
		std::span<std::uint8_t> a_values,
		std::span<std::uint8_t> a_masks)
	{
		if (a_values.size() != a_length || a_masks.size() != a_length) {
			throw std::runtime_error("runtime database packed pattern destination is invalid");
		}
		const auto layout = packed_pattern_layout(a_bytes, a_data, a_dataSize, a_length);
		std::ranges::fill(a_values, std::uint8_t{});
		std::ranges::fill(a_masks, std::uint8_t{});
		auto valueOffset = a_data + layout.bitmapSize;
		for (std::size_t index = 0; index < a_length; ++index) {
			if (((a_bytes[a_data + index / 8] >> (index % 8)) & 1u) == 0) {
				continue;
			}
			a_values[index] = a_bytes[valueOffset++];
			a_masks[index] = 0xFF;
		}
		for (std::size_t partial = 0; partial < layout.partialCount; ++partial) {
			const auto offset = layout.partialOffset + partial * 5;
			const auto index = read_le<std::uint32_t>(a_bytes, offset);
			const auto mask = a_bytes[offset + 4];
			a_masks[index] = mask;
			a_values[index] &= mask;
		}
	}

	[[nodiscard]] DecodedPattern decode_packed_pattern(
		std::span<const std::uint8_t> a_bytes,
		std::size_t a_data,
		std::uint32_t a_dataSize,
		std::uint32_t a_length)
	{
		DecodedPattern result;
		result.values.resize(a_length);
		result.masks.resize(a_length);
		decode_packed_pattern_into(
			a_bytes, a_data, a_dataSize, a_length, result.values, result.masks);
		return result;
	}

	[[nodiscard]] std::array<std::uint16_t, 4> read_version(
		std::span<const std::uint8_t> a_input,
		std::size_t a_offset)
	{
		std::array<std::uint16_t, 4> version{};
		for (std::size_t i = 0; i < version.size(); ++i) {
			version[i] = read_le<std::uint16_t>(a_input, a_offset + i * sizeof(std::uint16_t));
		}
		return version;
	}

	[[nodiscard]] std::uint32_t crc32(std::span<const std::uint8_t> a_bytes)
	{
		std::uint32_t crc = 0xFFFFFFFFu;
		for (const auto byte : a_bytes) {
			crc ^= byte;
			for (unsigned bit = 0; bit < 8; ++bit) {
				const auto mask = static_cast<std::uint32_t>(0u - (crc & 1u));
				crc = (crc >> 1) ^ (0xEDB88320u & mask);
			}
		}
		return ~crc;
	}

	void validate_table(
		std::size_t a_offset,
		std::uint32_t a_count,
		std::size_t a_elementSize,
		std::size_t a_nextOffset)
	{
		if (a_count > std::numeric_limits<std::size_t>::max() / a_elementSize) {
			throw std::runtime_error("runtime database table size overflows");
		}
		const auto bytes = static_cast<std::size_t>(a_count) * a_elementSize;
		if (a_offset > a_nextOffset || bytes > a_nextOffset - a_offset) {
			throw std::runtime_error("runtime database tables overlap");
		}
	}

	[[nodiscard]] std::size_t checked_offset(std::uint64_t a_value, std::size_t a_size)
	{
		if (a_value > a_size || a_value > std::numeric_limits<std::size_t>::max()) {
			throw std::runtime_error("runtime database table offset is invalid");
		}
		return static_cast<std::size_t>(a_value);
	}

	struct ChosenAnchor
	{
		std::uint64_t value{};
		std::uint32_t offset{};
		std::uint8_t length{};
	};

	struct AnchorReference
	{
		std::uint64_t value{};
		std::size_t patternIndex{};
		std::uint32_t patternOffset{};
		std::uint8_t length{};

		[[nodiscard]] bool operator<(const AnchorReference& a_other) const noexcept
		{
			return value < a_other.value;
		}
	};

	[[nodiscard]] std::uint64_t load_anchor(const std::uint8_t* a_bytes, std::size_t a_length)
	{
		std::uint64_t value{};
		std::memcpy(std::addressof(value), a_bytes, a_length);
		return value;
	}

	[[nodiscard]] std::uint64_t pattern_hash(
		std::uint8_t a_section,
		std::span<const std::uint8_t> a_values,
		std::span<const std::uint8_t> a_masks)
	{
		std::uint64_t hash = 1469598103934665603ull;
		const auto mix = [&](std::uint8_t a_byte) {
			hash ^= a_byte;
			hash *= 1099511628211ull;
		};
		mix(a_section);
		for (std::size_t index = 0; index < a_values.size(); ++index) {
			mix(a_masks[index]);
			mix(a_values[index] & a_masks[index]);
		}
		for (auto length = a_values.size(); length != 0; length >>= 8) {
			mix(static_cast<std::uint8_t>(length));
		}
		return hash;
	}

	[[nodiscard]] std::uint64_t next_pattern_hash(std::uint64_t a_hash) noexcept
	{
		a_hash ^= a_hash >> 30;
		a_hash *= 0xBF58476D1CE4E5B9ull;
		a_hash ^= a_hash >> 27;
		a_hash *= 0x94D049BB133111EBull;
		a_hash ^= a_hash >> 31;
		return a_hash + 0x9E3779B97F4A7C15ull;
	}

	[[nodiscard]] bool same_pattern(
		std::span<const std::uint8_t> a_leftValues,
		std::span<const std::uint8_t> a_leftMasks,
		std::span<const std::uint8_t> a_rightValues,
		std::span<const std::uint8_t> a_rightMasks)
	{
		if (a_leftValues.size() != a_rightValues.size() ||
			a_leftMasks.size() != a_rightMasks.size() ||
			a_leftValues.size() != a_leftMasks.size()) {
			return false;
		}
		for (std::size_t index = 0; index < a_leftValues.size(); ++index) {
			if (a_leftMasks[index] != a_rightMasks[index] ||
				(a_leftValues[index] & a_leftMasks[index]) !=
					(a_rightValues[index] & a_rightMasks[index])) {
				return false;
			}
		}
		return true;
	}

	struct ChosenAnchors
	{
		std::array<ChosenAnchor, 8> values;
		std::size_t count{};
	};

	[[nodiscard]] ChosenAnchors choose_anchors(
		std::span<const std::uint8_t> a_values,
		std::span<const std::uint8_t> a_masks)
	{
		ChosenAnchors result;
		for (const auto length : { std::size_t{ 8 }, std::size_t{ 4 } }) {
			if (a_values.size() < length) {
				continue;
			}
			std::size_t available{};
			const auto enumerate = [&](const auto& a_consumer) {
				for (std::size_t runBegin = 0; runBegin < a_masks.size();) {
					while (runBegin < a_masks.size() && a_masks[runBegin] != 0xFF) {
						++runBegin;
					}
					auto runEnd = runBegin;
					while (runEnd < a_masks.size() && a_masks[runEnd] == 0xFF) {
						++runEnd;
					}
					if (runEnd - runBegin >= length) {
						for (auto offset = runBegin; offset + length <= runEnd; offset += 4) {
							a_consumer(offset);
						}
						const auto last = runEnd - length;
						if ((last - runBegin) % 4 != 0) {
							a_consumer(last);
						}
					}
					runBegin = runEnd + (runEnd == runBegin ? 1 : 0);
				}
			};
			enumerate([&](std::size_t) { ++available; });
			if (available == 0) {
				continue;
			}
			const auto wanted = std::min(available, result.values.size());
			std::size_t ordinal{};
			std::size_t nextSample{};
			enumerate([&](std::size_t a_offset) {
				const auto target = wanted == 1 ? 0 :
				                    nextSample * (available - 1) / (wanted - 1);
				if (ordinal == target && nextSample < wanted) {
					result.values[result.count++] = {
						load_anchor(a_values.data() + a_offset, length),
						static_cast<std::uint32_t>(a_offset),
						static_cast<std::uint8_t>(length)
					};
					++nextSample;
				}
				++ordinal;
			});
			if (result.count != 0) {
				return result;
			}
		}
		return result;
	}

	void validate_mapped_database_once(
		const std::filesystem::path& a_path,
		std::uint32_t a_payloadCRC,
		std::size_t a_size,
		const std::function<void()>& a_validator)
	{
		std::error_code error;
		auto identityPath = std::filesystem::weakly_canonical(a_path, error);
		if (error) {
			error.clear();
			identityPath = std::filesystem::absolute(a_path, error);
		}
		if (error) {
			a_validator();
			return;
		}
		const auto writeTime = std::filesystem::last_write_time(identityPath, error);
		if (error) {
			a_validator();
			return;
		}
		std::uint64_t pathHash = 1469598103934665603ull;
		for (const auto character : identityPath.native()) {
			pathHash ^= static_cast<std::uint64_t>(character);
			pathHash *= 1099511628211ull;
		}
		const auto writeTimeValue = static_cast<std::uint64_t>(
			writeTime.time_since_epoch().count());

		struct State
		{
			std::uint64_t magic{};
			std::uint32_t layout{};
			std::uint32_t status{};
			std::uint32_t payloadCRC{};
			std::uint32_t reserved{};
			std::uint64_t size{};
			std::uint64_t pathHash{};
			std::uint64_t writeTime{};
		};
		constexpr std::uint64_t validationMagic = 0x3144494C41564452;
		constexpr std::uint32_t validationLayout = 2;
		const auto suffix = fmt::format("{}.{}.{}.{}.{}.v{}",
			REL::WinAPI::GetCurrentProcessID(),
			a_payloadCRC,
			a_size,
			pathHash,
			writeTimeValue,
			validationLayout);
		const auto mappingText = fmt::format("Local\\F4RD.DatabaseValidation.{}", suffix);
		const auto mutexText = fmt::format("Local\\F4RD.DatabaseValidation.Mutex.{}", suffix);
		const std::wstring mappingName(mappingText.begin(), mappingText.end());
		const std::wstring mutexName(mutexText.begin(), mutexText.end());
		const auto closeHandle = [](void* a_handle) {
			if (a_handle) {
				(void)REL::WinAPI::CloseHandle(a_handle);
			}
		};
		const auto unmapView = [](void* a_view) {
			if (a_view) {
				(void)REL::WinAPI::UnmapViewOfFile(a_view);
			}
		};
		const auto releaseMutex = [](void* a_mutex) {
			if (a_mutex) {
				(void)REL::WinAPI::ReleaseMutex(a_mutex);
			}
		};
		std::unique_ptr<void, decltype(closeHandle)> mutex(
			REL::WinAPI::CreateMutex(mutexName.c_str()), closeHandle);
		if (!mutex) {
			a_validator();
			return;
		}
		std::unique_ptr<void, decltype(closeHandle)> mapping(
			REL::WinAPI::CreateFileMapping(sizeof(State), mappingName.c_str()), closeHandle);
		if (!mapping) {
			a_validator();
			return;
		}
		std::unique_ptr<void, decltype(unmapView)> view(
			REL::WinAPI::MapViewOfFile(mapping.get(), sizeof(State)), unmapView);
		if (!view || !REL::WinAPI::WaitForMutex(mutex.get())) {
			a_validator();
			return;
		}
		std::unique_ptr<void, decltype(releaseMutex)> locked(mutex.get(), releaseMutex);
		auto* state = static_cast<State*>(view.get());
		if (state->magic != validationMagic || state->layout != validationLayout ||
			state->payloadCRC != a_payloadCRC || state->size != a_size ||
			state->pathHash != pathHash || state->writeTime != writeTimeValue) {
			std::memset(state, 0, sizeof(State));
			state->magic = validationMagic;
			state->layout = validationLayout;
			state->payloadCRC = a_payloadCRC;
			state->size = a_size;
			state->pathHash = pathHash;
			state->writeTime = writeTimeValue;
		}
		if (state->status == 1) {
			return;
		}
		if (state->status == 2) {
			throw std::runtime_error("runtime database validation previously failed");
		}
		try {
			a_validator();
			state->status = 1;
			(void)mapping.release();
		} catch (...) {
			state->status = 2;
			(void)mapping.release();
			throw;
		}
	}
	template <class Callback>
	void for_each_decoded_relative_branch(const REL::Module& a_module, Callback&& a_callback)
	{
		struct RuntimeFunction
		{
			std::uint32_t begin;
			std::uint32_t end;
			std::uint32_t unwind;
		};

		const auto pdata = a_module.segment(REL::Segment::pdata);
		if (pdata.size() < sizeof(RuntimeFunction) || pdata.size() % sizeof(RuntimeFunction) != 0) {
			return;
		}
		const auto functions = std::span(
			pdata.pointer<const RuntimeFunction>(), pdata.size() / sizeof(RuntimeFunction));
		const auto text = a_module.segment(REL::Segment::text);
		const auto textBegin = static_cast<std::uint64_t>(text.offset());
		const auto textEnd = textBegin + text.size();
		for (const auto& function : functions) {
			if (function.begin < textBegin || function.end <= function.begin || function.end > textEnd) {
				continue;
			}
			const auto functionSize = static_cast<std::size_t>(function.end - function.begin);
			const auto code = std::span(
				reinterpret_cast<const std::uint8_t*>(a_module.base() + function.begin), functionSize);
			std::vector<std::pair<std::uint32_t, std::size_t>> branches;
			const auto decoded = REL::detail::for_each_direct_relative_branch(
				code,
				[&](std::size_t a_offset, std::uint8_t) {
					std::int32_t displacement{};
					std::memcpy(std::addressof(displacement), code.data() + a_offset + 1,
						sizeof(displacement));
					const auto target = static_cast<std::int64_t>(function.begin) +
					                    static_cast<std::int64_t>(a_offset) + 5 + displacement;
					if (target >= 0 && static_cast<std::uint64_t>(target) < a_module.image_size()) {
						branches.emplace_back(
							static_cast<std::uint32_t>(target),
							static_cast<std::size_t>(function.begin - textBegin) + a_offset);
					}
				});
			if (decoded) {
				for (const auto& [target, offset] : branches) {
					std::invoke(a_callback, target, offset);
				}
			}
		}
	}
}

namespace REL
{
	class RuntimeDatabase::SharedCache
	{
	private:
		struct Entry
		{
			std::uint64_t id{};
			std::uint32_t rva{};
			std::uint32_t state{};
		};

		static constexpr std::uint64_t CACHE_MAGIC = 0x3145484341434452;
		static constexpr std::uint32_t CACHE_LAYOUT = 3;
		static constexpr std::uint32_t CACHE_CAPACITY = 32768;

		struct State
		{
			std::uint64_t magic{};
			std::uint32_t layout{};
			std::uint32_t capacity{};
			std::uint32_t payloadCRC{};
			std::array<std::uint16_t, 4> version{};
			std::uint32_t reserved{};
			std::array<Entry, CACHE_CAPACITY> entries{};
		};

		struct MutexRelease
		{
			void* mutex{};

			~MutexRelease()
			{
				if (mutex) {
					(void)WinAPI::ReleaseMutex(mutex);
				}
			}
		};

	public:
		struct Result
		{
			std::optional<std::uint32_t> rva;
			ResolveFailure failure{ ResolveFailure::kNone };
			bool cached{};
		};

		SharedCache(const Version& a_version, std::uint32_t a_payloadCRC)
		{
			const auto suffix = fmt::format("{}.{}.{}.v{}",
				WinAPI::GetCurrentProcessID(), a_version.string(), a_payloadCRC, CACHE_LAYOUT);
			const auto mappingText = fmt::format("Local\\F4RD.Cache.{}", suffix);
			const auto mutexText = fmt::format("Local\\F4RD.Cache.Mutex.{}", suffix);
			const std::wstring mappingName(mappingText.begin(), mappingText.end());
			const std::wstring mutexName(mutexText.begin(), mutexText.end());

			_mutex = WinAPI::CreateMutex(mutexName.c_str());
			if (!_mutex) {
				return;
			}
			_mapping = WinAPI::CreateFileMapping(sizeof(State), mappingName.c_str());
			if (!_mapping) {
				return;
			}
			_state = static_cast<State*>(WinAPI::MapViewOfFile(_mapping, sizeof(State)));
			if (!_state || !WinAPI::WaitForMutex(_mutex)) {
				return;
			}
			const MutexRelease release{ _mutex };
			std::array<std::uint16_t, 4> version{};
			for (std::size_t i = 0; i < version.size(); ++i) {
				version[i] = a_version[i];
			}
			if (_state->magic == 0) {
				std::memset(_state, 0, sizeof(State));
				_state->magic = CACHE_MAGIC;
				_state->layout = CACHE_LAYOUT;
				_state->capacity = CACHE_CAPACITY;
				_state->payloadCRC = a_payloadCRC;
				_state->version = version;
			}
			_valid = _state->magic == CACHE_MAGIC &&
			         _state->layout == CACHE_LAYOUT &&
			         _state->capacity == CACHE_CAPACITY &&
			         _state->payloadCRC == a_payloadCRC &&
			         _state->version == version;
		}

		~SharedCache()
		{
			if (_state) {
				(void)WinAPI::UnmapViewOfFile(_state);
			}
			if (_mapping) {
				(void)WinAPI::CloseHandle(_mapping);
			}
			if (_mutex) {
				(void)WinAPI::CloseHandle(_mutex);
			}
		}

		[[nodiscard]] Result resolve(
			std::uint64_t a_id,
			const std::function<PatternResolveResult()>& a_scan) const
		{
			if (!_valid || a_id == 0 || !WinAPI::WaitForMutex(_mutex)) {
				const auto scanned = a_scan();
				return { scanned.rva, scanned.failure, false };
			}
			const MutexRelease release{ _mutex };
			Entry* empty{};
			auto index = hash(a_id) % CACHE_CAPACITY;
			for (std::uint32_t probe = 0; probe < CACHE_CAPACITY; ++probe) {
				auto& entry = _state->entries[index];
				if (entry.state == 0) {
					empty = std::addressof(entry);
					break;
				}
				if (entry.id == a_id) {
					return entry.state == 1 ?
					           Result{ entry.rva, ResolveFailure::kNone, true } :
					           Result{ std::nullopt, decode_failure(entry.state), true };
				}
				index = (index + 1) % CACHE_CAPACITY;
			}
			const auto resolved = a_scan();
			if (empty) {
				empty->id = a_id;
				empty->rva = resolved.rva.value_or(0);
				empty->state = resolved.rva ? 1u : encode_failure(resolved.failure);
			}
			return { resolved.rva, resolved.failure, false };
		}

	private:
		[[nodiscard]] static std::uint32_t encode_failure(ResolveFailure a_failure) noexcept
		{
			switch (a_failure) {
			case ResolveFailure::kNoCompatiblePattern:
				return 3;
			case ResolveFailure::kPatternAmbiguous:
				return 4;
			case ResolveFailure::kInvalidPattern:
				return 5;
			case ResolveFailure::kUnknownID:
				return 6;
			case ResolveFailure::kRuntimeUnavailable:
				return 7;
			case ResolveFailure::kSemanticValidationFailed:
				return 8;
			case ResolveFailure::kPatternNotFound:
			case ResolveFailure::kNone:
			default:
				return 2;
			}
		}

		[[nodiscard]] static ResolveFailure decode_failure(std::uint32_t a_state) noexcept
		{
			switch (a_state) {
			case 3:
				return ResolveFailure::kNoCompatiblePattern;
			case 4:
				return ResolveFailure::kPatternAmbiguous;
			case 5:
				return ResolveFailure::kInvalidPattern;
			case 6:
				return ResolveFailure::kUnknownID;
			case 7:
				return ResolveFailure::kRuntimeUnavailable;
			case 8:
				return ResolveFailure::kSemanticValidationFailed;
			case 2:
			default:
				return ResolveFailure::kPatternNotFound;
			}
		}

		[[nodiscard]] static std::uint32_t hash(std::uint64_t a_id) noexcept
		{
			a_id ^= a_id >> 33;
			a_id *= 0xFF51AFD7ED558CCDull;
			a_id ^= a_id >> 33;
			a_id *= 0xC4CEB9FE1A85EC53ull;
			a_id ^= a_id >> 33;
			return static_cast<std::uint32_t>(a_id ^ (a_id >> 32));
		}

		void* _mutex{};
		void* _mapping{};
		State* _state{};
		bool _valid{};
	};

	RuntimeDatabase::~RuntimeDatabase() = default;

	bool RuntimeDatabase::contains(const std::filesystem::path& a_path)
	{
		std::ifstream file(a_path, std::ios::binary | std::ios::ate);
		if (!file) {
			return false;
		}
		const auto end = file.tellg();
		if (end < static_cast<std::streamoff>(MAGIC.size())) {
			return false;
		}
		const auto fileSize = static_cast<std::uint64_t>(end);
		std::array<std::uint8_t, 8> first{};
		file.seekg(0);
		if (!file.read(reinterpret_cast<char*>(first.data()), static_cast<std::streamsize>(first.size()))) {
			return false;
		}
		if (first == MAGIC) {
			return true;
		}
		const auto count = read_le<std::uint64_t>(first, 0);
		constexpr std::uint64_t recordSize = sizeof(std::uint64_t) * 2;
		if (count > (std::numeric_limits<std::uint64_t>::max() - sizeof(std::uint64_t)) / recordSize) {
			return false;
		}
		const auto offset = sizeof(std::uint64_t) + count * recordSize;
		if (offset > fileSize || MAGIC.size() > fileSize - offset ||
			offset > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max())) {
			return false;
		}
		std::array<std::uint8_t, 8> appended{};
		file.seekg(static_cast<std::streamoff>(offset));
		return file.read(reinterpret_cast<char*>(appended.data()),
				   static_cast<std::streamsize>(appended.size())) &&
		       appended == MAGIC;
	}

	std::unique_ptr<RuntimeDatabase> RuntimeDatabase::load(const std::filesystem::path& a_path)
	{
		auto database = std::make_unique<RuntimeDatabase>();
		const auto opened = database->_mappedFile.open(a_path);
		if (!opened) {
			throw std::runtime_error("could not open runtime database");
		}
		const auto fileSize = static_cast<std::uint64_t>(database->_mappedFile.size());
		if (fileSize < MAGIC.size()) {
			throw std::runtime_error("runtime database magic is invalid");
		}
		const auto fileBytes = std::span(
			reinterpret_cast<const std::uint8_t*>(database->_mappedFile.data()),
			database->_mappedFile.size());
		std::array<std::uint8_t, 8> first{};
		std::memcpy(first.data(), fileBytes.data(), first.size());

		std::uint64_t databaseOffset{};
		if (first != MAGIC) {
			const auto legacyCount = read_le<std::uint64_t>(first, 0);
			constexpr std::uint64_t legacyRecordSize = sizeof(std::uint64_t) * 2;
			if (legacyCount > (std::numeric_limits<std::uint64_t>::max() - sizeof(std::uint64_t)) /
								  legacyRecordSize) {
				throw std::runtime_error("runtime database legacy prefix overflows");
			}
			databaseOffset = sizeof(std::uint64_t) + legacyCount * legacyRecordSize;
			if (databaseOffset > fileSize || MAGIC.size() > fileSize - databaseOffset) {
				throw std::runtime_error("runtime database magic is invalid");
			}
			std::array<std::uint8_t, 8> appended{};
			std::memcpy(appended.data(), fileBytes.data() + databaseOffset, appended.size());
			if (appended != MAGIC) {
				throw std::runtime_error("runtime database magic is invalid");
			}
		}

		const auto runtimeSize = fileSize - databaseOffset;
		if (runtimeSize > std::numeric_limits<std::size_t>::max()) {
			throw std::runtime_error("runtime database size is invalid");
		}
		const auto bytes = fileBytes.subspan(
			static_cast<std::size_t>(databaseOffset),
			static_cast<std::size_t>(runtimeSize));
		if (bytes.size() < HEADER_SIZE) {
			throw std::runtime_error("runtime database header is truncated");
		}
		const auto formatMinor = read_le<std::uint16_t>(bytes, 10);
		if (read_le<std::uint16_t>(bytes, 8) != 1 || formatMinor > 5 ||
			read_le<std::uint32_t>(bytes, 12) != HEADER_SIZE ||
			read_le<std::uint32_t>(bytes, 16) != 0 ||
			read_le<std::uint32_t>(bytes, 20) != ENDIAN_MARKER ||
			read_le<std::uint64_t>(bytes, 96) != bytes.size() ||
			read_le<std::uint32_t>(bytes, 108) != 0) {
			throw std::runtime_error("runtime database header is unsupported");
		}
		const auto payloadCRC = read_le<std::uint32_t>(bytes, 104);

		const auto recordCount = read_le<std::uint32_t>(bytes, 24);
		const auto aliasCount = read_le<std::uint32_t>(bytes, 28);
		const auto knownCount = read_le<std::uint32_t>(bytes, 32);
		const auto candidateCount = read_le<std::uint32_t>(bytes, 36);
		const auto fragmentCount = read_le<std::uint32_t>(bytes, 40);
		const auto constraintCount = formatMinor >= 1 ? read_le<std::uint32_t>(bytes, 44) : 0;
		const auto recordsOffset = checked_offset(read_le<std::uint64_t>(bytes, 48), bytes.size());
		const auto aliasesOffset = checked_offset(read_le<std::uint64_t>(bytes, 56), bytes.size());
		const auto knownOffset = checked_offset(read_le<std::uint64_t>(bytes, 64), bytes.size());
		const auto candidatesOffset = checked_offset(read_le<std::uint64_t>(bytes, 72), bytes.size());
		const auto fragmentsOffset = checked_offset(read_le<std::uint64_t>(bytes, 80), bytes.size());
		const auto blobOffset = checked_offset(read_le<std::uint64_t>(bytes, 88), bytes.size());
		const auto constraintsOffset = checked_offset(
			static_cast<std::uint64_t>(fragmentsOffset) +
				static_cast<std::uint64_t>(fragmentCount) * FRAGMENT_SIZE,
			bytes.size());
		if (recordsOffset != HEADER_SIZE) {
			throw std::runtime_error("runtime database record table is misplaced");
		}
		validate_table(recordsOffset, recordCount, RECORD_SIZE, aliasesOffset);
		validate_table(aliasesOffset, aliasCount, ALIAS_SIZE, knownOffset);
		validate_table(knownOffset, knownCount, KNOWN_RVA_SIZE, candidatesOffset);
		const auto encodedCandidateSize = formatMinor >= 2 ? CANDIDATE_SIZE : CANDIDATE_SIZE_V1;
		validate_table(candidatesOffset, candidateCount, encodedCandidateSize, fragmentsOffset);
		validate_table(fragmentsOffset, fragmentCount, FRAGMENT_SIZE, constraintsOffset);
		validate_table(constraintsOffset, constraintCount, CONSTRAINT_SIZE, blobOffset);

		constexpr std::uint32_t mappedRecordThreshold = 100000;
		if (recordCount >= mappedRecordThreshold) {
			database->_payloadCRC = payloadCRC;
			database->_mapped = MappedLayout{
				bytes,
				formatMinor,
				recordCount,
				aliasCount,
				knownCount,
				candidateCount,
				fragmentCount,
				constraintCount,
				recordsOffset,
				aliasesOffset,
				knownOffset,
				candidatesOffset,
				fragmentsOffset,
				constraintsOffset,
				blobOffset,
				encodedCandidateSize
			};
			const auto loadAliases = [&]() {
				database->_aliasIndex.reserve(aliasCount);
				for (std::uint32_t i = 0; i < aliasCount; ++i) {
					const auto offset = aliasesOffset + static_cast<std::size_t>(i) * ALIAS_SIZE;
					const auto recordIndex = read_le<std::uint32_t>(bytes, offset + 16);
					const auto flags = read_le<std::uint32_t>(bytes, offset + 20);
					if (recordIndex >= recordCount ||
						(flags & ~ALIAS_VERSION_MAJOR_MINOR) != 0) {
						throw std::runtime_error("runtime database alias record is invalid");
					}
					database->_aliasIndex.push_back({ read_le<std::uint64_t>(bytes, offset),
						VersionKey{ read_version(bytes, offset + 8) },
						recordIndex,
						read_le<std::uint32_t>(bytes, offset + 20) });
				}
			};
			validate_mapped_database_once(a_path, payloadCRC, bytes.size(), [&]() {
				if (payloadCRC != crc32(bytes.subspan(HEADER_SIZE))) {
					throw std::runtime_error("runtime database payload checksum is invalid");
				}
				std::optional<std::uint64_t> previousID;
				for (std::uint32_t i = 0; i < recordCount; ++i) {
					const auto offset = recordsOffset + static_cast<std::size_t>(i) * RECORD_SIZE;
					const auto id = read_le<std::uint64_t>(bytes, offset);
					const auto firstAlias = read_le<std::uint32_t>(bytes, offset + 8);
					const auto aliases = read_le<std::uint32_t>(bytes, offset + 12);
					const auto firstKnown = read_le<std::uint32_t>(bytes, offset + 16);
					const auto known = read_le<std::uint32_t>(bytes, offset + 20);
					const auto firstCandidate = read_le<std::uint32_t>(bytes, offset + 24);
					const auto candidates = read_le<std::uint32_t>(bytes, offset + 28);
					const auto recordFlags = read_le<std::uint32_t>(bytes, offset + 32);
					if ((previousID && *previousID >= id) || firstAlias > aliasCount ||
						aliases > aliasCount - firstAlias || firstKnown > knownCount ||
						known > knownCount - firstKnown || firstCandidate > candidateCount ||
						candidates > candidateCount - firstCandidate ||
						(recordFlags & ~RECORD_ALL_FLAGS) != 0 ||
						read_le<std::uint32_t>(bytes, offset + 36) != 0) {
						throw std::runtime_error("runtime database record data is invalid");
					}
					for (std::uint32_t aliasIndex = 0; aliasIndex < aliases; ++aliasIndex) {
						const auto aliasOffset = aliasesOffset +
							static_cast<std::size_t>(firstAlias + aliasIndex) * ALIAS_SIZE;
						if (read_le<std::uint32_t>(bytes, aliasOffset + 16) != i) {
							throw std::runtime_error("runtime database alias ownership is invalid");
						}
					}
					for (std::uint32_t knownIndex = 0; knownIndex < known; ++knownIndex) {
						const auto knownEntry = knownOffset +
							static_cast<std::size_t>(firstKnown + knownIndex) * KNOWN_RVA_SIZE;
						if (read_le<std::uint32_t>(bytes, knownEntry + 12) != 0) {
							throw std::runtime_error("runtime database known RVA flags are invalid");
						}
						for (std::uint32_t previous = 0; previous < knownIndex; ++previous) {
							const auto previousEntry = knownOffset +
								static_cast<std::size_t>(firstKnown + previous) * KNOWN_RVA_SIZE;
							if (read_version(bytes, previousEntry) == read_version(bytes, knownEntry)) {
								throw std::runtime_error("runtime database known RVA version is duplicated");
							}
						}
					}
					previousID = id;
				}
				loadAliases();
				for (std::uint32_t i = 0; i < candidateCount; ++i) {
					const auto offset = candidatesOffset + static_cast<std::size_t>(i) * encodedCandidateSize;
					const auto resolution = read_le<std::uint8_t>(bytes, offset);
					const auto section = read_le<std::uint8_t>(bytes, offset + 1);
					const auto firstFragment = read_le<std::uint32_t>(bytes, offset + 4);
					const auto fragments = read_le<std::uint16_t>(bytes, offset + 8);
					const auto resolveFragment = read_le<std::uint16_t>(bytes, offset + 10);
					const auto firstConstraint = formatMinor >= 1 ?
					                                 read_le<std::uint32_t>(bytes, offset + 24) :
					                                 0;
					const auto constraints = formatMinor >= 1 ?
					                             read_le<std::uint16_t>(bytes, offset + 28) :
					                             0;
					const auto idOnlyDependency = is_id_only_dependency(resolution);
					const auto scopedRipDependency = is_scoped_rip_dependency(resolution);
					const auto slotPatternDependency = is_slot_pattern_dependency(resolution);
					const auto stringFunctionResolution =
						resolution == RESOLUTION_FUNCTION_FROM_RIP_RELATIVE_STRING;
					const auto dependent = is_dependent_resolution(resolution);
					const auto vtableDependency = resolution == RESOLUTION_RDATA_VTABLE_FROM_IDS;
					const auto resolveOffset = read_le<std::uint32_t>(bytes, offset + 12);
					const auto resultAdjustment = read_le<std::int32_t>(bytes, offset + 16);
					const auto minimumFixed = read_le<std::uint32_t>(bytes, offset + 20);
					const auto unavailable = is_unavailable_resolution(resolution);
					if (section < 1 || section > 4 || resolution < 1 ||
						resolution > RESOLUTION_FUNCTION_FROM_RIP_RELATIVE_STRING ||
						(formatMinor >= 1 &&
							read_le<std::uint16_t>(bytes, offset + 30) != 0) ||
						(!dependent && !unavailable && fragments == 0) ||
						(unavailable && (fragments != 0 || constraints != 0 || resolveFragment != 0 ||
							resolveOffset != 0 || resultAdjustment != 0 || minimumFixed != 0)) ||
						(idOnlyDependency && (fragments != 0 || constraints == 0 || resolveFragment != 0 ||
							resolveOffset != 0 || minimumFixed != 0 ||
							(vtableDependency && constraints < 2) ||
							(resolution == RESOLUTION_MSVC_TYPE_DESCRIPTOR_FROM_VTABLE_ID &&
								constraints != 1))) ||
						(scopedRipDependency && (fragments != 1 || constraints != 1 ||
							resolveFragment != 0 || minimumFixed == 0 || section != 1 ||
							resultAdjustment < 0)) ||
						(slotPatternDependency && (fragments < 2 || constraints < 2 ||
							resolveFragment != 0 || resolveOffset != 0 || minimumFixed == 0 ||
							section != 2 || resultAdjustment < 0)) ||
						(stringFunctionResolution && (section != 2 || fragments != 1 ||
							constraints != 0 || resolveFragment != 0 || resolveOffset != 0 ||
							resultAdjustment != 0 || minimumFixed < sizeof(std::uint64_t))) ||
						firstFragment > fragmentCount ||
						fragments > fragmentCount - firstFragment ||
						(!dependent && !unavailable && resolveFragment >= fragments) ||
						firstConstraint > constraintCount ||
						static_cast<std::uint32_t>(constraints) > constraintCount - firstConstraint) {
						throw std::runtime_error("runtime database candidate data is invalid");
					}
				}
				for (std::uint32_t i = 0; i < fragmentCount; ++i) {
					const auto offset = fragmentsOffset + static_cast<std::size_t>(i) * FRAGMENT_SIZE;
					const auto dataOffset = read_le<std::uint32_t>(bytes, offset);
					const auto length = read_le<std::uint32_t>(bytes, offset + 4);
					const auto minGap = read_le<std::uint32_t>(bytes, offset + 8);
					const auto maxGap = read_le<std::uint32_t>(bytes, offset + 12);
					const auto dataSize = formatMinor >= 5 ?
						read_le<std::uint32_t>(bytes, offset + 20) :
						(static_cast<std::uint64_t>(length) * 2 <= UINT32_MAX ? length * 2 : 0);
					const auto flags = read_le<std::uint32_t>(bytes, offset + 16);
					if (length == 0 || dataSize == 0 || minGap > maxGap ||
						(flags & ~FRAGMENT_REVERSE) != 0 ||
						dataOffset > bytes.size() - blobOffset ||
						dataSize > bytes.size() - blobOffset - dataOffset) {
						throw std::runtime_error("runtime database fragment data is invalid");
					}
					if (formatMinor >= 5) {
						static_cast<void>(packed_pattern_layout(
							bytes, blobOffset + dataOffset, dataSize, length));
					}
				}
				for (std::uint32_t i = 0; i < constraintCount; ++i) {
					const auto offset = constraintsOffset + static_cast<std::size_t>(i) * CONSTRAINT_SIZE;
					const auto kind = read_le<std::uint8_t>(bytes, offset + 20);
					const auto targetID = read_le<std::uint64_t>(bytes, offset);
					if ((kind == CONSTRAINT_POINTED_PATTERN && targetID != 0) ||
						(kind != CONSTRAINT_POINTED_PATTERN && targetID == 0) ||
						read_le<std::uint16_t>(bytes, offset + 18) != 0 ||
						read_le<std::uint8_t>(bytes, offset + 21) != 0 ||
						read_le<std::uint16_t>(bytes, offset + 22) != 0 ||
						(kind != CONSTRAINT_RELATIVE32_TARGET &&
							kind != CONSTRAINT_RESOLVED_ID &&
							kind != CONSTRAINT_POINTED_PATTERN)) {
						throw std::runtime_error("runtime database constraint data is invalid");
					}
				}
			});
			if (aliasCount != 0 && database->_aliasIndex.empty()) {
				loadAliases();
			}
			std::sort(database->_aliasIndex.begin(), database->_aliasIndex.end(),
				[](const AliasIndexEntry& a_left, const AliasIndexEntry& a_right) {
					return a_left.id < a_right.id ||
				           (a_left.id == a_right.id && a_left.version.parts < a_right.version.parts) ||
				           (a_left.id == a_right.id && a_left.version == a_right.version &&
							   a_left.flags < a_right.flags) ||
					       (a_left.id == a_right.id && a_left.version == a_right.version &&
							   a_left.flags == a_right.flags &&
							   a_left.recordIndex < a_right.recordIndex);
				});
			validate_alias_scopes(database->_aliasIndex);
			database->_aliasIndex.erase(std::unique(
											database->_aliasIndex.begin(), database->_aliasIndex.end(),
											[](const AliasIndexEntry& a_left, const AliasIndexEntry& a_right) {
												return a_left.id == a_right.id && a_left.version == a_right.version &&
				                                       a_left.recordIndex == a_right.recordIndex &&
				                                       a_left.flags == a_right.flags;
											}),
				database->_aliasIndex.end());
			return database;
		}
		if (payloadCRC != crc32(bytes.subspan(HEADER_SIZE))) {
			throw std::runtime_error("runtime database payload checksum is invalid");
		}

		struct FlatRecord
		{
			std::uint64_t id{};
			std::uint32_t firstAlias{};
			std::uint32_t aliasCount{};
			std::uint32_t firstKnown{};
			std::uint32_t knownCount{};
			std::uint32_t firstCandidate{};
			std::uint32_t candidateCount{};
			std::uint32_t flags{};
		};
		struct FlatAlias
		{
			Alias alias;
			std::uint32_t recordIndex{};
		};
		struct FlatCandidate
		{
			Candidate candidate;
			std::uint32_t firstFragment{};
			std::uint16_t fragmentCount{};
			std::uint32_t firstConstraint{};
			std::uint16_t constraintCount{};
		};

		std::vector<FlatRecord> flatRecords;
		flatRecords.reserve(recordCount);
		for (std::uint32_t i = 0; i < recordCount; ++i) {
			const auto offset = recordsOffset + static_cast<std::size_t>(i) * RECORD_SIZE;
			FlatRecord record{
				read_le<std::uint64_t>(bytes, offset),
				read_le<std::uint32_t>(bytes, offset + 8),
				read_le<std::uint32_t>(bytes, offset + 12),
				read_le<std::uint32_t>(bytes, offset + 16),
				read_le<std::uint32_t>(bytes, offset + 20),
				read_le<std::uint32_t>(bytes, offset + 24),
				read_le<std::uint32_t>(bytes, offset + 28),
				read_le<std::uint32_t>(bytes, offset + 32)
			};
			if (record.firstAlias > aliasCount || record.aliasCount > aliasCount - record.firstAlias ||
				record.firstKnown > knownCount || record.knownCount > knownCount - record.firstKnown ||
				record.firstCandidate > candidateCount ||
				record.candidateCount > candidateCount - record.firstCandidate ||
				(record.flags & ~RECORD_ALL_FLAGS) != 0 ||
				read_le<std::uint32_t>(bytes, offset + 36) != 0) {
				throw std::runtime_error("runtime database record range is invalid");
			}
			flatRecords.push_back(record);
		}

		std::vector<FlatAlias> aliases;
		aliases.reserve(aliasCount);
		for (std::uint32_t i = 0; i < aliasCount; ++i) {
			const auto offset = aliasesOffset + static_cast<std::size_t>(i) * ALIAS_SIZE;
			FlatAlias alias;
			alias.alias.id = read_le<std::uint64_t>(bytes, offset);
			alias.alias.version.parts = read_version(bytes, offset + 8);
			alias.recordIndex = read_le<std::uint32_t>(bytes, offset + 16);
			alias.alias.flags = read_le<std::uint32_t>(bytes, offset + 20);
			if (alias.recordIndex >= recordCount ||
				(alias.alias.flags & ~ALIAS_VERSION_MAJOR_MINOR) != 0) {
				throw std::runtime_error("runtime database alias record is invalid");
			}
			aliases.push_back(alias);
		}

		std::vector<KnownRVA> knownRVAs;
		knownRVAs.reserve(knownCount);
		for (std::uint32_t i = 0; i < knownCount; ++i) {
			const auto offset = knownOffset + static_cast<std::size_t>(i) * KNOWN_RVA_SIZE;
			KnownRVA known;
			known.version.parts = read_version(bytes, offset);
			known.rva = read_le<std::uint32_t>(bytes, offset + 8);
			if (read_le<std::uint32_t>(bytes, offset + 12) != 0) {
				throw std::runtime_error("runtime database known RVA flags are invalid");
			}
			knownRVAs.push_back(known);
		}

		std::vector<Fragment> fragments;
		fragments.reserve(fragmentCount);
		for (std::uint32_t i = 0; i < fragmentCount; ++i) {
			const auto offset = fragmentsOffset + static_cast<std::size_t>(i) * FRAGMENT_SIZE;
			const auto dataOffset = read_le<std::uint32_t>(bytes, offset);
			const auto length = read_le<std::uint32_t>(bytes, offset + 4);
			Fragment fragment;
			fragment.minGap = read_le<std::uint32_t>(bytes, offset + 8);
			fragment.maxGap = read_le<std::uint32_t>(bytes, offset + 12);
			fragment.flags = read_le<std::uint32_t>(bytes, offset + 16);
			const auto dataSize = formatMinor >= 5 ?
				read_le<std::uint32_t>(bytes, offset + 20) :
				(static_cast<std::uint64_t>(length) * 2 <= UINT32_MAX ? length * 2 : 0);
			if (length == 0 || dataSize == 0 || fragment.minGap > fragment.maxGap ||
				(fragment.flags & ~FRAGMENT_REVERSE) != 0 ||
				dataOffset > bytes.size() - blobOffset ||
				dataSize > bytes.size() - blobOffset - dataOffset) {
				throw std::runtime_error("runtime database fragment data is invalid");
			}
			const auto data = blobOffset + dataOffset;
			if (formatMinor >= 5) {
				auto decoded = decode_packed_pattern(bytes, data, dataSize, length);
				fragment.pattern.values = std::move(decoded.values);
				fragment.pattern.masks = std::move(decoded.masks);
			} else {
				fragment.pattern.values.assign(bytes.begin() + static_cast<std::ptrdiff_t>(data),
					bytes.begin() + static_cast<std::ptrdiff_t>(data + length));
				fragment.pattern.masks.assign(bytes.begin() + static_cast<std::ptrdiff_t>(data + length),
					bytes.begin() + static_cast<std::ptrdiff_t>(data + length * 2));
				for (std::size_t byte = 0; byte < length; ++byte) {
					fragment.pattern.values[byte] &= fragment.pattern.masks[byte];
				}
			}
			fragments.push_back(std::move(fragment));
		}

		std::vector<Constraint> constraints;
		constraints.reserve(constraintCount);
		for (std::uint32_t i = 0; i < constraintCount; ++i) {
			const auto offset = constraintsOffset + static_cast<std::size_t>(i) * CONSTRAINT_SIZE;
			Constraint constraint;
			constraint.targetID = read_le<std::uint64_t>(bytes, offset);
			constraint.operandOffset = read_le<std::uint32_t>(bytes, offset + 8);
			constraint.targetAdjustment = read_le<std::int32_t>(bytes, offset + 12);
			constraint.fragmentIndex = read_le<std::uint16_t>(bytes, offset + 16);
			constraint.flags = read_le<std::uint16_t>(bytes, offset + 18);
			constraint.kind = read_le<std::uint8_t>(bytes, offset + 20);
			if ((constraint.kind == CONSTRAINT_POINTED_PATTERN && constraint.targetID != 0) ||
				(constraint.kind != CONSTRAINT_POINTED_PATTERN && constraint.targetID == 0) ||
				constraint.flags != 0 ||
				read_le<std::uint8_t>(bytes, offset + 21) != 0 ||
				read_le<std::uint16_t>(bytes, offset + 22) != 0 ||
				(constraint.kind != CONSTRAINT_RELATIVE32_TARGET &&
					constraint.kind != CONSTRAINT_RESOLVED_ID &&
					constraint.kind != CONSTRAINT_POINTED_PATTERN)) {
				throw std::runtime_error("runtime database constraint data is invalid");
			}
			constraints.push_back(constraint);
		}

		std::vector<FlatCandidate> candidates;
		candidates.reserve(candidateCount);
		for (std::uint32_t i = 0; i < candidateCount; ++i) {
			const auto offset = candidatesOffset + static_cast<std::size_t>(i) * encodedCandidateSize;
			FlatCandidate candidate;
			candidate.candidate.resolution = read_le<std::uint8_t>(bytes, offset);
			candidate.candidate.section = read_le<std::uint8_t>(bytes, offset + 1);
			candidate.candidate.flags = read_le<std::uint16_t>(bytes, offset + 2);
			candidate.firstFragment = read_le<std::uint32_t>(bytes, offset + 4);
			candidate.fragmentCount = read_le<std::uint16_t>(bytes, offset + 8);
			candidate.candidate.resolveFragment = read_le<std::uint16_t>(bytes, offset + 10);
			candidate.candidate.resolveOffset = read_le<std::uint32_t>(bytes, offset + 12);
			candidate.candidate.resultAdjustment = read_le<std::int32_t>(bytes, offset + 16);
			candidate.candidate.minFixedBytes = read_le<std::uint32_t>(bytes, offset + 20);
			candidate.firstConstraint = formatMinor >= 1 ? read_le<std::uint32_t>(bytes, offset + 24) : 0;
			candidate.constraintCount = formatMinor >= 1 ? read_le<std::uint16_t>(bytes, offset + 28) : 0;
			if (formatMinor >= 1 && read_le<std::uint16_t>(bytes, offset + 30) != 0) {
				throw std::runtime_error("runtime database candidate reserved data is invalid");
			}
			if (formatMinor >= 2) {
				candidate.candidate.version.parts = read_version(bytes, offset + 32);
			}
			const auto idOnlyDependency = is_id_only_dependency(candidate.candidate.resolution);
			const auto scopedRipDependency =
				is_scoped_rip_dependency(candidate.candidate.resolution);
			const auto slotPatternDependency =
				is_slot_pattern_dependency(candidate.candidate.resolution);
			const auto stringFunctionResolution =
				candidate.candidate.resolution == RESOLUTION_FUNCTION_FROM_RIP_RELATIVE_STRING;
			const auto dependent = is_dependent_resolution(candidate.candidate.resolution);
			const auto unavailable = is_unavailable_resolution(candidate.candidate.resolution);
			const auto vtableDependency =
				candidate.candidate.resolution == RESOLUTION_RDATA_VTABLE_FROM_IDS;
			if (candidate.candidate.section < 1 || candidate.candidate.section > 4 ||
				candidate.candidate.resolution < 1 ||
				candidate.candidate.resolution > RESOLUTION_FUNCTION_FROM_RIP_RELATIVE_STRING ||
				(!dependent && !unavailable && candidate.fragmentCount == 0) ||
				(unavailable && (candidate.fragmentCount != 0 || candidate.constraintCount != 0 ||
					candidate.candidate.resolveFragment != 0 ||
					candidate.candidate.resolveOffset != 0 ||
					candidate.candidate.resultAdjustment != 0 ||
					candidate.candidate.minFixedBytes != 0)) ||
				(idOnlyDependency && (candidate.fragmentCount != 0 || candidate.constraintCount == 0 ||
					candidate.candidate.resolveFragment != 0 || candidate.candidate.resolveOffset != 0 ||
					candidate.candidate.minFixedBytes != 0 ||
					(vtableDependency && candidate.constraintCount < 2) ||
					(candidate.candidate.resolution ==
						RESOLUTION_MSVC_TYPE_DESCRIPTOR_FROM_VTABLE_ID &&
						candidate.constraintCount != 1))) ||
				(scopedRipDependency && (candidate.fragmentCount != 1 ||
					candidate.constraintCount != 1 || candidate.candidate.resolveFragment != 0 ||
					candidate.candidate.minFixedBytes == 0 || candidate.candidate.section != 1 ||
					candidate.candidate.resultAdjustment < 0)) ||
				(slotPatternDependency && (candidate.fragmentCount < 2 ||
					candidate.constraintCount < 2 || candidate.candidate.resolveFragment != 0 ||
					candidate.candidate.resolveOffset != 0 ||
					candidate.candidate.minFixedBytes == 0 || candidate.candidate.section != 2 ||
					candidate.candidate.resultAdjustment < 0)) ||
				(stringFunctionResolution && (candidate.candidate.section != 2 ||
					candidate.fragmentCount != 1 || candidate.constraintCount != 0 ||
					candidate.candidate.resolveFragment != 0 ||
					candidate.candidate.resolveOffset != 0 ||
					candidate.candidate.resultAdjustment != 0 ||
					candidate.candidate.minFixedBytes < sizeof(std::uint64_t))) ||
				candidate.firstFragment > fragmentCount ||
				candidate.fragmentCount > fragmentCount - candidate.firstFragment ||
				candidate.firstConstraint > constraintCount ||
				candidate.constraintCount > constraintCount - candidate.firstConstraint ||
				(!dependent && !unavailable &&
					candidate.candidate.resolveFragment >= candidate.fragmentCount)) {
				throw std::runtime_error("runtime database candidate data is invalid");
			}
			candidates.push_back(std::move(candidate));
		}

		database->_payloadCRC = payloadCRC;
		database->_records.reserve(recordCount);
		for (std::uint32_t recordIndex = 0; recordIndex < recordCount; ++recordIndex) {
			const auto& flat = flatRecords[recordIndex];
			Record record;
			record.id = flat.id;
			record.flags = flat.flags;
			for (std::uint32_t i = 0; i < flat.aliasCount; ++i) {
				const auto& alias = aliases[flat.firstAlias + i];
				if (alias.recordIndex != recordIndex) {
					throw std::runtime_error("runtime database alias ownership is invalid");
				}
				record.aliases.push_back(alias.alias);
			}
			record.knownRVAs.insert(record.knownRVAs.end(),
				knownRVAs.begin() + flat.firstKnown,
				knownRVAs.begin() + flat.firstKnown + flat.knownCount);
			for (std::size_t left = 0; left < record.knownRVAs.size(); ++left) {
				for (std::size_t right = left + 1; right < record.knownRVAs.size(); ++right) {
					if (record.knownRVAs[left].version == record.knownRVAs[right].version) {
						throw std::runtime_error(
							"runtime database known RVA version is duplicated");
					}
				}
			}
			for (std::uint32_t i = 0; i < flat.candidateCount; ++i) {
				auto candidate = candidates[flat.firstCandidate + i].candidate;
				const auto& flatCandidate = candidates[flat.firstCandidate + i];
				candidate.fragments.insert(candidate.fragments.end(),
					fragments.begin() + flatCandidate.firstFragment,
					fragments.begin() + flatCandidate.firstFragment + flatCandidate.fragmentCount);
				candidate.constraints.insert(candidate.constraints.end(),
					constraints.begin() + flatCandidate.firstConstraint,
					constraints.begin() + flatCandidate.firstConstraint + flatCandidate.constraintCount);
				const auto dependent = is_dependent_resolution(candidate.resolution);
				const auto vtableDependency =
					candidate.resolution == RESOLUTION_RDATA_VTABLE_FROM_IDS;
				if (std::any_of(candidate.constraints.begin(), candidate.constraints.end(),
						[&](const Constraint& a_constraint) {
							return (a_constraint.kind == CONSTRAINT_RELATIVE32_TARGET &&
									a_constraint.fragmentIndex >= candidate.fragments.size()) ||
							       (a_constraint.kind == CONSTRAINT_RESOLVED_ID &&
									   (!dependent ||
										   candidate.resolution ==
											   RESOLUTION_RDATA_VTABLE_FROM_SLOT_PATTERNS ||
										   (!vtableDependency && a_constraint.operandOffset != 0) ||
										   (vtableDependency &&
											   a_constraint.operandOffset % sizeof(std::uint64_t) != 0) ||
										   a_constraint.fragmentIndex != 0 || a_constraint.flags != 0)) ||
							       (a_constraint.kind == CONSTRAINT_POINTED_PATTERN &&
									   (candidate.resolution !=
											   RESOLUTION_RDATA_VTABLE_FROM_SLOT_PATTERNS ||
										   a_constraint.targetID != 0 ||
										   a_constraint.operandOffset % sizeof(std::uint64_t) != 0 ||
										   a_constraint.targetAdjustment != 0 ||
										   a_constraint.fragmentIndex >= candidate.fragments.size() ||
										   a_constraint.flags != 0)) ||
							       (a_constraint.kind != CONSTRAINT_RELATIVE32_TARGET &&
									   a_constraint.kind != CONSTRAINT_RESOLVED_ID &&
									   a_constraint.kind != CONSTRAINT_POINTED_PATTERN);
						})) {
					throw std::runtime_error("runtime database constraint fragment is invalid");
				}
				std::size_t fixedBytes{};
				for (const auto& fragment : candidate.fragments) {
					fixedBytes += static_cast<std::size_t>(std::count_if(
						fragment.pattern.masks.begin(), fragment.pattern.masks.end(),
						[](std::uint8_t mask) { return mask != 0; }));
				}
				if (fixedBytes < candidate.minFixedBytes) {
					throw std::runtime_error("runtime database candidate fixed-byte count is invalid");
				}
				record.candidates.push_back(std::move(candidate));
			}
			database->_records.push_back(std::move(record));
		}
		if (!std::is_sorted(database->_records.begin(), database->_records.end(),
				[](const Record& a_left, const Record& a_right) { return a_left.id < a_right.id; }) ||
			std::adjacent_find(database->_records.begin(), database->_records.end(),
				[](const Record& a_left, const Record& a_right) { return a_left.id == a_right.id; }) !=
				database->_records.end()) {
			throw std::runtime_error("runtime database IDs are not strictly sorted");
		}
		database->_aliasIndex.reserve(aliasCount);
		for (std::size_t recordIndex = 0; recordIndex < database->_records.size(); ++recordIndex) {
			for (const auto& alias : database->_records[recordIndex].aliases) {
				database->_aliasIndex.push_back({ alias.id, alias.version, recordIndex, alias.flags });
			}
		}
		std::sort(database->_aliasIndex.begin(), database->_aliasIndex.end(),
			[](const AliasIndexEntry& a_left, const AliasIndexEntry& a_right) {
				return a_left.id < a_right.id ||
			           (a_left.id == a_right.id && a_left.version.parts < a_right.version.parts) ||
			           (a_left.id == a_right.id && a_left.version == a_right.version &&
						   a_left.flags < a_right.flags) ||
				       (a_left.id == a_right.id && a_left.version == a_right.version &&
						   a_left.flags == a_right.flags &&
						   a_left.recordIndex < a_right.recordIndex);
			});
		validate_alias_scopes(database->_aliasIndex);
		database->_aliasIndex.erase(std::unique(database->_aliasIndex.begin(), database->_aliasIndex.end(),
										[](const AliasIndexEntry& a_left, const AliasIndexEntry& a_right) {
											return a_left.id == a_right.id && a_left.version == a_right.version &&
			                                       a_left.recordIndex == a_right.recordIndex &&
			                                       a_left.flags == a_right.flags;
										}),
			database->_aliasIndex.end());
		return database;
	}

	std::size_t RuntimeDatabase::record_count() const noexcept
	{
		return _mapped ? _mapped->recordCount : _records.size();
	}

	bool RuntimeDatabase::contains_id(std::uint64_t a_id, const Version& a_version) const
	{
		const std::shared_lock stateLock(_batchStateLock);
		VersionKey version;
		for (std::size_t i = 0; i < version.parts.size(); ++i) {
			version.parts[i] = a_version[i];
		}
		return find(a_id, version) != nullptr;
	}

	const RuntimeDatabase::Record* RuntimeDatabase::find(
		std::uint64_t a_id,
		const VersionKey& a_version) const
	{
		// OG and AE use independent numeric ID spaces. A versioned alias must
		// therefore win over an equal canonical ID from another runtime.
		const auto alias = std::lower_bound(_aliasIndex.begin(), _aliasIndex.end(), a_id,
			[](const AliasIndexEntry& a_entry, std::uint64_t a_value) { return a_entry.id < a_value; });
		const auto resolvedAlias = [&](bool a_majorMinor) -> const Record* {
			for (auto found = alias; found != _aliasIndex.end() && found->id == a_id; ++found) {
				const auto scoped = (found->flags & ALIAS_VERSION_MAJOR_MINOR) != 0;
				if (scoped == a_majorMinor &&
					version_scope_matches(found->version.parts, a_version.parts, scoped)) {
					return _mapped ? mapped_record(found->recordIndex) :
					                 std::addressof(_records[found->recordIndex]);
				}
			}
			return nullptr;
		};
		if (const auto exact = resolvedAlias(false)) {
			return exact;
		}
		if (const auto scoped = resolvedAlias(true)) {
			return scoped;
		}
		if (_mapped) {
			std::size_t low{};
			std::size_t high = _mapped->recordCount;
			while (low < high) {
				const auto middle = low + (high - low) / 2;
				const auto id = read_le<std::uint64_t>(
					_mapped->bytes,
					_mapped->recordsOffset + middle * RECORD_SIZE);
				if (id < a_id) {
					low = middle + 1;
				} else {
					high = middle;
				}
			}
			if (low < _mapped->recordCount && read_le<std::uint64_t>(
												  _mapped->bytes,
												  _mapped->recordsOffset + low * RECORD_SIZE) == a_id) {
				return mapped_record(low);
			}
			return nullptr;
		}
		const auto canonical = std::lower_bound(_records.begin(), _records.end(), a_id,
			[](const Record& a_record, std::uint64_t a_value) { return a_record.id < a_value; });
		return canonical != _records.end() && canonical->id == a_id ? std::addressof(*canonical) : nullptr;
	}

	const RuntimeDatabase::Record* RuntimeDatabase::mapped_record(std::size_t a_recordIndex) const
	{
		if (!_mapped || a_recordIndex >= _mapped->recordCount) {
			return nullptr;
		}
		{
			const std::scoped_lock lock(_recordCacheLock);
			if (const auto found = _recordCache.find(a_recordIndex); found != _recordCache.end()) {
				return found->second.get();
			}
		}
		const auto& layout = *_mapped;
		const auto& bytes = layout.bytes;
		const auto recordOffset = layout.recordsOffset + a_recordIndex * RECORD_SIZE;
		auto record = std::make_unique<Record>();
		record->id = read_le<std::uint64_t>(bytes, recordOffset);
		record->flags = read_le<std::uint32_t>(bytes, recordOffset + 32);
		const auto firstAlias = read_le<std::uint32_t>(bytes, recordOffset + 8);
		const auto aliasCount = read_le<std::uint32_t>(bytes, recordOffset + 12);
		const auto firstKnown = read_le<std::uint32_t>(bytes, recordOffset + 16);
		const auto knownCount = read_le<std::uint32_t>(bytes, recordOffset + 20);
		const auto firstCandidate = read_le<std::uint32_t>(bytes, recordOffset + 24);
		const auto candidateCount = read_le<std::uint32_t>(bytes, recordOffset + 28);

		record->aliases.reserve(aliasCount);
		for (std::uint32_t i = 0; i < aliasCount; ++i) {
			const auto offset = layout.aliasesOffset +
			                    static_cast<std::size_t>(firstAlias + i) * ALIAS_SIZE;
			if (read_le<std::uint32_t>(bytes, offset + 16) != a_recordIndex) {
				throw std::runtime_error("runtime database alias ownership is invalid");
			}
			record->aliases.push_back({ read_le<std::uint64_t>(bytes, offset),
				VersionKey{ read_version(bytes, offset + 8) },
				read_le<std::uint32_t>(bytes, offset + 20) });
		}
		record->knownRVAs.reserve(knownCount);
		for (std::uint32_t i = 0; i < knownCount; ++i) {
			const auto offset = layout.knownOffset +
			                    static_cast<std::size_t>(firstKnown + i) * KNOWN_RVA_SIZE;
			record->knownRVAs.push_back({ VersionKey{ read_version(bytes, offset) },
				read_le<std::uint32_t>(bytes, offset + 8) });
		}
		record->candidates.reserve(candidateCount);
		for (std::uint32_t i = 0; i < candidateCount; ++i) {
			const auto offset = layout.candidatesOffset +
			                    static_cast<std::size_t>(firstCandidate + i) * layout.candidateSize;
			Candidate candidate;
			candidate.resolution = read_le<std::uint8_t>(bytes, offset);
			candidate.section = read_le<std::uint8_t>(bytes, offset + 1);
			candidate.flags = read_le<std::uint16_t>(bytes, offset + 2);
			const auto firstFragment = read_le<std::uint32_t>(bytes, offset + 4);
			const auto fragmentCount = read_le<std::uint16_t>(bytes, offset + 8);
			candidate.resolveFragment = read_le<std::uint16_t>(bytes, offset + 10);
			candidate.resolveOffset = read_le<std::uint32_t>(bytes, offset + 12);
			candidate.resultAdjustment = read_le<std::int32_t>(bytes, offset + 16);
			candidate.minFixedBytes = read_le<std::uint32_t>(bytes, offset + 20);
			const auto firstConstraint = layout.formatMinor >= 1 ?
			                                 read_le<std::uint32_t>(bytes, offset + 24) :
			                                 0;
			const auto constraintCount = layout.formatMinor >= 1 ?
			                                 read_le<std::uint16_t>(bytes, offset + 28) :
			                                 0;
			if (layout.formatMinor >= 2) {
				candidate.version.parts = read_version(bytes, offset + 32);
			}
			candidate.fragments.reserve(fragmentCount);
			for (std::uint32_t fragmentIndex = 0; fragmentIndex < fragmentCount; ++fragmentIndex) {
				const auto fragmentOffset = layout.fragmentsOffset +
				                            static_cast<std::size_t>(firstFragment + fragmentIndex) * FRAGMENT_SIZE;
				const auto dataOffset = read_le<std::uint32_t>(bytes, fragmentOffset);
				const auto length = read_le<std::uint32_t>(bytes, fragmentOffset + 4);
				Fragment fragment;
				fragment.minGap = read_le<std::uint32_t>(bytes, fragmentOffset + 8);
				fragment.maxGap = read_le<std::uint32_t>(bytes, fragmentOffset + 12);
				fragment.flags = read_le<std::uint32_t>(bytes, fragmentOffset + 16);
				const auto data = layout.blobOffset + dataOffset;
				if (layout.formatMinor >= 5) {
					const auto dataSize = read_le<std::uint32_t>(bytes, fragmentOffset + 20);
					auto decoded = decode_packed_pattern(bytes, data, dataSize, length);
					fragment.pattern.values = std::move(decoded.values);
					fragment.pattern.masks = std::move(decoded.masks);
				} else {
					fragment.pattern.values.assign(
						bytes.begin() + static_cast<std::ptrdiff_t>(data),
						bytes.begin() + static_cast<std::ptrdiff_t>(data + length));
					fragment.pattern.masks.assign(
						bytes.begin() + static_cast<std::ptrdiff_t>(data + length),
						bytes.begin() + static_cast<std::ptrdiff_t>(data + length * 2));
					for (std::size_t byte = 0; byte < length; ++byte) {
						fragment.pattern.values[byte] &= fragment.pattern.masks[byte];
					}
				}
				candidate.fragments.push_back(std::move(fragment));
			}
			candidate.constraints.reserve(constraintCount);
			for (std::uint32_t constraintIndex = 0;
				 constraintIndex < static_cast<std::uint32_t>(constraintCount); ++constraintIndex) {
				const auto constraintOffset = layout.constraintsOffset +
				                              static_cast<std::size_t>(firstConstraint + constraintIndex) * CONSTRAINT_SIZE;
				Constraint constraint;
				constraint.targetID = read_le<std::uint64_t>(bytes, constraintOffset);
				constraint.operandOffset = read_le<std::uint32_t>(bytes, constraintOffset + 8);
				constraint.targetAdjustment = read_le<std::int32_t>(bytes, constraintOffset + 12);
				constraint.fragmentIndex = read_le<std::uint16_t>(bytes, constraintOffset + 16);
				constraint.flags = read_le<std::uint16_t>(bytes, constraintOffset + 18);
				constraint.kind = read_le<std::uint8_t>(bytes, constraintOffset + 20);
				const auto dependent = is_dependent_resolution(candidate.resolution);
				const auto vtableDependency =
					candidate.resolution == RESOLUTION_RDATA_VTABLE_FROM_IDS;
				if ((constraint.kind == CONSTRAINT_RELATIVE32_TARGET &&
						constraint.fragmentIndex >= candidate.fragments.size()) ||
					(constraint.kind == CONSTRAINT_RESOLVED_ID &&
						(!dependent ||
							candidate.resolution == RESOLUTION_RDATA_VTABLE_FROM_SLOT_PATTERNS ||
							(!vtableDependency && constraint.operandOffset != 0) ||
							(vtableDependency &&
								constraint.operandOffset % sizeof(std::uint64_t) != 0) ||
							constraint.fragmentIndex != 0 || constraint.flags != 0)) ||
					(constraint.kind == CONSTRAINT_POINTED_PATTERN &&
						(candidate.resolution != RESOLUTION_RDATA_VTABLE_FROM_SLOT_PATTERNS ||
							constraint.targetID != 0 ||
							constraint.operandOffset % sizeof(std::uint64_t) != 0 ||
							constraint.targetAdjustment != 0 ||
							constraint.fragmentIndex >= candidate.fragments.size() ||
							constraint.flags != 0)) ||
					(constraint.kind != CONSTRAINT_RELATIVE32_TARGET &&
						constraint.kind != CONSTRAINT_RESOLVED_ID &&
						constraint.kind != CONSTRAINT_POINTED_PATTERN)) {
					throw std::runtime_error("runtime database constraint fragment is invalid");
				}
				candidate.constraints.push_back(constraint);
			}
			const auto fixedBytes = std::accumulate(
				candidate.fragments.begin(), candidate.fragments.end(), std::size_t{},
				[](std::size_t a_total, const Fragment& a_fragment) {
					return a_total + static_cast<std::size_t>(std::count_if(
										 a_fragment.pattern.masks.begin(), a_fragment.pattern.masks.end(),
										 [](std::uint8_t a_mask) { return a_mask != 0; }));
				});
			if (fixedBytes < candidate.minFixedBytes) {
				throw std::runtime_error("runtime database candidate fixed-byte count is invalid");
			}
			record->candidates.push_back(std::move(candidate));
		}
		const std::scoped_lock lock(_recordCacheLock);
		const auto [inserted, unused] = _recordCache.emplace(a_recordIndex, std::move(record));
		static_cast<void>(unused);
		return inserted->second.get();
	}

	void RuntimeDatabase::prepare_batch(
		const VersionKey& a_version,
		const Module& a_module) const
	{
		clear_batch();
		auto batch = std::make_unique<BatchIndex>();
		const auto expectedPatterns = _mapped ? _mapped->candidateCount : _records.size();
		batch->patterns.reserve(expectedPatterns);
		batch->patternsByHash.reserve(expectedPatterns);

		const auto compatible = [&](const VersionKey& a_candidateVersion, std::uint16_t a_flags) {
			return candidate_scope_matches(
				a_candidateVersion.parts, a_version.parts, a_flags);
		};

		const auto addPattern = [&](std::uint8_t a_section,
								std::span<const std::uint8_t> a_values,
								std::span<const std::uint8_t> a_masks) {
			if (a_section < 1 || a_section > 4 || a_values.empty() ||
				a_values.size() != a_masks.size()) {
				return;
			}
			auto hash = pattern_hash(a_section, a_values, a_masks);
			for (;;) {
				const auto found = batch->patternsByHash.find(hash);
				if (found == batch->patternsByHash.end()) {
					break;
				}
				const auto& existing = batch->patterns[found->second];
				if (existing.section == a_section &&
					same_pattern(existing.values, existing.masks, a_values, a_masks)) {
					return;
				}
				hash = next_pattern_hash(hash);
			}
			const auto index = batch->patterns.size();
			batch->patterns.push_back({
				a_section,
				a_values,
				a_masks,
				0,
				0,
				0,
				false,
				{}
			});
			batch->patternsByHash.emplace(hash, index);
		};

		if (_mapped) {
			const auto& layout = *_mapped;
			if (layout.formatMinor >= 5) {
				struct PackedPatternRef
				{
					std::uint32_t dataOffset{};
					std::uint32_t dataSize{};
					std::uint32_t length{};
					std::size_t storageOffset{};
				};
				struct PackedCandidateRef
				{
					std::uint32_t patternIndex{};
					std::uint8_t section{};
				};

				std::unordered_map<std::uint32_t, std::uint32_t> patternsByDataOffset;
				patternsByDataOffset.reserve(layout.candidateCount);
				std::vector<PackedPatternRef> decodedPatterns;
				decodedPatterns.reserve(layout.candidateCount);
				std::vector<PackedCandidateRef> packedCandidates;
				packedCandidates.reserve(layout.candidateCount);
				std::size_t decodedBytes{};

				for (std::size_t candidateIndex = 0;
					 candidateIndex < layout.candidateCount;
					 ++candidateIndex) {
					const auto offset = layout.candidatesOffset + candidateIndex * layout.candidateSize;
					const auto resolution = read_le<std::uint8_t>(layout.bytes, offset);
					const auto section = read_le<std::uint8_t>(layout.bytes, offset + 1);
					const auto flags = read_le<std::uint16_t>(layout.bytes, offset + 2);
					const auto fragmentCount = read_le<std::uint16_t>(layout.bytes, offset + 8);
					VersionKey candidateVersion;
					candidateVersion.parts = read_version(layout.bytes, offset + 32);
					if (is_dependent_resolution(resolution) || resolution == 7 ||
						fragmentCount == 0 || !compatible(candidateVersion, flags)) {
						continue;
					}
					const auto firstFragment = read_le<std::uint32_t>(layout.bytes, offset + 4);
					const auto fragmentOffset = layout.fragmentsOffset +
					                            static_cast<std::size_t>(firstFragment) * FRAGMENT_SIZE;
					const auto dataOffset = read_le<std::uint32_t>(layout.bytes, fragmentOffset);
					const auto dataSize = read_le<std::uint32_t>(layout.bytes, fragmentOffset + 20);
					const auto length = read_le<std::uint32_t>(layout.bytes, fragmentOffset + 4);
					auto [found, inserted] = patternsByDataOffset.try_emplace(
						dataOffset, static_cast<std::uint32_t>(decodedPatterns.size()));
					if (inserted) {
						if (length > (std::numeric_limits<std::size_t>::max() - decodedBytes) / 2) {
							throw std::runtime_error("runtime database decoded pattern storage overflows");
						}
						decodedPatterns.push_back({ dataOffset, dataSize, length, decodedBytes });
						decodedBytes += static_cast<std::size_t>(length) * 2;
					} else {
						const auto& existing = decodedPatterns[found->second];
						if (existing.dataSize != dataSize || existing.length != length) {
							throw std::runtime_error("runtime database reuses incompatible packed pattern data");
						}
					}
					packedCandidates.push_back({ found->second, section });
				}

				batch->ownedPatternBytes.resize(decodedBytes);
				const auto workerCount = bulk_worker_count(decodedPatterns.size(), 4096);
				std::vector<std::future<void>> workers;
				workers.reserve(workerCount);
				for (std::size_t worker = 0; worker < workerCount; ++worker) {
					workers.push_back(std::async(std::launch::async, [&, worker] {
						for (auto index = worker; index < decodedPatterns.size(); index += workerCount) {
							const auto& pattern = decodedPatterns[index];
							auto values = std::span(batch->ownedPatternBytes).subspan(
								pattern.storageOffset, pattern.length);
							auto masks = std::span(batch->ownedPatternBytes).subspan(
								pattern.storageOffset + pattern.length, pattern.length);
							decode_packed_pattern_into(
								layout.bytes,
								layout.blobOffset + pattern.dataOffset,
								pattern.dataSize,
								pattern.length,
								values,
								masks);
						}
					}));
				}
				for (auto& worker : workers) {
					worker.get();
				}
				for (const auto& candidate : packedCandidates) {
					const auto& pattern = decodedPatterns[candidate.patternIndex];
					const auto bytes = std::span(batch->ownedPatternBytes);
					addPattern(
						candidate.section,
						bytes.subspan(pattern.storageOffset, pattern.length),
						bytes.subspan(pattern.storageOffset + pattern.length, pattern.length));
				}
			} else {
				for (std::size_t candidateIndex = 0;
					 candidateIndex < layout.candidateCount;
					 ++candidateIndex) {
					const auto offset = layout.candidatesOffset + candidateIndex * layout.candidateSize;
					const auto resolution = read_le<std::uint8_t>(layout.bytes, offset);
					const auto section = read_le<std::uint8_t>(layout.bytes, offset + 1);
					const auto flags = read_le<std::uint16_t>(layout.bytes, offset + 2);
					const auto fragmentCount = read_le<std::uint16_t>(layout.bytes, offset + 8);
					VersionKey candidateVersion;
					if (layout.formatMinor >= 2) {
						candidateVersion.parts = read_version(layout.bytes, offset + 32);
					}
					if (is_dependent_resolution(resolution) || resolution == 7 ||
						fragmentCount == 0 || !compatible(candidateVersion, flags)) {
						continue;
					}
					const auto firstFragment = read_le<std::uint32_t>(layout.bytes, offset + 4);
					const auto fragmentOffset = layout.fragmentsOffset +
					                            static_cast<std::size_t>(firstFragment) * FRAGMENT_SIZE;
					const auto dataOffset = read_le<std::uint32_t>(layout.bytes, fragmentOffset);
					const auto length = read_le<std::uint32_t>(layout.bytes, fragmentOffset + 4);
					const auto data = layout.blobOffset + dataOffset;
					addPattern(
						section,
						layout.bytes.subspan(data, length),
						layout.bytes.subspan(data + length, length));
				}
			}
		} else {
			for (const auto& record : _records) {
				for (const auto& candidate : record.candidates) {
					if (is_dependent_resolution(candidate.resolution) ||
						candidate.resolution == 7 || candidate.fragments.empty() ||
						!compatible(candidate.version, candidate.flags)) {
						continue;
					}
					addPattern(
						candidate.section,
						candidate.fragments.front().pattern.values,
						candidate.fragments.front().pattern.masks);
				}
			}
		}

		std::vector<std::atomic_uint32_t> batchMatchCounts(batch->patterns.size());
		for (auto& count : batchMatchCounts) {
			count.store(0, std::memory_order_relaxed);
		}
		const auto processSection = [&](std::size_t a_sectionIndex) {
			std::vector<AnchorReference> anchors;
			for (std::size_t patternIndex = 0; patternIndex < batch->patterns.size(); ++patternIndex) {
				auto& pattern = batch->patterns[patternIndex];
				if (pattern.section != a_sectionIndex + 1) {
					continue;
				}
				const auto choices = choose_anchors(pattern.values, pattern.masks);
				if (choices.count == 0) {
					continue;
				}
				const ChosenAnchor* best = std::addressof(choices.values.front());
				std::uint32_t bestScore{};
				for (std::size_t choiceIndex = 0; choiceIndex < choices.count; ++choiceIndex) {
					const auto& choice = choices.values[choiceIndex];
					std::uint32_t score{};
					for (std::size_t byte = 0; byte < choice.length; ++byte) {
						const auto value = pattern.values[choice.offset + byte];
						bool first = true;
						for (std::size_t earlier = 0; earlier < byte; ++earlier) {
							if (pattern.values[choice.offset + earlier] == value) {
								first = false;
								break;
							}
						}
						score += first ? 16 : 0;
						score += value != 0x00 && value != 0xFF && value != 0x48 &&
						                 value != 0x4C && value != 0x8B && value != 0x89 &&
						                 value != 0x44 && value != 0x24 && value != 0x83 &&
						                 value != 0x90 && value != 0xCC ?
						             5 :
						             0;
					}
					if (choiceIndex == 0 || score >= bestScore) {
						best = std::addressof(choice);
						bestScore = score;
					}
				}
				pattern.anchor = best->value;
				pattern.anchorOffset = best->offset;
				pattern.anchorLength = best->length;
				pattern.indexed = true;
				anchors.push_back({ best->value, patternIndex, best->offset, best->length });
			}

			std::vector<AnchorReference> anchors8;
			std::vector<AnchorReference> anchors4;
			anchors8.reserve(anchors.size());
			anchors4.reserve(anchors.size());
			for (const auto& anchor : anchors) {
				(anchor.length == 8 ? anchors8 : anchors4).push_back(anchor);
			}
			const auto byValue = [](const AnchorReference& a_left, const AnchorReference& a_right) {
				return a_left.value < a_right.value;
			};
			std::sort(anchors8.begin(), anchors8.end(), byValue);
			std::sort(anchors4.begin(), anchors4.end(), byValue);
			std::unordered_map<std::uint64_t, std::pair<std::size_t, std::size_t>> groups8;
			std::unordered_map<std::uint64_t, std::pair<std::size_t, std::size_t>> groups4;
			const auto groupAnchors = [](const auto& a_anchors, auto& a_groups) {
				a_groups.reserve(a_anchors.size());
				for (std::size_t begin = 0; begin < a_anchors.size();) {
					auto end = begin + 1;
					while (end < a_anchors.size() && a_anchors[end].value == a_anchors[begin].value) {
						++end;
					}
					a_groups.emplace(a_anchors[begin].value, std::pair{ begin, end });
					begin = end;
				}
			};
			groupAnchors(anchors8, groups8);
			groupAnchors(anchors4, groups4);

			Segment scan;
			switch (a_sectionIndex) {
			case 0:
				scan = a_module.segment(Segment::text);
				break;
			case 1:
				scan = a_module.segment(Segment::rdata);
				break;
			case 2:
				scan = a_module.segment(Segment::data);
				break;
			case 3:
				scan = a_module.segment(Segment::idata);
				break;
			default:
				break;
			}
			const auto bytes = std::span(scan.pointer<const std::uint8_t>(), scan.size());
			struct ChunkResult
			{
				std::vector<std::pair<std::size_t, std::size_t>> matches;
			};
			const auto scanChunk = [&](std::size_t a_begin, std::size_t a_end) {
				ChunkResult result;
				const auto verifyAt = [&](const auto& a_anchors,
									  const auto& a_groups,
									  std::size_t a_offset,
									  std::size_t a_length) {
					const auto found = a_groups.find(load_anchor(bytes.data() + a_offset, a_length));
					if (found == a_groups.end()) {
						return;
					}
					for (auto reference = found->second.first; reference < found->second.second; ++reference) {
						const auto& anchor = a_anchors[reference];
						if (a_offset < anchor.patternOffset) {
							continue;
						}
						const auto start = a_offset - anchor.patternOffset;
						const auto& pattern = batch->patterns[anchor.patternIndex];
						if (start > bytes.size() || pattern.values.size() > bytes.size() - start) {
							continue;
						}
						bool matches = true;
						for (std::size_t byte = 0; byte < pattern.values.size(); ++byte) {
							if ((bytes[start + byte] & pattern.masks[byte]) !=
								(pattern.values[byte] & pattern.masks[byte])) {
								matches = false;
								break;
							}
						}
						if (matches) {
							const auto previous = batchMatchCounts[anchor.patternIndex].fetch_add(
								1,
								std::memory_order_relaxed);
							if (previous <= MAX_INTERMEDIATE_MATCHES) {
								result.matches.emplace_back(anchor.patternIndex, start);
							}
						}
					}
				};
				for (auto offset = a_begin; offset < a_end; ++offset) {
					verifyAt(anchors4, groups4, offset, 4);
					if (offset + 8 <= bytes.size()) {
						verifyAt(anchors8, groups8, offset, 8);
					}
				}
				return result;
			};
			const auto scanOffsets = bytes.size() >= 4 ? bytes.size() - 3 : 0;
			const auto workerCount = bulk_worker_count(scanOffsets, 1u << 20);
			std::vector<std::future<ChunkResult>> workers;
			workers.reserve(workerCount);
			for (std::size_t worker = 0; worker < workerCount; ++worker) {
				workers.push_back(std::async(
					std::launch::async,
					scanChunk,
					worker * scanOffsets / workerCount,
					(worker + 1) * scanOffsets / workerCount));
			}
			for (auto& worker : workers) {
				auto result = worker.get();
				for (const auto& [patternIndex, offset] : result.matches) {
					batch->patterns[patternIndex].matches.push_back(offset);
				}
			}
			if (a_sectionIndex == 0) {
				for_each_decoded_relative_branch(a_module, [&](std::uint32_t a_target, std::size_t a_offset) {
					batch->relativeBranches[a_target].push_back(a_offset);
				});
			}
		};
		for (std::size_t sectionIndex = 0; sectionIndex < 4; ++sectionIndex) {
			processSection(sectionIndex);
		}
		_batchIndex = std::move(batch);
	}

	void RuntimeDatabase::clear_batch() const
	{
		_batchIndex.reset();
		if (_mapped) {
			const std::scoped_lock lock(_recordCacheLock);
			_recordCache.clear();
		}
	}

	const RuntimeDatabase::BatchPattern* RuntimeDatabase::find_batch_pattern(
		std::uint8_t a_section,
		const Pattern& a_pattern) const
	{
		if (!_batchIndex || a_pattern.values.empty() ||
			a_pattern.values.size() != a_pattern.masks.size()) {
			return nullptr;
		}
		auto hash = pattern_hash(a_section, a_pattern.values, a_pattern.masks);
		for (;;) {
			const auto found = _batchIndex->patternsByHash.find(hash);
			if (found == _batchIndex->patternsByHash.end()) {
				return nullptr;
			}
			const auto& pattern = _batchIndex->patterns[found->second];
			if (pattern.section == a_section &&
				same_pattern(pattern.values, pattern.masks,
					a_pattern.values, a_pattern.masks)) {
				return pattern.indexed ? std::addressof(pattern) : nullptr;
			}
			hash = next_pattern_hash(hash);
		}
	}

	std::vector<std::uint32_t> RuntimeDatabase::rip_string_owners(
		std::uint32_t a_targetRVA,
		const Module& a_module) const
	{
		const std::scoped_lock lock(_ripStringIndexLock);
		if (!_ripStringIndexReady || _ripStringModuleBase != a_module.base() ||
			_ripStringImageSize != a_module.image_size()) {
			_ripStringOwners.clear();
			_ripStringModuleBase = a_module.base();
			_ripStringImageSize = a_module.image_size();
			_ripStringIndexReady = true;

			struct RuntimeFunction
			{
				std::uint32_t begin;
				std::uint32_t end;
				std::uint32_t unwind;
			};
			const auto pdata = a_module.segment(Segment::pdata);
			const auto functions = std::span(
				pdata.pointer<const RuntimeFunction>(),
				pdata.size() / sizeof(RuntimeFunction));
			const auto text = a_module.segment(Segment::text);
			const auto rdata = a_module.segment(Segment::rdata);
			if (text.offset() <= UINT32_MAX && rdata.offset() <= UINT32_MAX) {
				const auto bytes = std::span(text.pointer<const std::uint8_t>(), text.size());
				const auto functionContaining = [&](std::uint32_t a_rva)
					-> const RuntimeFunction* {
					auto low = functions.begin();
					auto high = functions.end();
					while (low != high) {
						const auto middle = low + std::distance(low, high) / 2;
						if (middle->begin <= a_rva) {
							low = middle + 1;
						} else {
							high = middle;
						}
					}
					if (low == functions.begin()) {
						return nullptr;
					}
					--low;
					return low->begin <= a_rva && a_rva < low->end ?
					           std::addressof(*low) : nullptr;
				};
				for (std::size_t offset = 0; offset + 7 <= bytes.size(); ++offset) {
					if ((bytes[offset] & 0xF8) != 0x48 || bytes[offset + 1] != 0x8D ||
						(bytes[offset + 2] & 0xC7) != 0x05 ||
						offset > UINT32_MAX - static_cast<std::uint32_t>(text.offset())) {
						continue;
					}
					std::int32_t displacement{};
					std::memcpy(std::addressof(displacement), bytes.data() + offset + 3,
						sizeof(displacement));
					const auto instructionRVA = static_cast<std::uint32_t>(text.offset()) +
					                            static_cast<std::uint32_t>(offset);
					const auto target = static_cast<std::int64_t>(instructionRVA) + 7 + displacement;
					const auto rdataOffset = static_cast<std::int64_t>(rdata.offset());
					if (target < rdataOffset ||
						static_cast<std::uint64_t>(target - rdataOffset) >= rdata.size()) {
						continue;
					}
					const auto* owner = functionContaining(instructionRVA);
					if (owner) {
						_ripStringOwners[static_cast<std::uint32_t>(target)].push_back(owner->begin);
					}
				}
				for (auto& [target, owners] : _ripStringOwners) {
					static_cast<void>(target);
					std::sort(owners.begin(), owners.end());
					owners.erase(std::unique(owners.begin(), owners.end()), owners.end());
				}
			}
		}
		if (const auto found = _ripStringOwners.find(a_targetRVA);
			found != _ripStringOwners.end()) {
			return found->second;
		}
		return {};
	}
	bool RuntimeDatabase::validate_record_result(
		const Record& a_record,
		std::uint32_t a_rva,
		const Module& a_module) const
	{
		if (a_rva >= a_module.image_size()) {
			return false;
		}
		const auto flags = a_record.flags & RECORD_SEMANTIC_MASK;
		const auto requireFunctionStart =
			(a_record.flags & RECORD_REQUIRE_FUNCTION_START) != 0;
		if (flags == 0 && !requireFunctionStart) {
			return true;
		}
		const auto inSegment = [&](Segment::Name a_name, std::uint64_t a_value) {
			const auto segment = a_module.segment(a_name);
			return a_value >= segment.offset() &&
			       a_value - segment.offset() < segment.size();
		};
		const auto inResultSegment = [&](Segment::Name a_name) {
			return inSegment(a_name, a_rva);
		};
		const auto readable = [&](std::uint64_t a_value, std::size_t a_size) {
			for (std::size_t index = 0; index < Segment::total; ++index) {
				const auto segment = a_module.segment(static_cast<Segment::Name>(index));
				if (a_value >= segment.offset() && a_value - segment.offset() <= segment.size() &&
					a_size <= segment.size() - static_cast<std::size_t>(a_value - segment.offset())) {
					return true;
				}
			}
			return false;
		};
		const auto pointerRVA = [&](std::uint64_t a_pointer) -> std::optional<std::uint32_t> {
			const auto imageSize = static_cast<std::uint64_t>(a_module.image_size());
			if (a_pointer >= a_module.base() && a_pointer - a_module.base() < imageSize) {
				return static_cast<std::uint32_t>(a_pointer - a_module.base());
			}
			if (a_pointer >= a_module.preferred_base() &&
				a_pointer - a_module.preferred_base() < imageSize) {
				return static_cast<std::uint32_t>(a_pointer - a_module.preferred_base());
			}
			return std::nullopt;
		};
		const auto readPointer = [&](std::uint32_t a_value) -> std::optional<std::uint64_t> {
			if (!readable(a_value, sizeof(std::uint64_t))) {
				return std::nullopt;
			}
			std::uint64_t pointer{};
			std::memcpy(std::addressof(pointer),
				reinterpret_cast<const void*>(a_module.base() + a_value), sizeof(pointer));
			return pointer;
		};
		const auto readU32 = [&](std::uint32_t a_value) -> std::optional<std::uint32_t> {
			if (!readable(a_value, sizeof(std::uint32_t))) {
				return std::nullopt;
			}
			std::uint32_t value{};
			std::memcpy(std::addressof(value),
				reinterpret_cast<const void*>(a_module.base() + a_value), sizeof(value));
			return value;
		};
		const auto validAscii = [&](std::uint32_t a_value, std::size_t a_minimum) {
			constexpr std::size_t maximum = 4096;
			if (!readable(a_value, 1)) {
				return false;
			}
			const auto* text = reinterpret_cast<const unsigned char*>(a_module.base() + a_value);
			for (std::size_t index = 0; index < maximum && readable(a_value + index, 1); ++index) {
				if (text[index] == 0) {
					return index >= a_minimum;
				}
				if (text[index] < 0x20 || text[index] > 0x7E) {
					return false;
				}
			}
			return false;
		};
		const auto validTypeDescriptor = [&](std::uint32_t a_value) {
			if (a_value % alignof(std::uint64_t) != 0 ||
				(!inSegment(Segment::data, a_value) && !inSegment(Segment::rdata, a_value)) ||
				!readable(a_value, 20)) {
				return false;
			}
			const auto* descriptor = reinterpret_cast<const std::uint8_t*>(
				a_module.base() + a_value);
			if (descriptor[16] != '.' || descriptor[17] != '?' || descriptor[18] != 'A' ||
				!validAscii(a_value + 16, 5)) {
				return false;
			}
			const auto typeInfo = readPointer(a_value);
			const auto typeInfoRVA = typeInfo ? pointerRVA(*typeInfo) : std::nullopt;
			const auto spare = readPointer(a_value + sizeof(std::uint64_t));
			return spare && *spare == 0 && typeInfoRVA &&
				inSegment(Segment::rdata, *typeInfoRVA);
		};
		const auto validHierarchy = [&](std::uint32_t a_value, std::uint32_t a_expectedType) {
			if (a_value % alignof(std::uint32_t) != 0 ||
				!inSegment(Segment::rdata, a_value) || !readable(a_value, 16)) {
				return false;
			}
			const auto signature = readU32(a_value);
			const auto attributes = readU32(a_value + 4);
			const auto baseCount = readU32(a_value + 8);
			const auto baseArray = readU32(a_value + 12);
			if (!signature || *signature != 0 || !attributes ||
				(*attributes & 0xFFFFFFF0u) != 0 || !baseCount || *baseCount == 0 ||
				*baseCount > 4096 || !baseArray || !inSegment(Segment::rdata, *baseArray) ||
				!readable(*baseArray, static_cast<std::size_t>(*baseCount) * sizeof(std::uint32_t))) {
				return false;
			}
			const auto firstBase = readU32(*baseArray);
			if (!firstBase || !inSegment(Segment::rdata, *firstBase) ||
				!readable(*firstBase, 28)) {
				return false;
			}
			const auto firstType = readU32(*firstBase);
			const auto firstAttributes = readU32(*firstBase + 20);
			return firstType && *firstType == a_expectedType && firstAttributes &&
				(*firstAttributes & 0xFFFFFF00u) == 0;
		};
		const auto validLocator = [&](std::uint32_t a_value) {
			if (a_value % alignof(std::uint32_t) != 0 ||
				!inSegment(Segment::rdata, a_value) || !readable(a_value, 24)) {
				return false;
			}
			const auto signature = readU32(a_value);
			const auto typeDescriptor = readU32(a_value + 12);
			const auto hierarchy = readU32(a_value + 16);
			const auto self = readU32(a_value + 20);
			return signature && *signature == 1 && typeDescriptor && hierarchy && self &&
				*self == a_value && validTypeDescriptor(*typeDescriptor) &&
				validHierarchy(*hierarchy, *typeDescriptor);
		};

		if (requireFunctionStart) {
			if (!inResultSegment(Segment::text)) {
				return false;
			}
			struct RuntimeFunction
			{
				std::uint32_t begin;
				std::uint32_t end;
				std::uint32_t unwind;
			};
			const auto pdata = a_module.segment(Segment::pdata);
			const auto functions = std::span(
				pdata.pointer<const RuntimeFunction>(), pdata.size() / sizeof(RuntimeFunction));
			const auto found = std::lower_bound(functions.begin(), functions.end(), a_rva,
				[](const RuntimeFunction& a_function, std::uint32_t a_value) {
					return a_function.begin < a_value;
				});
			if (found == functions.end() || found->begin != a_rva) {
				return false;
			}
		}
		if ((flags & RECORD_FUNCTION) != 0) {
			if (!inResultSegment(Segment::text)) {
				return false;
			}
			struct RuntimeFunction
			{
				std::uint32_t begin;
				std::uint32_t end;
				std::uint32_t unwind;
			};
			const auto pdata = a_module.segment(Segment::pdata);
			const auto functions = std::span(
				pdata.pointer<const RuntimeFunction>(), pdata.size() / sizeof(RuntimeFunction));
			const auto found = std::upper_bound(functions.begin(), functions.end(), a_rva,
				[](std::uint32_t a_value, const RuntimeFunction& a_function) {
					return a_value < a_function.begin;
				});
			const auto inRuntimeFunction = found != functions.begin() && [&] {
				const auto& function = *std::prev(found);
				return a_rva >= function.begin && a_rva < function.end;
			}();
			if (!inRuntimeFunction) {
				if (!readable(a_rva, 8)) {
					return false;
				}
				const auto* bytes = reinterpret_cast<const std::uint8_t*>(a_module.base() + a_rva);
				if (std::all_of(bytes, bytes + 8, [](std::uint8_t value) {
						return value == 0 || value == 0xCC;
					})) {
					return false;
				}
			}
		}
		if ((flags & RECORD_MSVC_RTTI) != 0 && !validTypeDescriptor(a_rva)) {
			return false;
		}
		if ((flags & RECORD_VTABLE) != 0) {
			if (a_rva < sizeof(std::uint64_t) || a_rva % alignof(std::uint64_t) != 0 ||
				!inResultSegment(Segment::rdata) || !readable(a_rva - sizeof(std::uint64_t), 16)) {
				return false;
			}
			const auto locator = readPointer(a_rva - sizeof(std::uint64_t));
			const auto firstSlot = readPointer(a_rva);
			const auto locatorRVA = locator ? pointerRVA(*locator) : std::nullopt;
			const auto firstSlotRVA = firstSlot ? pointerRVA(*firstSlot) : std::nullopt;
			if (!locatorRVA || !firstSlotRVA || !validLocator(*locatorRVA) ||
				!inSegment(Segment::text, *firstSlotRVA)) {
				return false;
			}
		}
		if ((flags & RECORD_NIRTTI) != 0) {
			if (a_rva % alignof(std::uint64_t) != 0 ||
				(!inResultSegment(Segment::data) && !inResultSegment(Segment::rdata)) ||
				!readable(a_rva, 16)) {
				return false;
			}
			const auto name = readPointer(a_rva);
			const auto base = readPointer(a_rva + sizeof(std::uint64_t));
            if (!name || !base) {
                return false;
            }
            if (*name == 0 && *base == 0) {
                return inResultSegment(Segment::data);
            }
            const auto nameRVA = pointerRVA(*name);
            if (!nameRVA || !inSegment(Segment::rdata, *nameRVA) ||
                !validAscii(*nameRVA, 1)) {
				return false;
			}
			if (*base != 0) {
				const auto baseRVA = pointerRVA(*base);
				if (!baseRVA || *baseRVA % alignof(std::uint64_t) != 0 ||
					(!inSegment(Segment::data, *baseRVA) &&
						!inSegment(Segment::rdata, *baseRVA)) ||
					!readable(*baseRVA, sizeof(std::uint64_t) * 2)) {
					return false;
				}
			}
		}
		if ((flags & RECORD_GLOBAL) != 0) {
			if (a_rva % alignof(std::uint32_t) != 0 ||
				(!inResultSegment(Segment::data) && !inResultSegment(Segment::rdata) &&
					!inResultSegment(Segment::idata)) ||
				!readable(a_rva, sizeof(std::uint32_t))) {
				return false;
			}
		}
		if ((flags & RECORD_SINGLETON) != 0) {
			if (a_rva % alignof(std::uint64_t) != 0 ||
				!inResultSegment(Segment::data) ||
				!readable(a_rva, sizeof(std::uint64_t))) {
				return false;
			}
		}
		if ((flags & RECORD_EVENT) != 0) {
			if (a_rva % alignof(std::uint32_t) != 0 ||
				!inResultSegment(Segment::data) ||
				!readable(a_rva, sizeof(std::uint32_t))) {
				return false;
			}
		}
		return true;
	}

	std::optional<std::uint32_t> RuntimeDatabase::resolve_candidate(
		const Candidate& a_candidate,
		const Module& a_module,
		ResolveContext& a_context) const
	{
		if (is_unavailable_resolution(a_candidate.resolution)) {
			if (!a_candidate.fragments.empty() || !a_candidate.constraints.empty() ||
				a_candidate.resolveFragment != 0 || a_candidate.resolveOffset != 0 ||
				a_candidate.resultAdjustment != 0 || a_candidate.minFixedBytes != 0) {
				a_context.record_failure(ResolveFailure::kInvalidPattern);
			} else {
				a_context.record_failure(ResolveFailure::kRuntimeUnavailable);
			}
			return std::nullopt;
		}
		const auto commonResultAllowed = [&](std::uint32_t a_rva) {
			if ((a_candidate.flags & CANDIDATE_RESULT_ALIGN8) != 0 && a_rva % 8 != 0) {
				return false;
			}
			const auto required = a_candidate.flags & CANDIDATE_RESULT_SECTION_MASK;
			if (required == 0) {
				return true;
			}
			const auto inSegment = [&](Segment::Name a_name) {
				const auto segment = a_module.segment(a_name);
				return a_rva >= segment.offset() &&
				       static_cast<std::uint64_t>(a_rva - segment.offset()) < segment.size();
			};
			return ((required & CANDIDATE_RESULT_TEXT) != 0 && inSegment(Segment::text)) ||
			       ((required & CANDIDATE_RESULT_RDATA) != 0 && inSegment(Segment::rdata)) ||
			       ((required & CANDIDATE_RESULT_DATA) != 0 && inSegment(Segment::data)) ||
			       ((required & CANDIDATE_RESULT_IDATA) != 0 && inSegment(Segment::idata));
		};
		const auto commonReadable = [&](std::uint64_t a_rva, std::size_t a_size) {
			for (std::size_t index = 0; index < Segment::total; ++index) {
				const auto segment = a_module.segment(static_cast<Segment::Name>(index));
				if (a_rva >= segment.offset() && a_rva - segment.offset() <= segment.size() &&
					a_size <= segment.size() - static_cast<std::size_t>(a_rva - segment.offset())) {
					return true;
				}
			}
			return false;
		};
		const auto commonPointerRVA = [&](std::uint64_t a_pointer)
			-> std::optional<std::uint32_t> {
			const auto imageSize = static_cast<std::uint64_t>(a_module.image_size());
			if (a_pointer >= a_module.base() && a_pointer - a_module.base() < imageSize) {
				return static_cast<std::uint32_t>(a_pointer - a_module.base());
			}
			const auto preferred = a_module.preferred_base();
			if (a_pointer >= preferred && a_pointer - preferred < imageSize) {
				return static_cast<std::uint32_t>(a_pointer - preferred);
			}
			return std::nullopt;
		};
		const auto patternMatches = [](std::span<const std::uint8_t> a_bytes,
			const Pattern& a_pattern) {
			if (a_pattern.values.size() != a_pattern.masks.size() ||
				a_pattern.values.size() > a_bytes.size()) {
				return false;
			}
			for (std::size_t index = 0; index < a_pattern.values.size(); ++index) {
				if ((a_bytes[index] & a_pattern.masks[index]) !=
					(a_pattern.values[index] & a_pattern.masks[index])) {
					return false;
				}
			}
			return true;
		};

		if (is_slot_pattern_dependency(a_candidate.resolution)) {
			if (a_candidate.fragments.size() < 2 || a_candidate.constraints.size() < 2 ||
				a_candidate.resolveFragment != 0 || a_candidate.resolveOffset != 0 ||
				a_candidate.resultAdjustment < 0 || a_candidate.section != 2) {
				a_context.record_failure(ResolveFailure::kInvalidPattern);
				return std::nullopt;
			}
			std::size_t fixedBytes{};
			for (const auto& fragment : a_candidate.fragments) {
				if (fragment.pattern.values.empty() ||
					fragment.pattern.values.size() != fragment.pattern.masks.size()) {
					a_context.record_failure(ResolveFailure::kInvalidPattern);
					return std::nullopt;
				}
				fixedBytes += static_cast<std::size_t>(std::count_if(
					fragment.pattern.masks.begin(), fragment.pattern.masks.end(),
					[](const auto mask) { return mask != 0; }));
			}
			if (fixedBytes < a_candidate.minFixedBytes) {
				a_context.record_failure(ResolveFailure::kInvalidPattern);
				return std::nullopt;
			}
			std::uint32_t requiredBytes{};
			for (const auto& constraint : a_candidate.constraints) {
				if (constraint.kind != CONSTRAINT_POINTED_PATTERN || constraint.targetID != 0 ||
					constraint.targetAdjustment != 0 || constraint.flags != 0 ||
					constraint.fragmentIndex >= a_candidate.fragments.size() ||
					constraint.operandOffset % sizeof(std::uint64_t) != 0 ||
					constraint.operandOffset > UINT32_MAX - sizeof(std::uint64_t)) {
					a_context.record_failure(ResolveFailure::kInvalidPattern);
					return std::nullopt;
				}
				requiredBytes = std::max(requiredBytes,
					constraint.operandOffset + static_cast<std::uint32_t>(sizeof(std::uint64_t)));
			}
			const auto rdata = a_module.segment(Segment::rdata);
			const auto bytes = std::span(
				rdata.pointer<const std::uint8_t>(), static_cast<std::size_t>(rdata.size()));
			std::vector<std::uint32_t> matches;
			for (std::size_t start = 0; start + requiredBytes <= bytes.size();
				 start += alignof(std::uint64_t)) {
				bool matchesAll = true;
				for (const auto& constraint : a_candidate.constraints) {
					std::uint64_t pointer{};
					std::memcpy(std::addressof(pointer),
						bytes.data() + start + constraint.operandOffset, sizeof(pointer));
					const auto functionRVA = commonPointerRVA(pointer);
					const auto& pattern =
						a_candidate.fragments[constraint.fragmentIndex].pattern;
					if (!functionRVA || !commonReadable(*functionRVA, pattern.values.size()) ||
						!patternMatches(std::span(
							reinterpret_cast<const std::uint8_t*>(a_module.base() + *functionRVA),
							pattern.values.size()), pattern)) {
						matchesAll = false;
						break;
					}
				}
				if (!matchesAll || rdata.offset() > UINT32_MAX ||
					start > UINT32_MAX - static_cast<std::uint32_t>(rdata.offset())) {
					continue;
				}
				const auto found = static_cast<std::uint32_t>(rdata.offset()) +
				                   static_cast<std::uint32_t>(start);
				if (commonResultAllowed(found)) {
					matches.push_back(found);
				}
			}
			std::sort(matches.begin(), matches.end());
			matches.erase(std::unique(matches.begin(), matches.end()), matches.end());
			const auto ordinal = static_cast<std::size_t>(a_candidate.resultAdjustment);
			if (ordinal >= matches.size()) {
				a_context.record_failure(ResolveFailure::kPatternNotFound);
				return std::nullopt;
			}
			return matches[ordinal];
		}

		if (is_scoped_rip_dependency(a_candidate.resolution)) {
			if (a_candidate.fragments.size() != 1 || a_candidate.constraints.size() != 1 ||
				a_candidate.resolveFragment != 0 || a_candidate.section != 1 ||
				a_candidate.resultAdjustment < 0) {
				a_context.record_failure(ResolveFailure::kInvalidPattern);
				return std::nullopt;
			}
			const auto& fragment = a_candidate.fragments.front();
			const auto& constraint = a_candidate.constraints.front();
			const auto fixedBytes = static_cast<std::size_t>(std::count_if(
				fragment.pattern.masks.begin(), fragment.pattern.masks.end(),
				[](const auto mask) { return mask != 0; }));
			if (fragment.pattern.values.empty() ||
				fragment.pattern.values.size() != fragment.pattern.masks.size() ||
				fixedBytes < a_candidate.minFixedBytes ||
				a_candidate.resolveOffset > fragment.pattern.values.size() ||
				sizeof(std::int32_t) > fragment.pattern.values.size() - a_candidate.resolveOffset ||
				constraint.kind != CONSTRAINT_RESOLVED_ID || constraint.operandOffset != 0 ||
				constraint.targetAdjustment != 0 || constraint.fragmentIndex != 0 ||
				constraint.flags != 0) {
				a_context.record_failure(ResolveFailure::kInvalidPattern);
				return std::nullopt;
			}
			const auto anchor = resolve_patterns(constraint.targetID, a_module, a_context);
			if (!anchor) {
				return std::nullopt;
			}
			struct RuntimeFunction
			{
				std::uint32_t begin;
				std::uint32_t end;
				std::uint32_t unwind;
			};
			const auto pdata = a_module.segment(Segment::pdata);
			if (pdata.size() < sizeof(RuntimeFunction) ||
				pdata.size() % sizeof(RuntimeFunction) != 0) {
				a_context.record_failure(ResolveFailure::kInvalidPattern);
				return std::nullopt;
			}
			const auto functions = std::span(
				pdata.pointer<const RuntimeFunction>(), pdata.size() / sizeof(RuntimeFunction));
			const auto rootOf = [&](RuntimeFunction function) -> std::optional<RuntimeFunction> {
				if (function.begin >= function.end || function.end > a_module.image_size()) {
					return std::nullopt;
				}
				std::unordered_set<std::uint32_t> followed;
				while (function.unwind != 0) {
					if (!followed.insert(function.unwind).second ||
						!commonReadable(function.unwind, 4)) {
						return std::nullopt;
					}
					const auto* header = reinterpret_cast<const std::uint8_t*>(
						a_module.base() + function.unwind);
					if (((header[0] >> 3) & 0x4) == 0) {
						break;
					}
					const auto codeCount = static_cast<std::size_t>(header[2]);
					const auto chainedOffset = static_cast<std::size_t>(4) +
						((codeCount + 1) & ~std::size_t{ 1 }) * 2;
					if (function.unwind > UINT32_MAX - chainedOffset ||
						!commonReadable(function.unwind + chainedOffset, sizeof(RuntimeFunction))) {
						return std::nullopt;
					}
					RuntimeFunction chained{};
					std::memcpy(std::addressof(chained),
						reinterpret_cast<const void*>(a_module.base() + function.unwind + chainedOffset),
						sizeof(chained));
					if (chained.begin >= chained.end || chained.end > a_module.image_size()) {
						return std::nullopt;
					}
					function = chained;
				}
				return function;
			};
			std::optional<RuntimeFunction> root;
			for (const auto& function : functions) {
				if (function.begin <= *anchor && *anchor < function.end) {
					const auto candidate = rootOf(function);
					if (!candidate) {
						a_context.record_failure(ResolveFailure::kInvalidPattern);
						return std::nullopt;
					}
					if (!root) {
						root = *candidate;
					} else if (candidate->begin != root->begin || candidate->end != root->end) {
						a_context.record_failure(ResolveFailure::kInvalidPattern);
						return std::nullopt;
					}
				}
			}
			if (!root) {
				a_context.record_failure(ResolveFailure::kPatternNotFound);
				return std::nullopt;
			}
			std::vector<std::pair<std::uint32_t, std::uint32_t>> scopes;
			scopes.emplace_back(root->begin, root->end);
			for (const auto& function : functions) {
				const auto candidate = rootOf(function);
				if (!candidate) {
					a_context.record_failure(ResolveFailure::kInvalidPattern);
					return std::nullopt;
				}
				if (candidate->begin == root->begin && candidate->end == root->end) {
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
			for (const auto& [begin, end] : merged) {
				if (begin >= end || begin < text.offset() ||
					static_cast<std::uint64_t>(end - text.offset()) > text.size() ||
					!commonReadable(begin, end - begin)) {
					a_context.record_failure(ResolveFailure::kInvalidPattern);
					return std::nullopt;
				}
				const auto size = static_cast<std::size_t>(end - begin);
				if (size > MAX_SCOPED_FUNCTION_BYTES - totalBytes) {
					a_context.record_failure(ResolveFailure::kInvalidPattern);
					return std::nullopt;
				}
				totalBytes += size;
			}
			std::optional<std::uint32_t> resolved;
			for (const auto& [begin, end] : merged) {
				const auto bytes = std::span(
					reinterpret_cast<const std::uint8_t*>(a_module.base() + begin),
					static_cast<std::size_t>(end - begin));
				for (std::size_t offset = 0;
					offset + fragment.pattern.values.size() <= bytes.size(); ++offset) {
					if (!patternMatches(bytes.subspan(offset), fragment.pattern)) {
						continue;
					}
					std::int32_t displacement{};
					std::memcpy(std::addressof(displacement),
						bytes.data() + offset + a_candidate.resolveOffset, sizeof(displacement));
					const auto value = static_cast<std::int64_t>(begin) + offset +
						a_candidate.resolveOffset + sizeof(displacement) +
						a_candidate.resultAdjustment + displacement;
					if (value < 0 || static_cast<std::uint64_t>(value) >= a_module.image_size() ||
						!commonResultAllowed(static_cast<std::uint32_t>(value))) {
						continue;
					}
					const auto found = static_cast<std::uint32_t>(value);
					if (resolved && *resolved != found) {
						a_context.record_failure(ResolveFailure::kPatternAmbiguous);
						return std::nullopt;
					}
					resolved = found;
				}
			}
			if (!resolved) {
				a_context.record_failure(ResolveFailure::kPatternNotFound);
			}
			return resolved;
		}

		if (is_id_only_dependency(a_candidate.resolution)) {
			if (!a_candidate.fragments.empty() || a_candidate.constraints.empty() ||
				a_candidate.resolveFragment != 0 || a_candidate.resolveOffset != 0 ||
				a_candidate.minFixedBytes != 0) {
				a_context.record_failure(ResolveFailure::kInvalidPattern);
				return std::nullopt;
			}
			struct RuntimeFunction
			{
				std::uint32_t begin;
				std::uint32_t end;
				std::uint32_t unwind;
			};
			const auto pdata = a_module.segment(Segment::pdata);
			const auto functions = std::span(
				pdata.pointer<const RuntimeFunction>(), pdata.size() / sizeof(RuntimeFunction));
			const auto resultAllowed = [&](std::uint32_t a_rva) {
				if ((a_candidate.flags & CANDIDATE_RESULT_ALIGN8) != 0 && a_rva % 8 != 0) {
					return false;
				}
				const auto required = a_candidate.flags & CANDIDATE_RESULT_SECTION_MASK;
				if (required == 0) {
					return true;
				}
				const auto inSegment = [&](Segment::Name a_name) {
					const auto segment = a_module.segment(a_name);
					return a_rva >= segment.offset() &&
					       static_cast<std::uint64_t>(a_rva - segment.offset()) < segment.size();
				};
				return ((required & CANDIDATE_RESULT_TEXT) != 0 && inSegment(Segment::text)) ||
				       ((required & CANDIDATE_RESULT_RDATA) != 0 && inSegment(Segment::rdata)) ||
				       ((required & CANDIDATE_RESULT_DATA) != 0 && inSegment(Segment::data)) ||
				       ((required & CANDIDATE_RESULT_IDATA) != 0 && inSegment(Segment::idata));
			};
			if (a_candidate.resolution == RESOLUTION_RDATA_VTABLE_FROM_IDS) {
				if (a_candidate.constraints.size() < 2 || a_candidate.resultAdjustment != 0) {
					a_context.record_failure(ResolveFailure::kInvalidPattern);
					return std::nullopt;
				}
				struct Slot
				{
					std::uint32_t offset;
					std::uintptr_t pointer;
				};
				std::vector<Slot> slots;
				slots.reserve(a_candidate.constraints.size());
				std::uint32_t requiredBytes{};
				for (const auto& constraint : a_candidate.constraints) {
					if (constraint.kind != CONSTRAINT_RESOLVED_ID || constraint.fragmentIndex != 0 ||
						constraint.flags != 0 || constraint.operandOffset % sizeof(std::uint64_t) != 0 ||
						constraint.operandOffset > UINT32_MAX - sizeof(std::uint64_t)) {
						a_context.record_failure(ResolveFailure::kInvalidPattern);
						return std::nullopt;
					}
					const auto anchor = resolve_patterns(constraint.targetID, a_module, a_context);
					const auto adjusted = anchor ?
						static_cast<std::int64_t>(*anchor) + constraint.targetAdjustment : -1;
					if (adjusted < 0 || static_cast<std::uint64_t>(adjusted) >= a_module.image_size()) {
						return std::nullopt;
					}
					slots.push_back({ constraint.operandOffset,
						a_module.base() + static_cast<std::uint32_t>(adjusted) });
					requiredBytes = std::max(requiredBytes,
						constraint.operandOffset + static_cast<std::uint32_t>(sizeof(std::uint64_t)));
				}
				const auto rdata = a_module.segment(Segment::rdata);
				const auto bytes = std::span(
					rdata.pointer<const std::uint8_t>(), static_cast<std::size_t>(rdata.size()));
				std::optional<std::uint32_t> resolved;
				for (std::size_t start = 0; start + requiredBytes <= bytes.size();
					 start += alignof(std::uint64_t)) {
					const auto matches = std::all_of(slots.begin(), slots.end(), [&](const auto& slot) {
						std::uintptr_t pointer{};
						std::memcpy(std::addressof(pointer), bytes.data() + start + slot.offset,
							sizeof(pointer));
						return pointer == slot.pointer;
					});
					if (!matches || rdata.offset() > UINT32_MAX ||
						start > UINT32_MAX - static_cast<std::uint32_t>(rdata.offset())) {
						continue;
					}
					const auto found = static_cast<std::uint32_t>(rdata.offset()) +
						static_cast<std::uint32_t>(start);
					if (!resultAllowed(found)) {
						continue;
					}
					if (resolved && *resolved != found) {
						a_context.record_failure(ResolveFailure::kPatternAmbiguous);
						return std::nullopt;
					}
					resolved = found;
				}
				return resolved;
			}
			if (a_candidate.resolution ==
				RESOLUTION_MSVC_TYPE_DESCRIPTOR_FROM_VTABLE_ID) {
				if (a_candidate.constraints.size() != 1 || a_candidate.resultAdjustment != 0) {
					a_context.record_failure(ResolveFailure::kInvalidPattern);
					return std::nullopt;
				}
				const auto& constraint = a_candidate.constraints.front();
				if (constraint.kind != CONSTRAINT_RESOLVED_ID || constraint.operandOffset != 0 ||
					constraint.targetAdjustment != 0 || constraint.fragmentIndex != 0 ||
					constraint.flags != 0) {
					a_context.record_failure(ResolveFailure::kInvalidPattern);
					return std::nullopt;
				}
				const auto vtable = resolve_patterns(constraint.targetID, a_module, a_context);
				if (!vtable || *vtable < sizeof(std::uint64_t) ||
					!commonReadable(*vtable - sizeof(std::uint64_t), sizeof(std::uint64_t))) {
					return std::nullopt;
				}
				std::uint64_t locatorPointer{};
				std::memcpy(std::addressof(locatorPointer),
					reinterpret_cast<const void*>(a_module.base() + *vtable - sizeof(std::uint64_t)),
					sizeof(locatorPointer));
				const auto locator = commonPointerRVA(locatorPointer);
				if (!locator || *locator > UINT32_MAX - 12 ||
					!commonReadable(*locator + 12, sizeof(std::uint32_t))) {
					return std::nullopt;
				}
				std::uint32_t typeDescriptor{};
				std::memcpy(std::addressof(typeDescriptor),
					reinterpret_cast<const void*>(a_module.base() + *locator + 12),
					sizeof(typeDescriptor));
				if (typeDescriptor >= a_module.image_size() || !resultAllowed(typeDescriptor)) {
					return std::nullopt;
				}
				return typeDescriptor;
			}
			std::optional<std::uint32_t> resolved;
			for (const auto& constraint : a_candidate.constraints) {
				if (constraint.kind != CONSTRAINT_RESOLVED_ID || constraint.operandOffset != 0 ||
					constraint.fragmentIndex != 0 || constraint.flags != 0) {
					a_context.record_failure(ResolveFailure::kInvalidPattern);
					return std::nullopt;
				}
				const auto anchor = resolve_patterns(constraint.targetID, a_module, a_context);
				if (!anchor) {
					return std::nullopt;
				}
				std::int64_t value{};
				if (a_candidate.resolution == RESOLUTION_RELATIVE_TO_ID) {
					value = static_cast<std::int64_t>(*anchor) + constraint.targetAdjustment;
				} else {
					auto low = functions.begin();
					auto high = functions.end();
					while (low != high) {
						const auto middle = low + std::distance(low, high) / 2;
						if (middle->begin <= *anchor) {
							low = middle + 1;
						} else {
							high = middle;
						}
					}
					if (low == functions.begin()) {
						return std::nullopt;
					}
					--low;
					if (*anchor < low->begin || *anchor >= low->end) {
						return std::nullopt;
					}
					const auto index = static_cast<std::int64_t>(std::distance(functions.begin(), low));
					const auto targetIndex = index + constraint.targetAdjustment;
					if (targetIndex < 0 || static_cast<std::uint64_t>(targetIndex) >= functions.size()) {
						return std::nullopt;
					}
					value = functions[static_cast<std::size_t>(targetIndex)].begin;
				}
				if (value < 0 || static_cast<std::uint64_t>(value) >= a_module.image_size()) {
					return std::nullopt;
				}
				const auto candidateRVA = static_cast<std::uint32_t>(value);
				if (!resultAllowed(candidateRVA)) {
					return std::nullopt;
				}
				if (resolved && *resolved != candidateRVA) {
					a_context.record_failure(ResolveFailure::kPatternAmbiguous);
					return std::nullopt;
				}
				resolved = candidateRVA;
			}
			return resolved;
		}
		if (a_candidate.fragments.empty() || a_candidate.resolveFragment >= a_candidate.fragments.size()) {
			a_context.record_failure(ResolveFailure::kInvalidPattern);
			return std::nullopt;
		}
		if (std::any_of(a_candidate.fragments.begin(), a_candidate.fragments.end(),
				[](const Fragment& a_fragment) {
					return a_fragment.pattern.values.empty() ||
			               a_fragment.pattern.values.size() != a_fragment.pattern.masks.size();
				})) {
			a_context.record_failure(ResolveFailure::kInvalidPattern);
			return std::nullopt;
		}
		if (a_candidate.resolution == RESOLUTION_FUNCTION_FROM_RIP_RELATIVE_STRING) {
			const auto& fragment = a_candidate.fragments.front();
			if (a_candidate.section != 2 || a_candidate.fragments.size() != 1 ||
				!a_candidate.constraints.empty() || a_candidate.resolveFragment != 0 ||
				a_candidate.resolveOffset != 0 || a_candidate.resultAdjustment != 0 ||
				a_candidate.minFixedBytes < sizeof(std::uint64_t) ||
				fragment.minGap != 0 || fragment.maxGap != 0 || fragment.flags != 0 ||
				fragment.pattern.values.size() < sizeof(std::uint64_t) ||
				a_candidate.minFixedBytes > fragment.pattern.values.size() ||
				!std::all_of(fragment.pattern.masks.begin(), fragment.pattern.masks.end(),
					[](const auto mask) { return mask == 0xFF; })) {
				a_context.record_failure(ResolveFailure::kInvalidPattern);
				return std::nullopt;
			}
		}
		Segment scan;
		switch (a_candidate.section) {
		case 1:
			scan = a_module.segment(Segment::text);
			break;
		case 2:
			scan = a_module.segment(Segment::rdata);
			break;
		case 3:
			scan = a_module.segment(Segment::data);
			break;
		case 4:
			scan = a_module.segment(Segment::idata);
			break;
		default:
			a_context.record_failure(ResolveFailure::kInvalidPattern);
			return std::nullopt;
		}
		const auto scanBytes = std::span(
			scan.pointer<const std::uint8_t>(),
			scan.size());
		const auto matchesAt = [&](std::size_t a_offset, const Pattern& a_pattern) {
			if (a_offset > scanBytes.size() || a_pattern.values.size() > scanBytes.size() - a_offset) {
				return false;
			}
			for (std::size_t byte = 0; byte < a_pattern.values.size(); ++byte) {
				if ((scanBytes[a_offset + byte] & a_pattern.masks[byte]) != a_pattern.values[byte]) {
					return false;
				}
			}
			return true;
		};

		const auto findMatches = [&](const Pattern& a_pattern,
									 std::size_t a_begin,
									 std::size_t a_end,
									 std::size_t a_limit) {
			std::vector<std::size_t> matches;
			if (a_pattern.values.empty() || a_pattern.values.size() != a_pattern.masks.size() ||
				a_begin > a_end || a_end > scanBytes.size() ||
				a_pattern.values.size() > a_end - a_begin) {
				return matches;
			}
			std::size_t fixedRunStart{};
			std::size_t fixedRunLength{};
			for (std::size_t begin = 0; begin < a_pattern.masks.size();) {
				if (a_pattern.masks[begin] != 0xFF) {
					++begin;
					continue;
				}
				auto end = begin + 1;
				while (end < a_pattern.masks.size() && a_pattern.masks[end] == 0xFF) {
					++end;
				}
				if (end - begin > fixedRunLength) {
					fixedRunStart = begin;
					fixedRunLength = end - begin;
				}
				begin = end;
			}
			if (fixedRunLength != 0) {
				const auto lastStart = a_end - a_pattern.values.size();
				std::array<std::size_t, 256> skip;
				skip.fill(fixedRunLength);
				for (std::size_t i = 0; i + 1 < fixedRunLength; ++i) {
					skip[a_pattern.values[fixedRunStart + i]] = fixedRunLength - 1 - i;
				}
				for (auto offset = a_begin; offset <= lastStart && matches.size() < a_limit;) {
					auto remaining = fixedRunLength;
					while (remaining != 0 &&
						   scanBytes[offset + fixedRunStart + remaining - 1] ==
							   a_pattern.values[fixedRunStart + remaining - 1]) {
						--remaining;
					}
					if (remaining == 0) {
						if (matchesAt(offset, a_pattern)) {
							matches.push_back(offset);
						}
						++offset;
					} else {
						offset += skip[scanBytes[offset + fixedRunStart + fixedRunLength - 1]];
					}
				}
			} else {
				const auto lastStart = a_end - a_pattern.values.size();
				for (auto offset = a_begin; offset <= lastStart && matches.size() < a_limit; ++offset) {
					if (matchesAt(offset, a_pattern)) {
						matches.push_back(offset);
					}
				}
			}
			return matches;
		};

		std::vector<std::size_t> firstMatches;
		if (a_candidate.resolution == 7) {
			if (a_candidate.section != 1 || a_candidate.constraints.empty() ||
				a_candidate.constraints.front().operandOffset == 0) {
				a_context.record_failure(ResolveFailure::kInvalidPattern);
				return std::nullopt;
			}
			const auto& anchor = a_candidate.constraints.front();
			const auto target = resolve_patterns(anchor.targetID, a_module, a_context);
			if (!target) {
				return std::nullopt;
			}
			const auto inspectBranch = [&](std::size_t a_offset) {
				if (a_offset + 1 < anchor.operandOffset) {
					return;
				}
				const auto source = a_offset + 1 - anchor.operandOffset;
				if (matchesAt(source, a_candidate.fragments.front().pattern)) {
					firstMatches.push_back(source);
				}
			};
			const auto rawTarget = static_cast<std::int64_t>(*target) - anchor.targetAdjustment;
			if (_batchIndex && rawTarget >= 0 && rawTarget <= UINT32_MAX) {
				if (const auto found = _batchIndex->relativeBranches.find(
						static_cast<std::uint32_t>(rawTarget));
					found != _batchIndex->relativeBranches.end()) {
					for (const auto offset : found->second) {
						inspectBranch(offset);
						if (firstMatches.size() > MAX_INTERMEDIATE_MATCHES) {
							break;
						}
					}
				}
			} else {
				for_each_decoded_relative_branch(
					a_module,
					[&](std::uint32_t a_branchTarget, std::size_t a_offset) {
						if (firstMatches.size() > MAX_INTERMEDIATE_MATCHES ||
							static_cast<std::int64_t>(a_branchTarget) + anchor.targetAdjustment != *target) {
							return;
						}
						inspectBranch(a_offset);
					});
			}
		} else {
			if (const auto* batchPattern = find_batch_pattern(
					a_candidate.section,
					a_candidate.fragments.front().pattern)) {
				firstMatches = batchPattern->matches;
			} else {
				firstMatches = findMatches(
					a_candidate.fragments.front().pattern,
					0,
					scanBytes.size(),
					MAX_INTERMEDIATE_MATCHES + 1);
			}
		}
		if (firstMatches.empty()) {
			a_context.record_failure(ResolveFailure::kPatternNotFound);
			return std::nullopt;
		}
		if (firstMatches.size() > MAX_INTERMEDIATE_MATCHES) {
			a_context.record_failure(ResolveFailure::kPatternAmbiguous);
			return std::nullopt;
		}

		if (a_candidate.resolution == RESOLUTION_FUNCTION_FROM_RIP_RELATIVE_STRING) {
			if (firstMatches.size() != 1 || scan.offset() > UINT32_MAX ||
				firstMatches.front() > UINT32_MAX - static_cast<std::uint32_t>(scan.offset())) {
				a_context.record_failure(ResolveFailure::kPatternAmbiguous);
				return std::nullopt;
			}
			const auto targetRVA = static_cast<std::uint32_t>(scan.offset()) +
			                       static_cast<std::uint32_t>(firstMatches.front());
			const auto owners = rip_string_owners(targetRVA, a_module);
			if (owners.empty()) {
				a_context.record_failure(ResolveFailure::kPatternNotFound);
				return std::nullopt;
			}
			if (owners.size() != 1) {
				a_context.record_failure(ResolveFailure::kPatternAmbiguous);
				return std::nullopt;
			}
			if (!commonResultAllowed(owners.front())) {
				a_context.record_failure(ResolveFailure::kSemanticValidationFailed);
				return std::nullopt;
			}
			return owners.front();
		}
		const auto imageSize = static_cast<std::uint64_t>(a_module.image_size());
		const auto resultAllowed = [&](std::uint32_t a_rva) {
			if ((a_candidate.flags & CANDIDATE_RESULT_ALIGN8) != 0 && a_rva % 8 != 0) {
				return false;
			}
			const auto required = a_candidate.flags & CANDIDATE_RESULT_SECTION_MASK;
			if (required == 0) {
				return true;
			}
			const auto inSegment = [&](Segment::Name a_name) {
				const auto segment = a_module.segment(a_name);
				return a_rva >= segment.offset() &&
				       static_cast<std::uint64_t>(a_rva - segment.offset()) < segment.size();
			};
			return ((required & CANDIDATE_RESULT_TEXT) != 0 && inSegment(Segment::text)) ||
			       ((required & CANDIDATE_RESULT_RDATA) != 0 && inSegment(Segment::rdata)) ||
			       ((required & CANDIDATE_RESULT_DATA) != 0 && inSegment(Segment::data)) ||
			       ((required & CANDIDATE_RESULT_IDATA) != 0 && inSegment(Segment::idata));
		};
		const auto preferredBase = a_module.preferred_base();
		const auto pointerRVA = [&](std::uint64_t a_pointer) -> std::optional<std::uint32_t> {
			if (a_pointer >= a_module.base() && a_pointer - a_module.base() < imageSize) {
				return static_cast<std::uint32_t>(a_pointer - a_module.base());
			}
			if (a_pointer >= preferredBase && a_pointer - preferredBase < imageSize) {
				return static_cast<std::uint32_t>(a_pointer - preferredBase);
			}
			return std::nullopt;
		};
		const auto readable = [&](std::uint64_t a_rva, std::size_t a_size) {
			for (std::size_t i = 0; i < Segment::total; ++i) {
				const auto segment = a_module.segment(static_cast<Segment::Name>(i));
				if (a_rva >= segment.offset() && a_rva - segment.offset() <= segment.size() &&
					a_size <= segment.size() - static_cast<std::size_t>(a_rva - segment.offset())) {
					return true;
				}
			}
			return false;
		};

		const auto inSegment = [&](Segment::Name a_name, std::uint64_t a_rva) {
			const auto segment = a_module.segment(a_name);
			return a_rva >= segment.offset() && a_rva - segment.offset() < segment.size();
		};
		const auto readPointer = [&](std::uint32_t a_rva) -> std::optional<std::uint64_t> {
			if (!readable(a_rva, sizeof(std::uint64_t))) {
				return std::nullopt;
			}
			std::uint64_t value{};
			std::memcpy(std::addressof(value),
				reinterpret_cast<const void*>(a_module.base() + a_rva), sizeof(value));
			return value;
		};
		const auto readU32 = [&](std::uint32_t a_rva) -> std::optional<std::uint32_t> {
			if (!readable(a_rva, sizeof(std::uint32_t))) {
				return std::nullopt;
			}
			std::uint32_t value{};
			std::memcpy(std::addressof(value),
				reinterpret_cast<const void*>(a_module.base() + a_rva), sizeof(value));
			return value;
		};
		const auto validMsvcName = [&](std::uint32_t a_rva) {
			constexpr std::size_t maximum = 4096;
			for (std::size_t index = 0; index < maximum; ++index) {
				if (index > UINT32_MAX - a_rva ||
					!readable(a_rva + static_cast<std::uint32_t>(index), 1)) {
					return false;
				}
				const auto value = *reinterpret_cast<const unsigned char*>(
					a_module.base() + a_rva + index);
				if (value == 0) {
					return index >= 5;
				}
				if (value < 0x20 || value > 0x7E ||
					(index == 0 && value != '.') ||
					(index == 1 && value != '?') ||
					(index == 2 && value != 'A')) {
					return false;
				}
			}
			return false;
		};
		const auto validMsvcTypeDescriptor = [&](std::uint32_t a_rva) {
			if (a_rva % alignof(std::uint64_t) != 0 ||
				(!inSegment(Segment::data, a_rva) && !inSegment(Segment::rdata, a_rva)) ||
				!readable(a_rva, 20)) {
				return false;
			}
			const auto typeInfo = readPointer(a_rva);
			const auto spare = readPointer(a_rva + sizeof(std::uint64_t));
			const auto typeInfoRVA = typeInfo ? pointerRVA(*typeInfo) : std::nullopt;
			return spare && *spare == 0 && typeInfoRVA &&
				inSegment(Segment::rdata, *typeInfoRVA) &&
				a_rva <= UINT32_MAX - 16 && validMsvcName(a_rva + 16);
		};
		const auto validMsvcBaseDescriptor = [&](std::uint32_t a_rva,
			std::optional<std::uint32_t> a_expectedType = std::nullopt) {
			if (a_rva % alignof(std::uint32_t) != 0 ||
				!inSegment(Segment::rdata, a_rva) || !readable(a_rva, 28)) {
				return false;
			}
			const auto type = readU32(a_rva);
			const auto containedBases = readU32(a_rva + 4);
			const auto attributes = readU32(a_rva + 20);
			const auto hierarchy = readU32(a_rva + 24);
			if (!type || !containedBases || !attributes || !hierarchy ||
				(a_expectedType && *type != *a_expectedType) || *containedBases > 4096 ||
				(*attributes & 0xFFFFFF00u) != 0 || !validMsvcTypeDescriptor(*type)) {
				return false;
			}
			return *hierarchy == 0 ||
				(inSegment(Segment::rdata, *hierarchy) && readable(*hierarchy, 16));
		};
		const auto validMsvcHierarchy = [&](std::uint32_t a_rva,
			std::uint32_t a_expectedType, bool a_validateAllBases) {
			if (a_rva % alignof(std::uint32_t) != 0 ||
				!inSegment(Segment::rdata, a_rva) || !readable(a_rva, 16)) {
				return false;
			}
			const auto signature = readU32(a_rva);
			const auto attributes = readU32(a_rva + 4);
			const auto baseCount = readU32(a_rva + 8);
			const auto baseArray = readU32(a_rva + 12);
			if (!signature || *signature != 0 || !attributes ||
				(*attributes & 0xFFFFFFF0u) != 0 || !baseCount || *baseCount == 0 ||
				*baseCount > 4096 || !baseArray ||
				!inSegment(Segment::rdata, *baseArray) ||
				!readable(*baseArray,
					static_cast<std::size_t>(*baseCount) * sizeof(std::uint32_t))) {
				return false;
			}
			for (std::uint32_t slot = 0; slot < (a_validateAllBases ? *baseCount : 1); ++slot) {
				const auto descriptor = readU32(
					*baseArray + slot * static_cast<std::uint32_t>(sizeof(std::uint32_t)));
				if (!descriptor || !validMsvcBaseDescriptor(
						*descriptor,
						slot == 0 ? std::optional{ a_expectedType } : std::nullopt)) {
					return false;
				}
			}
			return true;
		};
		const auto validMsvcLocator = [&](std::uint32_t a_rva, bool a_validateAllBases) {
			if (a_rva % alignof(std::uint32_t) != 0 ||
				!inSegment(Segment::rdata, a_rva) || !readable(a_rva, 24)) {
				return false;
			}
			const auto signature = readU32(a_rva);
			const auto typeDescriptor = readU32(a_rva + 12);
			const auto hierarchy = readU32(a_rva + 16);
			const auto self = readU32(a_rva + 20);
			return signature && *signature == 1 && typeDescriptor && hierarchy && self &&
				*self == a_rva && validMsvcTypeDescriptor(*typeDescriptor) &&
				validMsvcHierarchy(*hierarchy, *typeDescriptor, a_validateAllBases);
		};
		const auto validMsvcVtable = [&](std::uint32_t a_rva, std::uint32_t a_locatorRVA) {
			if (a_rva < sizeof(std::uint64_t) || a_rva % alignof(std::uint64_t) != 0 ||
				!inSegment(Segment::rdata, a_rva) ||
				!readable(a_rva - sizeof(std::uint64_t), sizeof(std::uint64_t) * 2)) {
				return false;
			}
			const auto locator = readPointer(a_rva - sizeof(std::uint64_t));
			const auto firstSlot = readPointer(a_rva);
			const auto functionRVA = firstSlot ? pointerRVA(*firstSlot) : std::nullopt;
			return locator && *locator == a_module.base() + a_locatorRVA && functionRVA &&
				inSegment(Segment::text, *functionRVA);
		};

		const auto functionContaining = [&](std::uint32_t a_rva) -> std::optional<std::uint32_t> {
			struct RuntimeFunction
			{
				std::uint32_t begin;
				std::uint32_t end;
				std::uint32_t unwind;
			};
			const auto pdata = a_module.segment(Segment::pdata);
			const auto functions = std::span(
				pdata.pointer<const RuntimeFunction>(),
				pdata.size() / sizeof(RuntimeFunction));
			auto low = functions.begin();
			auto high = functions.end();
			while (low != high) {
				const auto middle = low + std::distance(low, high) / 2;
				if (middle->begin <= a_rva) {
					low = middle + 1;
				} else {
					high = middle;
				}
			}
			if (low == functions.begin()) {
				return std::nullopt;
			}
			--low;
			return low->begin <= a_rva && a_rva < low->end ?
			           std::optional<std::uint32_t>(low->begin) :
			           std::nullopt;
		};
		const auto findVTable = [&](std::uint32_t a_typeDescriptorRVA,
									std::uint32_t a_objectOffset) -> std::optional<std::uint32_t> {
			const auto rdata = a_module.segment(Segment::rdata);
			const auto rdataBytes = std::span(
				rdata.pointer<const std::uint8_t>(), rdata.size());
			constexpr std::size_t locatorSize = 24;
			if (rdata.offset() > UINT32_MAX || rdataBytes.size() < locatorSize ||
				!validMsvcTypeDescriptor(a_typeDescriptorRVA)) {
				return std::nullopt;
			}
			std::optional<std::uint32_t> vtable;
			for (std::size_t offset = 0; offset + locatorSize <= rdataBytes.size();
				 offset += alignof(std::uint32_t)) {
				std::uint32_t typeRVA{};
				std::memcpy(std::addressof(typeRVA), rdataBytes.data() + offset + 12,
					sizeof(typeRVA));
				if (typeRVA != a_typeDescriptorRVA ||
					offset > UINT32_MAX - static_cast<std::uint32_t>(rdata.offset())) {
					continue;
				}
				std::uint32_t signature{};
				std::uint32_t objectOffset{};
				std::uint32_t selfRVA{};
				std::memcpy(std::addressof(signature), rdataBytes.data() + offset, sizeof(signature));
				std::memcpy(std::addressof(objectOffset), rdataBytes.data() + offset + 4,
					sizeof(objectOffset));
				std::memcpy(std::addressof(selfRVA), rdataBytes.data() + offset + 20,
					sizeof(selfRVA));
				const auto locatorRVA = static_cast<std::uint32_t>(rdata.offset()) +
				                        static_cast<std::uint32_t>(offset);
				if (signature != 1 || objectOffset != a_objectOffset || selfRVA != locatorRVA ||
					!validMsvcLocator(locatorRVA, true)) {
					continue;
				}
				const auto locatorPointer = a_module.base() + locatorRVA;
				for (std::size_t sourceOffset = 0;
					 sourceOffset + sizeof(std::uint64_t) <= rdataBytes.size();
					 sourceOffset += alignof(std::uint64_t)) {
					std::uint64_t pointer{};
					std::memcpy(std::addressof(pointer), rdataBytes.data() + sourceOffset,
						sizeof(pointer));
					if (pointer != locatorPointer ||
						rdata.offset() > UINT32_MAX - sizeof(std::uint64_t) ||
						sourceOffset > UINT32_MAX - static_cast<std::uint32_t>(rdata.offset()) -
										   sizeof(std::uint64_t)) {
						continue;
					}
					const auto found = static_cast<std::uint32_t>(rdata.offset()) +
					                   static_cast<std::uint32_t>(sourceOffset) +
					                   static_cast<std::uint32_t>(sizeof(std::uint64_t));
					if (!validMsvcVtable(found, locatorRVA)) {
						continue;
					}
					if (vtable && *vtable != found) {
						a_context.record_failure(ResolveFailure::kPatternAmbiguous);
						return std::nullopt;
					}
					vtable = found;
				}
			}
			return vtable;
		};
		const auto resolveMsvcRttiNode = [&](std::uint32_t a_nameRVA,
											 std::uint32_t a_objectOffset,
											 std::uint8_t a_kind,
											 std::int32_t a_slotIndex) -> std::optional<std::uint32_t> {
			if (a_nameRVA < sizeof(std::uint64_t) * 2) {
				return std::nullopt;
			}
			const auto rdata = a_module.segment(Segment::rdata);
			const auto bytes = std::span(rdata.pointer<const std::uint8_t>(), rdata.size());
			if (rdata.offset() > UINT32_MAX || bytes.size() < 24) {
				return std::nullopt;
			}
			const auto typeDescriptorRVA = a_nameRVA -
			                               static_cast<std::uint32_t>(sizeof(std::uint64_t) * 2);
			if (!validMsvcTypeDescriptor(typeDescriptorRVA)) {
				return std::nullopt;
			}
			std::optional<std::uint32_t> locator;
			constexpr std::size_t locatorSize = 24;
			for (std::size_t offset = 0; offset + locatorSize <= bytes.size();
				 offset += alignof(std::uint32_t)) {
				std::uint32_t signature{};
				std::uint32_t currentOffset{};
				std::uint32_t typeRVA{};
				std::uint32_t selfRVA{};
				std::memcpy(std::addressof(signature), bytes.data() + offset, sizeof(signature));
				std::memcpy(std::addressof(currentOffset), bytes.data() + offset + 4,
					sizeof(currentOffset));
				std::memcpy(std::addressof(typeRVA), bytes.data() + offset + 12, sizeof(typeRVA));
				std::memcpy(std::addressof(selfRVA), bytes.data() + offset + 20, sizeof(selfRVA));
				if (offset > UINT32_MAX - static_cast<std::uint32_t>(rdata.offset())) {
					continue;
				}
				const auto found = static_cast<std::uint32_t>(rdata.offset()) +
				                   static_cast<std::uint32_t>(offset);
				if (signature != 1 || currentOffset != a_objectOffset ||
					typeRVA != typeDescriptorRVA || selfRVA != found ||
					!validMsvcLocator(found, true)) {
					continue;
				}
				if (locator && *locator != found) {
					a_context.record_failure(ResolveFailure::kPatternAmbiguous);
					return std::nullopt;
				}
				locator = found;
			}
			if (!locator || a_kind == 16) {
				return locator;
			}
			if (!readable(*locator, locatorSize)) {
				return std::nullopt;
			}
			std::uint32_t hierarchyRVA{};
			std::memcpy(std::addressof(hierarchyRVA),
				reinterpret_cast<const void*>(a_module.base() + *locator + 16),
				sizeof(hierarchyRVA));
			if (!readable(hierarchyRVA, 16)) {
				return std::nullopt;
			}
			if (a_kind == 17) {
				return hierarchyRVA;
			}
			std::uint32_t baseCount{};
			std::uint32_t baseArrayRVA{};
			std::memcpy(std::addressof(baseCount),
				reinterpret_cast<const void*>(a_module.base() + hierarchyRVA + 8),
				sizeof(baseCount));
			std::memcpy(std::addressof(baseArrayRVA),
				reinterpret_cast<const void*>(a_module.base() + hierarchyRVA + 12),
				sizeof(baseArrayRVA));
			if (baseCount == 0 || baseCount > 4096 ||
				!readable(baseArrayRVA, static_cast<std::size_t>(baseCount) * sizeof(std::uint32_t))) {
				return std::nullopt;
			}
			if (a_kind == 18) {
				return baseArrayRVA;
			}
			if (a_kind != 19 || a_slotIndex < 0 ||
				static_cast<std::uint32_t>(a_slotIndex) >= baseCount) {
				return std::nullopt;
			}
			const auto slotRVA = baseArrayRVA + static_cast<std::uint32_t>(a_slotIndex) *
			                                        static_cast<std::uint32_t>(sizeof(std::uint32_t));
			std::uint32_t descriptorRVA{};
			std::memcpy(std::addressof(descriptorRVA),
				reinterpret_cast<const void*>(a_module.base() + slotRVA),
				sizeof(descriptorRVA));
			return validMsvcBaseDescriptor(
				       descriptorRVA,
				       a_slotIndex == 0 ? std::optional{ typeDescriptorRVA } : std::nullopt) ?
			           std::optional<std::uint32_t>(descriptorRVA) :
			           std::nullopt;
		};

		std::vector<std::size_t> chain(a_candidate.fragments.size());
		std::optional<std::uint32_t> resolved;
		bool conflicting{};
		bool exceeded{};
		std::size_t completeChains{};
		const auto resolveChain = [&]() -> std::optional<std::uint32_t> {
			for (const auto& constraint : a_candidate.constraints) {
				if (constraint.kind != 1 || constraint.fragmentIndex >= chain.size()) {
					a_context.record_failure(ResolveFailure::kInvalidPattern);
					return std::nullopt;
				}
				const auto matchOffset = chain[constraint.fragmentIndex];
				if (scan.offset() > UINT32_MAX ||
					matchOffset > UINT32_MAX - static_cast<std::uint32_t>(scan.offset())) {
					return std::nullopt;
				}
				const auto matchRVA = static_cast<std::uint32_t>(scan.offset()) +
				                      static_cast<std::uint32_t>(matchOffset);
				if (constraint.operandOffset > UINT32_MAX - matchRVA) {
					return std::nullopt;
				}
				const auto operandRVA = matchRVA + constraint.operandOffset;
				if (!readable(operandRVA, sizeof(std::int32_t))) {
					return std::nullopt;
				}
				std::int32_t displacement{};
				std::memcpy(std::addressof(displacement),
					reinterpret_cast<const void*>(a_module.base() + operandRVA), sizeof(displacement));
				const auto actual = static_cast<std::int64_t>(operandRVA) + sizeof(displacement) +
				                    displacement + constraint.targetAdjustment;
				const auto expected = resolve_patterns(constraint.targetID, a_module, a_context);
				if (!expected || actual != *expected) {
					return std::nullopt;
				}
			}
			const auto anchorOffset = chain[a_candidate.resolveFragment];
			if (scan.offset() > UINT32_MAX ||
				anchorOffset > UINT32_MAX - static_cast<std::uint32_t>(scan.offset())) {
				return std::nullopt;
			}
			const auto anchorRVA = static_cast<std::uint32_t>(scan.offset()) +
			                       static_cast<std::uint32_t>(anchorOffset);
			if (a_candidate.resolveOffset > UINT32_MAX - anchorRVA) {
				return std::nullopt;
			}
			const auto operandRVA = anchorRVA + a_candidate.resolveOffset;
			std::int64_t value{};
			switch (a_candidate.resolution) {
			case 1:
				{
					const auto owner = functionContaining(static_cast<std::uint32_t>(operandRVA));
					if (!owner) {
						return std::nullopt;
					}
					value = *owner;
					break;
				}
			case 2:
			case 4:
				{
					if (!readable(operandRVA, sizeof(std::int32_t))) {
						return std::nullopt;
					}
					std::int32_t displacement{};
					std::memcpy(std::addressof(displacement),
						reinterpret_cast<const void*>(a_module.base() + operandRVA),
						sizeof(displacement));
					value = static_cast<std::int64_t>(operandRVA) + sizeof(displacement) + displacement;
					break;
				}
			case 5:
				{
					if (!readable(operandRVA, sizeof(std::int32_t))) {
						return std::nullopt;
					}
					std::int32_t displacement{};
					std::memcpy(std::addressof(displacement),
						reinterpret_cast<const void*>(a_module.base() + operandRVA), sizeof(displacement));
					const auto base = static_cast<std::int64_t>(operandRVA) + sizeof(displacement) + displacement;
					const auto slot = base + a_candidate.resultAdjustment;
					if (slot < 0 || !readable(static_cast<std::uint64_t>(slot), sizeof(std::uint64_t))) {
						return std::nullopt;
					}
					std::uint64_t pointer{};
					std::memcpy(std::addressof(pointer),
						reinterpret_cast<const void*>(a_module.base() + slot), sizeof(pointer));
					const auto pointerOffset = pointerRVA(pointer);
					if (!pointerOffset) {
						return std::nullopt;
					}
					return pointerOffset;
				}
			case 3:
			case 7:
				value = operandRVA;
				break;
			case 6:
				{
					if (!readable(operandRVA, sizeof(std::uint64_t))) {
						return std::nullopt;
					}
					std::uint64_t pointer{};
					std::memcpy(std::addressof(pointer),
						reinterpret_cast<const void*>(a_module.base() + operandRVA), sizeof(pointer));
					const auto pointerOffset = pointerRVA(pointer);
					if (!pointerOffset) {
						return std::nullopt;
					}
					value = *pointerOffset;
					break;
				}
			case 8:
				{
					const auto target = a_module.base() + anchorRVA;
					std::optional<std::uint32_t> source;
					for (std::size_t offset = 0; offset + sizeof(std::uint64_t) <= scanBytes.size();
						 offset += alignof(std::uint64_t)) {
						std::uint64_t pointer{};
						std::memcpy(std::addressof(pointer), scanBytes.data() + offset, sizeof(pointer));
						if (pointer != target || scan.offset() > UINT32_MAX ||
							offset > UINT32_MAX - static_cast<std::uint32_t>(scan.offset())) {
							continue;
						}
						const auto found = static_cast<std::uint32_t>(scan.offset()) +
						                   static_cast<std::uint32_t>(offset);
						if (source && *source != found) {
							a_context.record_failure(ResolveFailure::kPatternAmbiguous);
							return std::nullopt;
						}
						source = found;
					}
					if (!source) {
						return std::nullopt;
					}
					value = *source;
					break;
				}
			case 9:
				{
					if (anchorRVA < sizeof(std::uint64_t) * 2) {
						return std::nullopt;
					}
					const auto typeDescriptorRVA = anchorRVA - sizeof(std::uint64_t) * 2;
					const auto vtable = findVTable(
						static_cast<std::uint32_t>(typeDescriptorRVA), a_candidate.resolveOffset);
					if (!vtable) {
						return std::nullopt;
					}
					value = *vtable;
					break;
				}
			case 10:
				{
					if (anchorRVA < sizeof(std::uint64_t) * 2) {
						return std::nullopt;
					}
					const auto typeDescriptorRVA = anchorRVA - sizeof(std::uint64_t) * 2;
					const auto vtable = findVTable(
						static_cast<std::uint32_t>(typeDescriptorRVA), a_candidate.resolveOffset);
					if (!vtable || a_candidate.resultAdjustment < 0 || a_candidate.resultAdjustment >= 16) {
						return std::nullopt;
					}
					const auto slotRVA = *vtable + sizeof(std::uint64_t) *
					                                   static_cast<std::uint32_t>(a_candidate.resultAdjustment);
					if (!readable(slotRVA, sizeof(std::uint64_t))) {
						return std::nullopt;
					}
					std::uint64_t functionPointer{};
					std::memcpy(std::addressof(functionPointer),
						reinterpret_cast<const void*>(a_module.base() + slotRVA),
						sizeof(functionPointer));
					auto functionRVA = pointerRVA(functionPointer);
					if (!functionRVA) {
						return std::nullopt;
					}
					const auto data = a_module.segment(Segment::data);
					std::unordered_set<std::uint32_t> followed;
					std::optional<std::uint32_t> niRtti;
					for (std::size_t depth = 0;
						 depth < 4 && followed.insert(*functionRVA).second && !niRtti; ++depth) {
						constexpr std::size_t probeSize = 96;
						if (!readable(*functionRVA, probeSize)) {
							return std::nullopt;
						}
						const auto* code = reinterpret_cast<const std::uint8_t*>(a_module.base() + *functionRVA);
						std::optional<std::uint32_t> jumpTarget;
						for (std::size_t offset = 0; offset + 7 <= probeSize; ++offset) {
							if (offset == 0 && code[offset] == 0xE9) {
								std::int32_t displacement{};
								std::memcpy(std::addressof(displacement), code + 1, sizeof(displacement));
								const auto target = static_cast<std::int64_t>(*functionRVA) + 5 + displacement;
								if (target >= 0 && static_cast<std::uint64_t>(target) < imageSize) {
									jumpTarget = static_cast<std::uint32_t>(target);
								}
							}
							if (code[offset] == 0x48 && code[offset + 1] == 0x8D && code[offset + 2] == 0x05) {
								std::int32_t displacement{};
								std::memcpy(std::addressof(displacement), code + offset + 3, sizeof(displacement));
								const auto target = static_cast<std::int64_t>(*functionRVA) + offset + 7 + displacement;
								if (target >= data.offset() &&
									static_cast<std::uint64_t>(target - data.offset()) < data.size()) {
									niRtti = static_cast<std::uint32_t>(target);
									break;
								}
							}
							if (code[offset] == 0xC3) {
								break;
							}
						}
						if (!jumpTarget) {
							break;
						}
						functionRVA = jumpTarget;
					}
					if (!niRtti) {
						return std::nullopt;
					}
					return niRtti;
				}
			case 11:
				{
					struct RuntimeFunction
					{
						std::uint32_t begin;
						std::uint32_t end;
						std::uint32_t unwind;
					};
					const auto pdata = a_module.segment(Segment::pdata);
					const auto functions = std::span(
						pdata.pointer<const RuntimeFunction>(), pdata.size() / sizeof(RuntimeFunction));
					auto owner = std::upper_bound(functions.begin(), functions.end(), operandRVA,
						[](const auto a_rva, const auto& a_function) {
							return a_rva < a_function.begin;
						});
					if (owner == functions.begin()) {
						return std::nullopt;
					}
					--owner;
					if (operandRVA < owner->begin || operandRVA >= owner->end) {
						return std::nullopt;
					}
					const auto index = static_cast<std::int64_t>(std::distance(functions.begin(), owner));
					const auto targetIndex = index + a_candidate.resultAdjustment;
					if (targetIndex < 0 || static_cast<std::uint64_t>(targetIndex) >= functions.size()) {
						return std::nullopt;
					}
					return functions[static_cast<std::size_t>(targetIndex)].begin;
				}
			case 15:
				{
					struct RuntimeFunction
					{
						std::uint32_t begin;
						std::uint32_t end;
						std::uint32_t unwind;
					};
					const auto pdata = a_module.segment(Segment::pdata);
					const auto functions = std::span(
						pdata.pointer<const RuntimeFunction>(), pdata.size() / sizeof(RuntimeFunction));
					auto anchor = std::upper_bound(functions.begin(), functions.end(), anchorRVA,
						[](const auto a_rva, const auto& a_function) {
							return a_rva < a_function.begin;
						});
					if (anchor == functions.begin()) {
						return std::nullopt;
					}
					--anchor;
					if (anchorRVA < anchor->begin || anchorRVA >= anchor->end) {
						return std::nullopt;
					}
					const auto anchorIndex = static_cast<std::int64_t>(
						std::distance(functions.begin(), anchor));
					const auto targetIndex = anchorIndex + a_candidate.resultAdjustment;
					if (targetIndex < 0 || static_cast<std::uint64_t>(targetIndex) >= functions.size()) {
						return std::nullopt;
					}
					const auto& targetFunction = functions[static_cast<std::size_t>(targetIndex)];
					if (a_candidate.resolveOffset > UINT32_MAX - targetFunction.begin) {
						return std::nullopt;
					}
					const auto displacementRVA = targetFunction.begin + a_candidate.resolveOffset;
					if (!readable(displacementRVA, sizeof(std::int32_t))) {
						return std::nullopt;
					}
					std::int32_t displacement{};
					std::memcpy(std::addressof(displacement),
						reinterpret_cast<const void*>(a_module.base() + displacementRVA), sizeof(displacement));
					const auto result = static_cast<std::int64_t>(displacementRVA) +
					                    sizeof(displacement) +
					                    ((a_candidate.flags & CANDIDATE_RIP_TRAILING_BYTES_MASK) >>
											CANDIDATE_RIP_TRAILING_BYTES_SHIFT) +
					                    displacement;
					if (result < 0 || !readable(static_cast<std::uint64_t>(result), 1)) {
						return std::nullopt;
					}
					return static_cast<std::uint32_t>(result);
				}
			case 16:
			case 17:
			case 18:
			case 19:
				return resolveMsvcRttiNode(
					anchorRVA,
					a_candidate.resolveOffset,
					a_candidate.resolution,
					a_candidate.resultAdjustment);
			case 12:
				{
					const auto data = a_module.segment(Segment::data);
					const auto dataBytes = std::span(
						data.pointer<const std::uint8_t>(), data.size());
					const auto pointerTarget = a_module.base() + anchorRVA;
					std::optional<std::uint32_t> object;
					for (std::size_t offset = 0;
						 offset + sizeof(std::uint64_t) <= dataBytes.size();
						 offset += alignof(std::uint64_t)) {
						std::uint64_t pointer{};
						std::memcpy(std::addressof(pointer), dataBytes.data() + offset, sizeof(pointer));
						if (pointer != pointerTarget || data.offset() > UINT32_MAX ||
							offset > UINT32_MAX - static_cast<std::uint32_t>(data.offset())) {
							continue;
						}
						const auto source = static_cast<std::uint32_t>(data.offset()) +
						                    static_cast<std::uint32_t>(offset);
						if (source < a_candidate.resolveOffset) {
							continue;
						}
						const auto found = source - a_candidate.resolveOffset;
						if (object && *object != found) {
							a_context.record_failure(ResolveFailure::kPatternAmbiguous);
							return std::nullopt;
						}
						object = found;
					}
					return object;
				}
			case 13:
			case RESOLUTION_IMPORT_THUNK_FROM_NAME:
				{
					if (anchorRVA < 2) {
						return std::nullopt;
					}
					auto idata = a_module.segment(Segment::idata);
					if (idata.size() == 0) {
						idata = a_module.segment(Segment::rdata);
					}
					const auto idataBytes = std::span(
						idata.pointer<const std::uint8_t>(), idata.size());
					const auto importByNameRVA = anchorRVA - 2;
					std::int32_t peOffset{};
					std::memcpy(std::addressof(peOffset),
						reinterpret_cast<const void*>(a_module.base() + 0x3C), sizeof(peOffset));
					if (peOffset < 0 || static_cast<std::uint64_t>(peOffset) + 24 + 128 > imageSize) {
						return std::nullopt;
					}
					const auto optionalHeader = a_module.base() + static_cast<std::uint32_t>(peOffset) + 24;
					std::uint16_t optionalMagic{};
					std::memcpy(std::addressof(optionalMagic),
						reinterpret_cast<const void*>(optionalHeader), sizeof(optionalMagic));
					if (optionalMagic != 0x20B) {
						return std::nullopt;
					}
					std::uint32_t importDirectoryRVA{};
					std::uint32_t importDirectorySize{};
					std::memcpy(std::addressof(importDirectoryRVA),
						reinterpret_cast<const void*>(optionalHeader + 112 + 8), sizeof(importDirectoryRVA));
					std::memcpy(std::addressof(importDirectorySize),
						reinterpret_cast<const void*>(optionalHeader + 112 + 12), sizeof(importDirectorySize));
					if (!importDirectoryRVA || !importDirectorySize ||
						importDirectoryRVA < idata.offset() ||
						importDirectoryRVA - idata.offset() >= idata.size()) {
						return std::nullopt;
					}
					const auto directoryBegin = static_cast<std::size_t>(
						importDirectoryRVA - idata.offset());
					const auto directoryEnd = static_cast<std::size_t>(std::min<std::uint64_t>(
						idataBytes.size(), static_cast<std::uint64_t>(directoryBegin) + importDirectorySize));
					std::optional<std::uint32_t> iat;
					for (std::size_t offset = directoryBegin;
						 offset + 20 <= directoryEnd; offset += 20) {
						std::uint32_t originalFirstThunk{};
						std::uint32_t dllName{};
						std::uint32_t firstThunk{};
						std::memcpy(std::addressof(originalFirstThunk), idataBytes.data() + offset,
							sizeof(originalFirstThunk));
						std::memcpy(std::addressof(dllName), idataBytes.data() + offset + 12,
							sizeof(dllName));
						std::memcpy(std::addressof(firstThunk), idataBytes.data() + offset + 16,
							sizeof(firstThunk));
						const auto lookup = originalFirstThunk ? originalFirstThunk : firstThunk;
						if (!lookup || !dllName || !firstThunk || lookup < idata.offset() ||
							firstThunk < idata.offset() || dllName < idata.offset() ||
							lookup - idata.offset() >= idata.size() ||
							firstThunk - idata.offset() >= idata.size() ||
							dllName - idata.offset() >= idata.size()) {
							continue;
						}
						for (std::size_t index = 0; index < 65536; ++index) {
							const auto lookupRVA = static_cast<std::uint64_t>(lookup) +
							                       index * sizeof(std::uint64_t);
							if (!readable(lookupRVA, sizeof(std::uint64_t))) {
								break;
							}
							std::uint64_t thunkValue{};
							std::memcpy(std::addressof(thunkValue),
								reinterpret_cast<const void*>(a_module.base() + lookupRVA), sizeof(thunkValue));
							if (thunkValue == 0) {
								break;
							}
							if (thunkValue != importByNameRVA) {
								continue;
							}
							const auto found64 = static_cast<std::uint64_t>(firstThunk) +
							                     index * sizeof(std::uint64_t);
							if (found64 > UINT32_MAX) {
								return std::nullopt;
							}
							const auto found = static_cast<std::uint32_t>(found64);
							if (iat && *iat != found) {
								a_context.record_failure(ResolveFailure::kPatternAmbiguous);
								return std::nullopt;
							}
							iat = found;
						}
					}
					if (a_candidate.resolution == 13 || !iat) {
						return iat;
					}
					const auto text = a_module.segment(Segment::text);
					const auto textBytes = std::span(
						text.pointer<const std::uint8_t>(), text.size());
					std::optional<std::uint32_t> thunk;
					constexpr std::size_t thunkLength = 6;
					for (std::size_t offset = 0; offset + thunkLength <= textBytes.size(); ++offset) {
						if (textBytes[offset] != 0xFF || textBytes[offset + 1] != 0x25) {
							continue;
						}
						std::int32_t displacement{};
						std::memcpy(std::addressof(displacement),
							textBytes.data() + offset + 2, sizeof(displacement));
						const auto thunkRVA64 = static_cast<std::uint64_t>(text.offset()) + offset;
						const auto target = static_cast<std::int64_t>(thunkRVA64 + thunkLength) +
							displacement;
						if (target != *iat || thunkRVA64 > UINT32_MAX) {
							continue;
						}
						const auto found = static_cast<std::uint32_t>(thunkRVA64);
						if (thunk && *thunk != found) {
							a_context.record_failure(ResolveFailure::kPatternAmbiguous);
							return std::nullopt;
						}
						thunk = found;
					}
					return thunk;
				}
			case 14:
				{
					if (anchorRVA < sizeof(std::uint64_t) * 2) {
						return std::nullopt;
					}
					const auto text = a_module.segment(Segment::text);
					const auto data = a_module.segment(Segment::data);
					const auto typeDescriptorRVA = anchorRVA - sizeof(std::uint64_t) * 2;
					const auto vtable = findVTable(
						static_cast<std::uint32_t>(typeDescriptorRVA), a_candidate.resolveOffset);
					if (!vtable) {
						return std::nullopt;
					}
					struct RuntimeFunction
					{
						std::uint32_t begin;
						std::uint32_t end;
						std::uint32_t unwind;
					};
					const auto pdata = a_module.segment(Segment::pdata);
					const auto functions = std::span(
						pdata.pointer<const RuntimeFunction>(), pdata.size() / sizeof(RuntimeFunction));
					const auto functionContainingRVA = [&](std::uint32_t a_rva)
						-> const RuntimeFunction* {
						auto low = functions.begin();
						auto high = functions.end();
						while (low != high) {
							const auto middle = low + std::distance(low, high) / 2;
							if (middle->begin <= a_rva) {
								low = middle + 1;
							} else {
								high = middle;
							}
						}
						if (low == functions.begin()) {
							return nullptr;
						}
						--low;
						return low->begin <= a_rva && a_rva < low->end ? std::addressof(*low) : nullptr;
					};
					const auto textBytes = std::span(text.pointer<const std::uint8_t>(), text.size());
					std::optional<std::uint32_t> singleton;
					std::unordered_set<std::uint32_t> owners;
					std::optional<std::uint32_t> encodedInstructionOffset;
					std::uint32_t encodedInstructionLength{};
					std::uint32_t encodedOperandOffset{};
					if (a_candidate.resultAdjustment > 0) {
						const auto packed = static_cast<std::uint32_t>(a_candidate.resultAdjustment);
						const auto offsetPlusOne = packed >> 16;
						encodedInstructionLength = (packed >> 8) & 0xFF;
						encodedOperandOffset = packed & 0xFF;
						if (offsetPlusOne == 0 || encodedInstructionLength == 0 ||
							encodedOperandOffset + sizeof(std::int32_t) > encodedInstructionLength) {
							return std::nullopt;
						}
						encodedInstructionOffset = offsetPlusOne - 1;
					}
					for (std::size_t offset = 0; offset + 7 <= textBytes.size(); ++offset) {
						if (textBytes[offset] != 0x48 || textBytes[offset + 1] != 0x8D ||
							(textBytes[offset + 2] & 0xC7) != 0x05) {
							continue;
						}
						std::int32_t displacement{};
						std::memcpy(std::addressof(displacement), textBytes.data() + offset + 3,
							sizeof(displacement));
						const auto textOffset = static_cast<std::uint64_t>(text.offset());
						if (textOffset > UINT32_MAX || offset > UINT32_MAX - textOffset) {
							break;
						}
						const auto instructionRVA =
							static_cast<std::uint32_t>(textOffset + offset);
						const auto target = static_cast<std::int64_t>(instructionRVA) + 7 + displacement;
						if (target != *vtable) {
							continue;
						}
						const auto* owner = functionContainingRVA(instructionRVA);
						if (!owner || !owners.insert(owner->begin).second || owner->end <= owner->begin ||
							!readable(owner->begin, owner->end - owner->begin)) {
							continue;
						}
						if (encodedInstructionOffset) {
							if (*encodedInstructionOffset + encodedInstructionLength >
								owner->end - owner->begin) {
								continue;
							}
							const auto instruction = owner->begin + *encodedInstructionOffset;
							const auto operand = instruction + encodedOperandOffset;
							if (!readable(operand, sizeof(std::int32_t))) {
								continue;
							}
							std::int32_t storeDisplacement{};
							std::memcpy(std::addressof(storeDisplacement),
								reinterpret_cast<const void*>(a_module.base() + operand), sizeof(storeDisplacement));
							const auto storeTarget = static_cast<std::int64_t>(instruction) +
							                         encodedInstructionLength + storeDisplacement;
							if (storeTarget < static_cast<std::int64_t>(data.offset()) ||
								static_cast<std::uint64_t>(storeTarget - data.offset()) >= data.size()) {
								continue;
							}
							const auto found = static_cast<std::uint32_t>(storeTarget);
							if (singleton && *singleton != found) {
								a_context.record_failure(ResolveFailure::kPatternAmbiguous);
								return std::nullopt;
							}
							singleton = found;
							continue;
						}
						const auto* functionBytes = reinterpret_cast<const std::uint8_t*>(
							a_module.base() + owner->begin);
						const auto functionSize = static_cast<std::size_t>(owner->end - owner->begin);
						for (std::size_t item = 0; item + 7 <= functionSize; ++item) {
							if (functionBytes[item] != 0x48 || functionBytes[item + 1] != 0x89 ||
								(functionBytes[item + 2] & 0xC7) != 0x05) {
								continue;
							}
							std::int32_t storeDisplacement{};
							std::memcpy(std::addressof(storeDisplacement), functionBytes + item + 3,
								sizeof(storeDisplacement));
							const auto storeRVA = owner->begin + static_cast<std::uint32_t>(item);
							const auto storeTarget = static_cast<std::int64_t>(storeRVA) + 7 +
							                         storeDisplacement;
							if (storeTarget < static_cast<std::int64_t>(data.offset()) ||
								static_cast<std::uint64_t>(storeTarget - data.offset()) >= data.size()) {
								continue;
							}
							const auto found = static_cast<std::uint32_t>(storeTarget);
							if (singleton && *singleton != found) {
								a_context.record_failure(ResolveFailure::kPatternAmbiguous);
								return std::nullopt;
							}
							singleton = found;
						}
					}
					return singleton;
				}
			default:
				a_context.record_failure(ResolveFailure::kInvalidPattern);
				return std::nullopt;
			}
			value += a_candidate.resultAdjustment;
			if (value < 0 || static_cast<std::uint64_t>(value) >= imageSize) {
				return std::nullopt;
			}
			return static_cast<std::uint32_t>(value);
		};

		std::function<void(std::size_t)> extend;
		extend = [&](std::size_t a_fragmentIndex) {
			if (conflicting || exceeded) {
				return;
			}
			if (a_fragmentIndex == a_candidate.fragments.size()) {
				if (++completeChains > 16384) {
					exceeded = true;
					return;
				}
				if (const auto value = resolveChain(); value && resultAllowed(*value)) {
					if (!resolved) {
						resolved = value;
					} else if (*resolved != *value) {
						conflicting = true;
					}
				}
				return;
			}
			const auto& previous = a_candidate.fragments[a_fragmentIndex - 1];
			const auto& current = a_candidate.fragments[a_fragmentIndex];
			std::uint64_t begin{};
			std::uint64_t maxEnd{};
			if ((current.flags & FRAGMENT_REVERSE) != 0) {
				const auto previousStart = chain[a_fragmentIndex - 1];
				if (previousStart < static_cast<std::uint64_t>(current.minGap) +
										current.pattern.values.size()) {
					return;
				}
				const auto maximumStart = previousStart - current.minGap - current.pattern.values.size();
				begin = previousStart >= static_cast<std::uint64_t>(current.maxGap) +
				                             current.pattern.values.size() ?
				            previousStart - current.maxGap - current.pattern.values.size() :
				            0;
				maxEnd = maximumStart + current.pattern.values.size();
			} else {
				const auto previousEnd = chain[a_fragmentIndex - 1] + previous.pattern.values.size();
				begin = static_cast<std::uint64_t>(previousEnd) + current.minGap;
				maxEnd = static_cast<std::uint64_t>(previousEnd) + current.maxGap +
				         current.pattern.values.size();
			}
			if (begin > scanBytes.size()) {
				return;
			}
			const auto matches = findMatches(
				current.pattern,
				static_cast<std::size_t>(begin),
				static_cast<std::size_t>(std::min<std::uint64_t>(scanBytes.size(), maxEnd)),
				MAX_INTERMEDIATE_MATCHES + 1);
			if (matches.size() > MAX_INTERMEDIATE_MATCHES) {
				exceeded = true;
				return;
			}
			for (const auto match : matches) {
				chain[a_fragmentIndex] = match;
				extend(a_fragmentIndex + 1);
				if (conflicting || exceeded) {
					return;
				}
			}
		};

		for (const auto first : firstMatches) {
			chain[0] = first;
			extend(1);
			if (conflicting || exceeded) {
				a_context.record_failure(ResolveFailure::kPatternAmbiguous);
				return std::nullopt;
			}
		}
		if (!resolved) {
			a_context.record_failure(ResolveFailure::kPatternNotFound);
		}
		return resolved;
	}

	std::optional<std::uint32_t> RuntimeDatabase::resolve_patterns(
		std::uint64_t a_id,
		const Module& a_module,
		ResolveContext& a_context) const
	{
		if (const auto cached = a_context.cache.find(a_id); cached != a_context.cache.end()) {
			return cached->second;
		}
		if (a_context.resolving.size() >= MAX_RESOLUTION_DEPTH) {
			a_context.record_failure(ResolveFailure::kInvalidPattern);
			return std::nullopt;
		}
		if (!a_context.resolving.insert(a_id).second) {
			a_context.record_failure(ResolveFailure::kInvalidPattern);
			return std::nullopt;
		}
		struct EraseOnExit
		{
			std::unordered_set<std::uint64_t>& values;
			std::uint64_t id;
			~EraseOnExit() { values.erase(id); }
		} erase{ a_context.resolving, a_id };
		const auto* record = find(a_id, a_context.version);
		if (!record) {
			a_context.record_failure(ResolveFailure::kInvalidPattern);
			return std::nullopt;
		}
		const auto candidateTier = [&](const Candidate& a_candidate) {
			if (a_candidate.version == VersionKey{}) {
				return 2u;
			}
			if ((a_candidate.flags & CANDIDATE_VERSION_MAJOR_MINOR) == 0) {
				return a_candidate.version == a_context.version ? 0u : 3u;
			}
			return version_scope_matches(
				a_candidate.version.parts, a_context.version.parts, true) ? 1u : 3u;
		};
		std::size_t applicableCandidates{};
		for (unsigned tier = 0; tier < 3; ++tier) {
			const auto tierCandidates = static_cast<std::size_t>(std::count_if(
				record->candidates.begin(), record->candidates.end(),
				[&](const Candidate& a_candidate) { return candidateTier(a_candidate) == tier; }));
			if (tierCandidates == 0) {
				continue;
			}
			applicableCandidates += tierCandidates;
			const auto unavailableCandidates = static_cast<std::size_t>(std::count_if(
				record->candidates.begin(), record->candidates.end(), [&](const Candidate& a_candidate) {
					return candidateTier(a_candidate) == tier &&
					       is_unavailable_resolution(a_candidate.resolution);
				}));
			if (unavailableCandidates != 0) {
				const auto malformed = unavailableCandidates != tierCandidates ||
					std::any_of(record->candidates.begin(), record->candidates.end(),
						[&](const Candidate& a_candidate) {
							return candidateTier(a_candidate) == tier &&
							       (!a_candidate.fragments.empty() ||
								   !a_candidate.constraints.empty() ||
								   a_candidate.resolveFragment != 0 ||
								   a_candidate.resolveOffset != 0 ||
								   a_candidate.resultAdjustment != 0 ||
								   a_candidate.minFixedBytes != 0);
						});
				a_context.record_failure(malformed ? ResolveFailure::kInvalidPattern :
				                                           ResolveFailure::kRuntimeUnavailable);
				return std::nullopt;
			}
			for (unsigned fallbackPass = 0; fallbackPass < 2; ++fallbackPass) {
				std::optional<std::uint32_t> resolved;
				for (const auto& candidate : record->candidates) {
					if (candidateTier(candidate) != tier ||
						is_dependent_resolution(candidate.resolution) != (fallbackPass != 0)) {
						continue;
					}
					if (const auto candidateResult = resolve_candidate(candidate, a_module, a_context)) {
						if (!validate_record_result(*record, *candidateResult, a_module)) {
							a_context.record_failure(ResolveFailure::kSemanticValidationFailed);
							continue;
						}
						if (resolved && *resolved != *candidateResult) {
							a_context.record_failure(ResolveFailure::kPatternAmbiguous);
							return std::nullopt;
						}
						resolved = candidateResult;
					}
				}
				if (resolved) {
					a_context.cache[record->id] = *resolved;
					a_context.cache[a_id] = *resolved;
					return resolved;
				}
			}
		}
		if (applicableCandidates == 0) {
			a_context.record_failure(ResolveFailure::kNoCompatiblePattern);
		} else if (a_context.failure == ResolveFailure::kNone) {
			a_context.record_failure(ResolveFailure::kPatternNotFound);
		}
		return std::nullopt;
	}

	std::optional<std::size_t> RuntimeDatabase::resolve(
		std::uint64_t a_id,
		const Version& a_version,
		const Module& a_module,
		ResolveMode a_mode,
		ResolveSource* a_source) const
	{
		const auto result = resolve_detailed(a_id, a_version, a_module, a_mode);
		if (a_source) {
			*a_source = result.source;
		}
		return result.rva;
	}

	RuntimeDatabase::DetailedResolveResult RuntimeDatabase::resolve_detailed(
		std::uint64_t a_id,
		const Version& a_version,
		const Module& a_module,
		ResolveMode a_mode) const
	{
		const std::shared_lock stateLock(_batchStateLock);
		VersionKey version;
		for (std::size_t i = 0; i < version.parts.size(); ++i) {
			version.parts[i] = a_version[i];
		}
		const auto* record = find(a_id, version);
		if (!record) {
			return { std::nullopt, ResolveSource::kNone, ResolveFailure::kUnknownID };
		}
		const auto useLocalCache = a_mode == ResolveMode::kNormal;
		const auto useKnownRVA =
			a_mode == ResolveMode::kNormal || a_mode == ResolveMode::kKnownOnly;
		const auto useSharedCache = a_mode != ResolveMode::kPatternsOnly;
		if (useLocalCache) {
			const std::scoped_lock lock(_cacheLock);
			if (const auto cached = _cache.find(record->id); cached != _cache.end()) {
				if (!validate_record_result(*record, cached->second, a_module)) {
					return { std::nullopt, ResolveSource::kNone,
						ResolveFailure::kSemanticValidationFailed };
				}
				return { cached->second, ResolveSource::kLocalCache, ResolveFailure::kNone };
			}
		}
		if (useKnownRVA) {
			if (const auto known = std::find_if(record->knownRVAs.begin(), record->knownRVAs.end(),
					[&](const KnownRVA& a_value) { return a_value.version == version; });
				known != record->knownRVAs.end()) {
				if (!validate_record_result(*record, known->rva, a_module)) {
					return { std::nullopt, ResolveSource::kNone,
						ResolveFailure::kSemanticValidationFailed };
				}
				const std::scoped_lock lock(_cacheLock);
				_cache[record->id] = known->rva;
				return { known->rva, ResolveSource::kKnownRVA, ResolveFailure::kNone };
			}
		}
		if (a_mode == ResolveMode::kKnownOnly) {
			return { std::nullopt, ResolveSource::kNone, ResolveFailure::kNoCompatiblePattern };
		}
		const auto scan = [&]() -> PatternResolveResult {
			ResolveContext context;
			context.version = version;
			const auto resolved = resolve_patterns(record->id, a_module, context);
			if (!resolved && context.failure == ResolveFailure::kNone) {
				context.record_failure(ResolveFailure::kPatternNotFound);
			}
			return { resolved, context.failure };
		};

		PatternResolveResult resolved;
		bool sharedHit{};
		if (useSharedCache) {
			SharedCache* shared{};
			{
				const std::scoped_lock lock(_cacheLock);
				if (!_sharedCache) {
					_sharedCache = std::make_unique<SharedCache>(a_version, _payloadCRC);
				}
				shared = _sharedCache.get();
			}
			const auto result = shared->resolve(record->id, scan);
			resolved = { result.rva, result.failure };
			sharedHit = result.cached;
		} else {
			resolved = scan();
		}
		if (!resolved.rva) {
			return {
				std::nullopt,
				sharedHit ? ResolveSource::kSharedCache : ResolveSource::kPattern,
				resolved.failure == ResolveFailure::kNone ?
					ResolveFailure::kPatternNotFound :
					resolved.failure
			};
		}
		if (!validate_record_result(*record, *resolved.rva, a_module)) {
			return { std::nullopt, ResolveSource::kNone,
				ResolveFailure::kSemanticValidationFailed };
		}
		if (useLocalCache) {
			const std::scoped_lock lock(_cacheLock);
			_cache[record->id] = *resolved.rva;
		}
		return {
			*resolved.rva,
			sharedHit ? ResolveSource::kSharedCache : ResolveSource::kPattern,
			ResolveFailure::kNone
		};
	}

	std::vector<RuntimeDatabase::BulkResolveResult> RuntimeDatabase::resolve_all_patterns(
		const Version& a_version,
		const Module& a_module) const
	{
		const std::scoped_lock operationLock(_batchOperationLock);
		VersionKey version;
		for (std::size_t index = 0; index < version.parts.size(); ++index) {
			version.parts[index] = a_version[index];
		}
		const auto ids = pattern_ids(a_version);
		std::vector<BulkResolveResult> results(ids.size());
		{
			const std::unique_lock stateLock(_batchStateLock);
			prepare_batch(version, a_module);
		}
		try {
			const auto workerCount = bulk_worker_count(ids.size(), 4096);
			std::vector<std::future<void>> workers;
			workers.reserve(workerCount);
			for (std::size_t worker = 0; worker < workerCount; ++worker) {
				workers.push_back(std::async(std::launch::async, [&, worker] {
					for (auto index = worker; index < ids.size(); index += workerCount) {
						results[index] = {
							ids[index],
							resolve_detailed(
								ids[index],
								a_version,
								a_module,
								ResolveMode::kPatternsOnly)
						};
					}
				}));
			}
			for (auto& worker : workers) {
				worker.get();
			}
		} catch (...) {
			{
				const std::unique_lock stateLock(_batchStateLock);
				clear_batch();
			}
			throw;
		}
		{
			const std::unique_lock stateLock(_batchStateLock);
			clear_batch();
		}
		return results;
	}

	std::vector<std::pair<std::uint64_t, std::uint64_t>> RuntimeDatabase::known_mappings(
		const Version& a_version) const
	{
		VersionKey version;
		for (std::size_t index = 0; index < version.parts.size(); ++index) {
			version.parts[index] = a_version[index];
		}
		std::vector<std::uint64_t> ids;
		ids.reserve(record_count() + _aliasIndex.size());
		if (_mapped) {
			for (std::size_t index = 0; index < _mapped->recordCount; ++index) {
				ids.push_back(read_le<std::uint64_t>(
					_mapped->bytes, _mapped->recordsOffset + index * RECORD_SIZE));
			}
		} else {
			for (const auto& record : _records) {
				ids.push_back(record.id);
			}
		}
		for (const auto& alias : _aliasIndex) {
			const auto scoped = (alias.flags & ALIAS_VERSION_MAJOR_MINOR) != 0;
			if (version_scope_matches(alias.version.parts, version.parts, scoped)) {
				ids.push_back(alias.id);
			}
		}
		std::ranges::sort(ids);
		ids.erase(std::unique(ids.begin(), ids.end()), ids.end());

		std::vector<std::pair<std::uint64_t, std::uint64_t>> mappings;
		mappings.reserve(ids.size());
		for (const auto id : ids) {
			const auto* record = find(id, version);
			if (!record) {
				continue;
			}
			const auto known = std::find_if(record->knownRVAs.begin(), record->knownRVAs.end(),
				[&](const KnownRVA& a_value) { return a_value.version == version; });
			if (known != record->knownRVAs.end()) {
				mappings.emplace_back(id, known->rva);
			}
		}
		return mappings;
	}

	std::vector<std::pair<std::uint64_t, std::uint64_t>> RuntimeDatabase::pattern_mappings(
		const Version& a_version) const
	{
		VersionKey version;
		for (std::size_t i = 0; i < version.parts.size(); ++i) {
			version.parts[i] = a_version[i];
		}
		const auto ids = pattern_ids(a_version);
		std::vector<std::pair<std::uint64_t, std::uint64_t>> mappings;
		mappings.reserve(ids.size());
		for (const auto id : ids) {
			const auto* record = find(id, version);
			if (!record || record->candidates.empty()) {
				continue;
			}
			if (const auto known = std::find_if(record->knownRVAs.begin(), record->knownRVAs.end(),
					[&](const KnownRVA& a_value) { return a_value.version == version; });
				known != record->knownRVAs.end()) {
				mappings.emplace_back(id, known->rva);
			}
		}
		return mappings;
	}

	std::vector<std::uint64_t> RuntimeDatabase::pattern_ids(const Version& a_version) const
	{
		VersionKey version;
		for (std::size_t i = 0; i < version.parts.size(); ++i) {
			version.parts[i] = a_version[i];
		}
		std::vector<std::uint64_t> ids;
		if (_mapped) {
			ids.reserve(_mapped->recordCount);
			for (std::size_t recordIndex = 0; recordIndex < _mapped->recordCount; ++recordIndex) {
				const auto recordOffset = _mapped->recordsOffset + recordIndex * RECORD_SIZE;
				if (read_le<std::uint32_t>(_mapped->bytes, recordOffset + 28) == 0) {
					continue;
				}
				ids.push_back(read_le<std::uint64_t>(_mapped->bytes, recordOffset));
				const auto firstAlias = read_le<std::uint32_t>(_mapped->bytes, recordOffset + 8);
				const auto aliasCount = read_le<std::uint32_t>(_mapped->bytes, recordOffset + 12);
				for (std::uint32_t aliasIndex = 0; aliasIndex < aliasCount; ++aliasIndex) {
					const auto offset = _mapped->aliasesOffset +
					                    static_cast<std::size_t>(firstAlias + aliasIndex) * ALIAS_SIZE;
					if (version_scope_matches(read_version(_mapped->bytes, offset + 8), version.parts,
							(read_le<std::uint32_t>(_mapped->bytes, offset + 20) &
								ALIAS_VERSION_MAJOR_MINOR) != 0)) {
						ids.push_back(read_le<std::uint64_t>(_mapped->bytes, offset));
					}
				}
			}
		} else {
			ids.reserve(_records.size());
			for (const auto& record : _records) {
				if (record.candidates.empty()) {
					continue;
				}
				ids.push_back(record.id);
				for (const auto& alias : record.aliases) {
					if (version_scope_matches(alias.version.parts, version.parts,
							(alias.flags & ALIAS_VERSION_MAJOR_MINOR) != 0)) {
						ids.push_back(alias.id);
					}
				}
			}
		}
		std::sort(ids.begin(), ids.end());
		ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
		return ids;
	}
}
