#pragma once

#include "RE/Bethesda/BSTempEffect.h"
#include "RE/NetImmerse/NiMatrix3.h"

namespace RE
{
	class __declspec(novtable) BSTempEffectDebris :
		public BSTempEffect  // 00
	{
	public:
		static constexpr auto RTTI{ RTTI::BSTempEffectDebris };
		static constexpr auto VTABLE{ VTABLE::BSTempEffectDebris };
		static constexpr auto Ni_RTTI{ Ni_RTTI::BSTempEffectDebris };

		//unk0 = 1.0, unk1 = false, unk2 = true
		BSTempEffectDebris(TESObjectCELL* parentCell, float lifetime, const char* modelPath, TESObjectREFR* target, const NiPoint3& pos, const NiMatrix3& rot, const NiPoint3& vel, const NiPoint3& angVel, float unk0, bool unk1, bool unk2, bool isFirstPerson)
		{
			typedef BSTempEffectDebris* func_t(BSTempEffectDebris*, TESObjectCELL*, float, const char*, TESObjectREFR*, const NiPoint3&, const NiMatrix3&, const NiPoint3&, const NiPoint3&, float, bool, bool, bool);
			REL::Relocation<func_t> func{ REL::ID(1075623, 2212059) };
			func(this, parentCell, lifetime, modelPath, target, pos, rot, vel, angVel, unk0, unk1, unk2, isFirstPerson);
		}

		// members
		std::uint64_t unk40;
	};
	static_assert(sizeof(BSTempEffectDebris) == 0x48);
}
