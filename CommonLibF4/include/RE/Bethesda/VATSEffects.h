#pragma once

#include "RE/Bethesda/BSTSmartPointer.h"
#include "RE/Bethesda/MemoryManager.h"
#include "RE/NetImmerse/NiSmartPointer.h"

namespace RE
{
	class NiAVObject;

	class VatsEffectTarget :
		public BSIntrusiveRefCounted
	{
	public:
		explicit VatsEffectTarget(NiAVObject* a_object);
		~VatsEffectTarget();

		F4_HEAP_REDEFINE_NEW(VatsEffectTarget);

		void AddTargetObject(const NiPointer<NiAVObject>& a_object);

	private:
		std::byte data_[0xA4]{};
	};
	static_assert(sizeof(VatsEffectTarget) == 0xA8);

	using VatsEffectTargetPtr = BSTSmartPointer<VatsEffectTarget, BSTSmartPointerIntrusiveRefCount>;

	class VatsEffectControl
	{
	public:
		static void AddTarget(const VatsEffectTargetPtr& a_target);
		static void RemoveTarget(const VatsEffectTargetPtr& a_target);
	};
}
