#pragma once

#include <RE/Bethesda/TESBoundObjects.h>
#include <RE/Bethesda/TESObjectREFRs.h>
#include <RE/NetImmerse/NiAVObject.h>
#include <REL/Relocation.h>

namespace RE
{
	class TaskQueueInterface
	{
	public:
		static TaskQueueInterface* GetSingleton()
		{
			REL::Relocation<TaskQueueInterface**> singleton{ REL::ID(7491, 2698331) };
			return *singleton;
		}

		void QueueRebuildBendableSpline(TESObjectREFR* ref, bool rebuildCollision, NiAVObject* target)
		{
			using func_t = decltype(&TaskQueueInterface::QueueRebuildBendableSpline);
			REL::Relocation<func_t> func{ REL::ID(198419, 2229302) };
			return func(this, ref, rebuildCollision, target);
		}

		void QueueShow1stPerson(bool shouldShow)
		{
			using func_t = decltype(&TaskQueueInterface::QueueShow1stPerson);
			REL::Relocation<func_t> func{ REL::ID(994377, 2229268) };
			return func(this, shouldShow);
		}

		void QueueUpdate3D(TESObjectREFR* ref, std::uint32_t unk = 0)
		{
			using func_t = decltype(&TaskQueueInterface::QueueUpdate3D);
			REL::Relocation<func_t> func{ REL::ID(581890, 2229234) };
			return func(this, ref, unk);
		}

		void QueueWeaponFire(TESObjectWEAP* wep, TESObjectREFR* shooter, BGSEquipIndex index, TESAmmo* ammo)
		{
			using func_t = decltype(&TaskQueueInterface::QueueWeaponFire);
			REL::Relocation<func_t> func{ REL::ID(15449, 2229186) };
			return func(this, wep, shooter, index, ammo);
		}
	};
}
