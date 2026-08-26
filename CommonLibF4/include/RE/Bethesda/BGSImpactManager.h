#pragma once

#include "RE/Bethesda/BSFixedString.h"
#include "RE/NetImmerse/NiPoint3.h"

namespace RE
{
	class BGSImpactDataSet;
	class TESObjectREFR;

	class BGSImpactManager
	{
	public:
		static constexpr auto RTTI{ RTTI::BGSImpactManager };
		static constexpr auto VTABLE{ VTABLE::BGSImpactManager };

		[[nodiscard]] static BGSImpactManager* GetSingleton()
		{
			static REL::Relocation<BGSImpactManager**> singleton{ REL::ID(73076, 4799276) };
			return *singleton;
		}

		bool PlayImpactEffect(
			TESObjectREFR* a_reference,
			BGSImpactDataSet* a_impactEffect,
			const BSFixedString& a_nodeName,
			const NiPoint3A& a_pickDirection,
			float a_pickLength,
			bool a_applyNodeRotation,
			bool a_useNodeLocalRotation)
		{
			using func_t = decltype(&BGSImpactManager::PlayImpactEffect);
			static REL::Relocation<func_t> func{ REL::ID(142410, 2228497) };
			return func(
				this,
				a_reference,
				a_impactEffect,
				a_nodeName,
				a_pickDirection,
				a_pickLength,
				a_applyNodeRotation,
				a_useNodeLocalRotation);
		}
	};
}
