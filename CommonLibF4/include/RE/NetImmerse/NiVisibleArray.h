#pragma once

namespace RE
{
	class BSGeometry;

	class alignas(0x08) NiVisibleArray
	{
	public:
		// members
		BSGeometry** array;          // 00
		std::uint32_t currentSize;   // 08
		std::uint32_t allocatedSize; // 0C
		std::uint32_t growBy;        // 10
	};
	static_assert(sizeof(NiVisibleArray) == 0x18);
}
