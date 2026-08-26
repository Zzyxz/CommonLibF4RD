#pragma once
#pragma once
#include <cstdint>
#include <cstring>

namespace RE
{
	class NiAVObject;
	class hknpBody;

	namespace HavokUtil
	{
		// Fallout 4:
		// hknpBody + 0x88 -> userData wrapper
		// wrapper  + 0x10 -> NiAVObject*
		inline NiAVObject* GetNiAVObject(const hknpBody* body) noexcept
		{
			if (!body) {
				return nullptr;
			}

			std::uintptr_t wrapper = 0;
			std::memcpy(
				&wrapper,
				reinterpret_cast<const std::uint8_t*>(body) + 0x88,
				sizeof(wrapper));

			// basic pointer sanity
			if (!wrapper || (wrapper & 0x7) != 0) {
				return nullptr;
			}

			std::uintptr_t niPtr = 0;
			std::memcpy(
				&niPtr,
				reinterpret_cast<const void*>(wrapper + 0x10),
				sizeof(niPtr));

			if (!niPtr || (niPtr & 0x7) != 0) {
				return nullptr;
			}

			return reinterpret_cast<NiAVObject*>(niPtr);
		}
	}
}
