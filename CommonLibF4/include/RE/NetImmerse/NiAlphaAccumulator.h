#pragma once

#include "RE/NetImmerse/NiBackToFrontAccumulator.h"

namespace RE
{
	class __declspec(novtable) NiAlphaAccumulator :
		public NiBackToFrontAccumulator  // 00
	{
	public:
		static constexpr auto RTTI{ RTTI::NiAlphaAccumulator };
		static constexpr auto VTABLE{ VTABLE::NiAlphaAccumulator };
		static constexpr auto Ni_RTTI{ Ni_RTTI::NiAlphaAccumulator };

		bool observeNoSortHint;   // 50
		bool sortByClosestPoint;  // 51
		bool interfaceSort;       // 52
		std::byte padding[0x5];   // 53
	};
	static_assert(sizeof(NiAlphaAccumulator) == 0x58);
	static_assert(offsetof(NiAlphaAccumulator, observeNoSortHint) == 0x50);
}
