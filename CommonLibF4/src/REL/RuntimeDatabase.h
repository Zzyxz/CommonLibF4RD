#pragma once

#include "REL/Relocation.h"

#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>

namespace REL
{
	class RuntimeDatabase
	{
	public:
		enum class ResolveMode : std::uint8_t
		{
			kNormal,
			kKnownOnly,
			kPatternsOnly,
			kPatternsShared
		};

		enum class ResolveSource : std::uint8_t
		{
			kNone,
			kLocalCache,
			kKnownRVA,
			kSharedCache,
			kPattern
		};

		enum class ResolveFailure : std::uint8_t
		{
			kNone,
			kUnknownID,
			kNoCompatiblePattern,
			kPatternNotFound,
			kPatternAmbiguous,
			kInvalidPattern,
			kRuntimeUnavailable,
			kSemanticValidationFailed
		};

		struct DetailedResolveResult
		{
			std::optional<std::size_t> rva;
			ResolveSource source{ ResolveSource::kNone };
			ResolveFailure failure{ ResolveFailure::kNone };

			[[nodiscard]] explicit operator bool() const noexcept { return rva.has_value(); }
		};

		struct BulkResolveResult
		{
			std::uint64_t id{};
			DetailedResolveResult result;
		};

		~RuntimeDatabase();

		[[nodiscard]] static bool contains(const std::filesystem::path& a_path);
		[[nodiscard]] static std::unique_ptr<RuntimeDatabase> load(const std::filesystem::path& a_path);
		[[nodiscard]] std::size_t record_count() const noexcept;
		[[nodiscard]] bool contains_id(std::uint64_t a_id, const Version& a_version) const;

		[[nodiscard]] std::optional<std::size_t> resolve(
			std::uint64_t a_id,
			const Version& a_version,
			const Module& a_module,
			ResolveMode a_mode = ResolveMode::kNormal,
			ResolveSource* a_source = nullptr) const;

		[[nodiscard]] DetailedResolveResult resolve_detailed(
			std::uint64_t a_id,
			const Version& a_version,
			const Module& a_module,
			ResolveMode a_mode = ResolveMode::kNormal) const;

		[[nodiscard]] std::vector<BulkResolveResult> resolve_all_patterns(
			const Version& a_version,
			const Module& a_module) const;

		[[nodiscard]] std::vector<std::pair<std::uint64_t, std::uint64_t>> known_mappings(
			const Version& a_version) const;

		[[nodiscard]] std::vector<std::pair<std::uint64_t, std::uint64_t>> pattern_mappings(
			const Version& a_version) const;
		[[nodiscard]] std::vector<std::uint64_t> pattern_ids(const Version& a_version) const;

	private:
		struct PatternResolveResult
		{
			std::optional<std::uint32_t> rva;
			ResolveFailure failure{ ResolveFailure::kNone };
		};

		class SharedCache;

		struct VersionKey
		{
			std::array<std::uint16_t, 4> parts{};

			[[nodiscard]] bool operator==(const VersionKey&) const noexcept = default;
		};

		struct Alias
		{
			std::uint64_t id{};
			VersionKey version;
			std::uint32_t flags{};
		};

		struct KnownRVA
		{
			VersionKey version;
			std::uint32_t rva{};
		};

		struct Pattern
		{
			std::vector<std::uint8_t> values;
			std::vector<std::uint8_t> masks;
		};

		struct Fragment
		{
			Pattern pattern;
			std::uint32_t minGap{};
			std::uint32_t maxGap{};
			std::uint32_t flags{};
		};

		struct Constraint
		{
			std::uint64_t targetID{};
			std::uint32_t operandOffset{};
			std::int32_t targetAdjustment{};
			std::uint16_t fragmentIndex{};
			std::uint16_t flags{};
			std::uint8_t kind{};
		};

		struct Candidate
		{
			std::uint8_t resolution{};
			std::uint8_t section{ 1 };
			std::uint16_t flags{};
			VersionKey version;
			std::uint16_t resolveFragment{};
			std::uint32_t resolveOffset{};
			std::int32_t resultAdjustment{};
			std::uint32_t minFixedBytes{};
			std::vector<Fragment> fragments;
			std::vector<Constraint> constraints;
		};

		struct ResolveContext
		{
			VersionKey version;
			std::unordered_map<std::uint64_t, std::uint32_t> cache;
			std::unordered_set<std::uint64_t> resolving;
			ResolveFailure failure{ ResolveFailure::kNone };

			void record_failure(ResolveFailure a_failure) noexcept
			{
				const auto priority = [](ResolveFailure a_value) noexcept {
					switch (a_value) {
					case ResolveFailure::kSemanticValidationFailed:
						return 7;
					case ResolveFailure::kRuntimeUnavailable:
						return 6;
					case ResolveFailure::kInvalidPattern:
						return 5;
					case ResolveFailure::kPatternAmbiguous:
						return 4;
					case ResolveFailure::kNoCompatiblePattern:
						return 3;
					case ResolveFailure::kPatternNotFound:
						return 2;
					case ResolveFailure::kUnknownID:
						return 1;
					case ResolveFailure::kNone:
						return 0;
					}
					return 0;
				};
				if (priority(a_failure) > priority(failure)) {
					failure = a_failure;
				}
			}
		};

		struct Record
		{
			std::uint64_t id{};
			std::uint32_t flags{};
			std::vector<Alias> aliases;
			std::vector<KnownRVA> knownRVAs;
			std::vector<Candidate> candidates;
		};

		struct AliasIndexEntry
		{
			std::uint64_t id{};
			VersionKey version;
			std::size_t recordIndex{};
			std::uint32_t flags{};
		};

		struct BatchPattern
		{
			std::uint8_t section{};
			std::span<const std::uint8_t> values;
			std::span<const std::uint8_t> masks;
			std::uint64_t anchor{};
			std::uint32_t anchorOffset{};
			std::uint8_t anchorLength{};
			bool indexed{};
			std::vector<std::size_t> matches;
		};

		struct BatchIndex
		{
			std::vector<BatchPattern> patterns;
			std::vector<std::uint8_t> ownedPatternBytes;
			std::unordered_map<std::uint64_t, std::size_t> patternsByHash;
			std::unordered_map<std::uint32_t, std::vector<std::size_t>> relativeBranches;
		};

		struct MappedLayout
		{
			std::span<const std::uint8_t> bytes;
			std::uint16_t formatMinor{};
			std::uint32_t recordCount{};
			std::uint32_t aliasCount{};
			std::uint32_t knownCount{};
			std::uint32_t candidateCount{};
			std::uint32_t fragmentCount{};
			std::uint32_t constraintCount{};
			std::size_t recordsOffset{};
			std::size_t aliasesOffset{};
			std::size_t knownOffset{};
			std::size_t candidatesOffset{};
			std::size_t fragmentsOffset{};
			std::size_t constraintsOffset{};
			std::size_t blobOffset{};
			std::size_t candidateSize{};
		};

		[[nodiscard]] const Record* find(std::uint64_t a_id, const VersionKey& a_version) const;
		[[nodiscard]] const Record* mapped_record(std::size_t a_recordIndex) const;
		[[nodiscard]] std::optional<std::uint32_t> resolve_candidate(
			const Candidate& a_candidate,
			const Module& a_module,
			ResolveContext& a_context) const;
		[[nodiscard]] std::optional<std::uint32_t> resolve_patterns(
			std::uint64_t a_id,
			const Module& a_module,
			ResolveContext& a_context) const;
		[[nodiscard]] bool validate_record_result(
			const Record& a_record,
			std::uint32_t a_rva,
			const Module& a_module) const;
		void prepare_batch(const VersionKey& a_version, const Module& a_module) const;
		void clear_batch() const;
		[[nodiscard]] std::vector<std::uint32_t> rip_string_owners(
			std::uint32_t a_targetRVA,
			const Module& a_module) const;
		[[nodiscard]] const BatchPattern* find_batch_pattern(
			std::uint8_t a_section,
			const Pattern& a_pattern) const;

		std::vector<Record> _records;
		std::vector<AliasIndexEntry> _aliasIndex;
		mmio::mapped_file_source _mappedFile;
		std::optional<MappedLayout> _mapped;
		mutable std::mutex _recordCacheLock;
		mutable std::unordered_map<std::size_t, std::unique_ptr<Record>> _recordCache;
		std::uint32_t _payloadCRC{};
		mutable std::mutex _cacheLock;
		mutable std::unordered_map<std::uint64_t, std::uint32_t> _cache;
		mutable std::unique_ptr<SharedCache> _sharedCache;
		mutable std::unique_ptr<BatchIndex> _batchIndex;
		mutable std::shared_mutex _batchStateLock;
		mutable std::mutex _batchOperationLock;
		mutable std::mutex _ripStringIndexLock;
		mutable std::unordered_map<std::uint32_t, std::vector<std::uint32_t>> _ripStringOwners;
		mutable std::uintptr_t _ripStringModuleBase{};
		mutable std::size_t _ripStringImageSize{};
		mutable bool _ripStringIndexReady{};
	};
}
