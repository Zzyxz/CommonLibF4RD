#pragma once

#include "RE/NetImmerse/NiAccumulator.h"

namespace RE
{
	class __declspec(novtable) NiBackToFrontAccumulator :
		public NiAccumulator  // 00
	{
	public:
		static constexpr auto RTTI{ RTTI::NiBackToFrontAccumulator };
		static constexpr auto VTABLE{ VTABLE::NiBackToFrontAccumulator };
		static constexpr auto Ni_RTTI{ Ni_RTTI::NiBackToFrontAccumulator };

		std::byte data[0x38];  // 18
	};
	static_assert(sizeof(NiBackToFrontAccumulator) == 0x50);
}
