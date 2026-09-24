#include "RE/Bethesda/VATSEffects.h"

#include "RE/NetImmerse/NiAVObject.h"

namespace RE
{
	VatsEffectTarget::VatsEffectTarget(NiAVObject* a_object)
	{
		using func_t = void(VatsEffectTarget*, NiAVObject*);
		REL::Relocation<func_t> func{ REL::ID(820921, 2194796, 2194796) };
		func(this, a_object);
	}

	VatsEffectTarget::~VatsEffectTarget()
	{
		using func_t = void(VatsEffectTarget*);
		REL::Relocation<func_t> func{ REL::ID(1073050, 2194797, 2194797) };
		func(this);
	}

	void VatsEffectTarget::AddTargetObject(const NiPointer<NiAVObject>& a_object)
	{
		using func_t = void(VatsEffectTarget*, const NiPointer<NiAVObject>&);
		REL::Relocation<func_t> func{ REL::ID(886596, 2194798, 2194798) };
		func(this, a_object);
	}

	void VatsEffectControl::AddTarget(const VatsEffectTargetPtr& a_target)
	{
		using func_t = void(const VatsEffectTargetPtr&);
		REL::Relocation<func_t> func{ REL::ID(126855, 2194806, 2194806) };
		func(a_target);
	}

	void VatsEffectControl::RemoveTarget(const VatsEffectTargetPtr& a_target)
	{
		using func_t = void(const VatsEffectTargetPtr&);
		REL::Relocation<func_t> func{ REL::ID(1335846, 2194808, 2194808) };
		func(a_target);
	}
}
