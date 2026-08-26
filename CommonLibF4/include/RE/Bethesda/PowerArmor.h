#pragma once

#include "RE/Bethesda/BSTEvent.h"
#include "RE/Bethesda/Events.h"
#include "RE/Bethesda/HUDModes.h"

namespace RE
{
	namespace ExitPowerArmor
	{
		struct Event;
	}

	namespace PreloadPowerArmor
	{
		struct Event;
	}

	namespace PowerArmor
	{
		[[nodiscard]] inline bool ActorInPowerArmor(const Actor& a_actor)
		{
			using func_t = decltype(&ActorInPowerArmor);
			REL::Relocation<func_t> func{ REL::ID(1176757, 2219437) };
			return func(a_actor);
		}

		[[nodiscard]] inline bool IsLimbCoveredByPowerArmor(const Actor& a_actor, const BGSBodyPartData::PartType& a_part)
		{
			using func_t = decltype(&IsLimbCoveredByPowerArmor);
			REL::Relocation<func_t> func{ REL::ID(1387488, 2219440) };
			return func(a_actor, a_part);
		}

		[[nodiscard]] inline bool PlayerInPowerArmor()
		{
			return ActorInPowerArmor(*PlayerCharacter::GetSingleton());
		}

		[[nodiscard]] inline BGSKeyword* GetArmorKeyword()
		{
			using func_t = decltype(&GetArmorKeyword);
			REL::Relocation<func_t> func{ REL::ID(961172, 2194743) };
			return func();
		}

		[[nodiscard]] inline BGSKeyword* GetBatteryKeyword()
		{
			using func_t = decltype(&GetBatteryKeyword);
			REL::Relocation<func_t> func{ REL::ID(1493537, 2194740) };
			return func();
		}

		[[nodiscard]] inline TESAmmo* GetDefaultBatteryObject()
		{
			using func_t = decltype(&GetDefaultBatteryObject);
			REL::Relocation<func_t> func{ REL::ID(1279247, 2194761) };
			return func();
		}

		inline void SyncFurnitureVisualsToInventory(TESObjectREFR* a_furniture, bool a_force3DUpdate, BGSInventoryItem* a_tempItemToAdd, bool a_hideCore)
		{
			using func_t = decltype(&SyncFurnitureVisualsToInventory);
			REL::Relocation<func_t> func{ REL::ID(1078979, 2219453) };
			return func(a_furniture, a_force3DUpdate, a_tempItemToAdd, a_hideCore);
		}

		inline REL::Relocation<SettingT<GameSettingCollection>*> fNewBatteryCapacity{ REL::ID(1134204, 1134204) };
	}

	class __declspec(novtable) PowerArmorGeometry :
		public SimpleAnimationGraphManagerHolder,       // 00
		public BSTEventSink<PreloadPowerArmor::Event>,  // 18
		public BSTEventSink<ExitPowerArmor::Event>,     // 20
		public BSIntrusiveRefCounted,                   // 28
		public BSTSingletonSDM<PowerArmorGeometry>      // 2C
	{
	public:
		static constexpr auto RTTI{ RTTI::PowerArmorGeometry };
		static constexpr auto VTABLE{ VTABLE::PowerArmorGeometry };

		virtual ~PowerArmorGeometry();  // 00

		// override
		virtual void BackgroundTaskFinishedLoading() override;                                                                                            // 01
		virtual BSEventNotifyControl ProcessEvent(const PreloadPowerArmor::Event& a_event, BSTEventSource<PreloadPowerArmor::Event>* a_source) override;  // 02
		virtual BSEventNotifyControl ProcessEvent(const ExitPowerArmor::Event& a_event, BSTEventSource<ExitPowerArmor::Event>* a_source) override;        // 03

		[[nodiscard]] static PowerArmorGeometry* GetSingleton()
		{
			REL::Relocation<PowerArmorGeometry**> singleton{ REL::ID(1365745, 4801732) };
			return *singleton;
		}

		// members
		BSTValueEventSink<PipboyLightEvent> pipboySpotLightEvent;             // 030
		BSTValueEventSink<PlayerAmmoCountEvent> nextAmmoCount;                // 048
		BSTValueEventSink<PlayerWeaponReloadEvent> playerReloaded;            // 068
		BSTValueEventSink<CurrentRadsDisplayMagnitude> radsCount;             // 080
		BSTValueEventSink<CurrentRadsPercentOfLethal> radsPercentOfLethal;    // 0A0
		BSTValueEventSink<CurrentRadiationSourceCount> radiationSourceCount;  // 0C0
		TESImageSpaceModifier* paPipboyEffect;                                // 0E0
		NiPointer<NiNode> paDashDials;                                        // 0E8
		NiPointer<NiNode> pipboyPAGlass;                                      // 0F0
		NiAVObject* paDashLightSpot;                                          // 0F8
		NiAVObject* paDashLightScope;                                         // 100
		NiAVObject* paDashLightAux;                                           // 108
		NiAVObject* paDash;                                                   // 110
		NiAVObject* plane;                                                    // 118
		NiAVObject* compass;                                                  // 120
		BSTArray<void*> queued3D;                                             // 128
		HUDModes validHUDModes;                                               // 140
		float radiationSmoother;                                              // 160
		NiPointer<NiNode> dbHUDRain;                                          // 168
		bool hudRain;                                                         // 170
		bool prevShouldBeVisible;                                             // 171
		bool powerArmorEmergencyLightOn;                                      // 172
		bool queueInit;                                                       // 173
		bool prevIsFirstPerson;                                               // 174
		bool ammoDirty;                                                       // 175
		bool initialized;                                                     // 176
	};
	static_assert(sizeof(PowerArmorGeometry) == 0x178);
}
