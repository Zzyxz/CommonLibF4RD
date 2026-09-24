#pragma once

#include "RE/NetImmerse/NiObject.h"
#include "RE/NetImmerse/NiVisibleArray.h"

namespace RE
{
	class NiBound;
	class NiCamera;

	class __declspec(novtable) NiAccumulator :
		public NiObject  // 00
	{
	public:
		static constexpr auto RTTI{ RTTI::NiAccumulator };
		static constexpr auto VTABLE{ VTABLE::NiAccumulator };
		static constexpr auto Ni_RTTI{ Ni_RTTI::NiAccumulator };

		virtual void StartAccumulating(const NiCamera*) {}              // 28
		virtual void FinishAccumulating() {}                            // 29
		virtual void RegisterObjectArray(NiVisibleArray*) {}            // 2A
		virtual void StartGroupingAlphas(const NiBound*, bool) {}       // 2B
		virtual void StopGroupingAlphas() {}                             // 2C
		virtual void RegisterObject(BSGeometry*) {}                      // 2D

		// members
		const NiCamera* camera;  // 10
	};
	static_assert(sizeof(NiAccumulator) == 0x18);
	static_assert(offsetof(NiAccumulator, camera) == 0x10);
}
