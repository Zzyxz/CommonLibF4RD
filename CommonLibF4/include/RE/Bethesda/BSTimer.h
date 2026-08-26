#pragma once

namespace RE
{
	class BSTimer
	{
	public:
		static constexpr std::size_t OG_SIZE = 0x40;
		static constexpr std::size_t NG_SIZE = 0x40;
		static constexpr std::size_t AE_SIZE = 0x50;
		static constexpr std::size_t OG_RUNTIME_DATA_OFFSET = 0x20;
		static constexpr std::size_t NG_RUNTIME_DATA_OFFSET = 0x20;
		static constexpr std::size_t AE_RUNTIME_DATA_OFFSET = 0x30;

		class RuntimeData
		{
		public:
			std::uint64_t firstTime;              // 00
			std::uint64_t disabledLastTime;       // 08
			std::uint64_t disabledFirstTime;      // 10
			std::uint32_t disableCounter;         // 18
			bool useGlobalTimeMultiplierTarget;  // 1C
		};
		static_assert(sizeof(RuntimeData) == 0x20);

		[[nodiscard]] static constexpr std::size_t GetRuntimeDataOffset(const REL::Version& a_version) noexcept
		{
			return REL::Relocate(
				a_version,
				OG_RUNTIME_DATA_OFFSET,
				NG_RUNTIME_DATA_OFFSET,
				AE_RUNTIME_DATA_OFFSET);
		}

		[[nodiscard]] static std::size_t GetRuntimeDataOffset() noexcept
		{
			return GetRuntimeDataOffset(REL::Module::get().version());
		}

		[[nodiscard]] static constexpr std::size_t GetRuntimeSize(const REL::Version& a_version) noexcept
		{
			return REL::Relocate(a_version, OG_SIZE, NG_SIZE, AE_SIZE);
		}

		[[nodiscard]] static std::size_t GetRuntimeSize() noexcept
		{
			return GetRuntimeSize(REL::Module::get().version());
		}

		[[nodiscard]] RuntimeData& GetRuntimeData() noexcept
		{
			return REL::RelocateMember<RuntimeData>(
				this,
				OG_RUNTIME_DATA_OFFSET,
				NG_RUNTIME_DATA_OFFSET,
				AE_RUNTIME_DATA_OFFSET);
		}

		[[nodiscard]] const RuntimeData& GetRuntimeData() const noexcept
		{
			return REL::RelocateMember<RuntimeData>(
				this,
				OG_RUNTIME_DATA_OFFSET,
				NG_RUNTIME_DATA_OFFSET,
				AE_RUNTIME_DATA_OFFSET);
		}

		// Common prefix shared by every supported runtime.
		std::int64_t highPrecisionInitTime;  // 00
		float clamp;                         // 08
		float clampRemainder;                // 0C
		float delta;                         // 10
		float realTimeDelta;                 // 14
		std::uint64_t lastTime;              // 18
	};
	static_assert(sizeof(BSTimer) == 0x20);
}
