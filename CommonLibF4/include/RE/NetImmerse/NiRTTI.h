#pragma once

namespace RE
{
	struct NiRTTI
	{
	public:
		[[nodiscard]] constexpr const char* GetName() const noexcept { return name; }
		[[nodiscard]] constexpr const NiRTTI* GetBaseRTTI() const noexcept { return baseRTTI; }

		[[nodiscard]] constexpr bool IsKindOf(const NiRTTI* a_rtti) const noexcept
		{
			for (auto iter = this; iter; iter = iter->GetBaseRTTI()) {
				if (iter == a_rtti) {
					return true;
				}
			}
			return false;
		}

		// members
		const char* name;  // 00
		NiRTTI* baseRTTI;  // 08
	};
	static_assert(sizeof(NiRTTI) == 0x10);
}
