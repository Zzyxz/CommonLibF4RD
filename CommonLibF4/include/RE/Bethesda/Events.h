#pragma once

#include "RE/Bethesda/BGSBodyPartDefs.h"
#include "RE/Bethesda/BSFixedString.h"
#include "RE/Bethesda/BSPointerHandle.h"
#include "RE/Bethesda/BSTArray.h"
#include "RE/Bethesda/BSTEvent.h"
#include "RE/Bethesda/TESObjectREFRs.h"
#include "RE/Bethesda/UserEvents.h"
#include "RE/NetImmerse/NiMatrix3.h"
#include "RE/NetImmerse/NiPoint3.h"
#include "RE/NetImmerse/NiSmartPointer.h"
#include "RE/Scaleform/GFx/GFx_Player.h"

namespace RE
{
	class bhkNPCollisionObject;
	class BGSAttackData;
	class BGSMessage;
	class TESObjectCELL;
	class TESObjectREFR;

	struct InventoryUserUIInterfaceEntry;

	struct BSThreadEvent
	{
	public:
		enum class ThreadEvent
		{
			kOnStartup,
			kOnShutdown
		};
		using Event = ThreadEvent;

		static void InitSDM()
		{
			using func_t = decltype(&BSThreadEvent::InitSDM);
			REL::Relocation<func_t> func{ REL::ID(1425097, 2268180) };
			return func();
		}

		static void RegisterSink(BSTEventSink<ThreadEvent>* a_sink)
		{
			using func_t = decltype(&BSThreadEvent::RegisterSink);
			REL::Relocation<func_t> func{ REL::ID(1159379, 2268181) };
			return func(a_sink);
		}

		static void UnregisterSink(BSTEventSink<ThreadEvent>* a_sink)
		{
			using func_t = decltype(&BSThreadEvent::UnregisterSink);
			REL::Relocation<func_t> func{ REL::ID(454294, 2268182) };
			return func(a_sink);
		}

		static void KillSDM()
		{
			using func_t = decltype(&BSThreadEvent::KillSDM);
			REL::Relocation<func_t> func{ REL::ID(1331907, 2268183) };
			return func();
		}
	};
	static_assert(std::is_empty_v<BSThreadEvent>);

	enum class QuickContainerMode : std::int32_t
	{
		kLoot,
		kTeammate,
		kPowerArmor,
		kTurret,
		kWorkshop,
		kCrafting,
		kStealing,
		kStealingPowerArmor
	};

	struct ApplyColorUpdateEvent
	{
	private:
		using EventSource_t = BSTGlobalEvent::EventSource<ApplyColorUpdateEvent>;

	public:
		[[nodiscard]] static EventSource_t* GetEventSource()
		{
			REL::Relocation<EventSource_t**> singleton{ REL::ID(421543, 2707340) };
			if (!*singleton) {
				*singleton = new EventSource_t(&BSTGlobalEvent::GetSingleton()->eventSourceSDMKiller);
			}
			return *singleton;
		}
	};
	static_assert(std::is_empty_v<ApplyColorUpdateEvent>);

	class CanDisplayNextHUDMessage :
		public BSTValueEvent<bool>
	{
	private:
		using EventSource_t = BSTGlobalEvent::EventSource<CanDisplayNextHUDMessage>;

	public:
		CanDisplayNextHUDMessage(bool a_value)
		{
			optionalValue = a_value;
		}

		[[nodiscard]] static EventSource_t* GetEventSource()
		{
			REL::Relocation<EventSource_t**> singleton{ REL::ID(344866, 4802332) };
			if (!*singleton) {
				*singleton = new EventSource_t(&BSTGlobalEvent::GetSingleton()->eventSourceSDMKiller);
			}
			return *singleton;
		}
	};

	struct CellAttachDetachEvent
	{
	public:
		enum class EVENT_TYPE
		{
			kPreAttach,
			kPostAttach,
			kPreDetach,
			kPostDetach
		};

		// members
		TESObjectCELL* cell;                              // 00
		stl::enumeration<EVENT_TYPE, std::int32_t> type;  // 08
	};
	static_assert(sizeof(CellAttachDetachEvent) == 0x10);

	class CurrentRadiationSourceCount :
		public BSTValueEvent<std::uint32_t>
	{
	private:
		using EventSource_t = BSTGlobalEvent::EventSource<CurrentRadiationSourceCount>;

	public:
		[[nodiscard]] static EventSource_t* GetEventSource()
		{
			REL::Relocation<EventSource_t**> singleton{ REL::ID(696410, 4803487) };
			if (!*singleton) {
				*singleton = new EventSource_t(&BSTGlobalEvent::GetSingleton()->eventSourceSDMKiller);
			}
			return *singleton;
		}
	};
	static_assert(sizeof(CurrentRadiationSourceCount) == 0x08);

	class CurrentRadsDisplayMagnitude :
		public BSTValueEvent<float>
	{
	public:
	};
	static_assert(sizeof(CurrentRadsDisplayMagnitude) == 0x08);

	class CurrentRadsPercentOfLethal :
		public BSTValueEvent<float>
	{
	public:
	};
	static_assert(sizeof(CurrentRadsPercentOfLethal) == 0x08);

	struct DoBeforeNewOrLoadCompletedEvent
	{
	private:
		using EventSource_t = BSTGlobalEvent::EventSource<DoBeforeNewOrLoadCompletedEvent>;

	public:
		[[nodiscard]] static EventSource_t* GetEventSource()
		{
			REL::Relocation<EventSource_t**> singleton{ REL::ID(787908, 4802833) };
			if (!*singleton) {
				*singleton = new EventSource_t(&BSTGlobalEvent::GetSingleton()->eventSourceSDMKiller);
			}
			return *singleton;
		}
	};
	static_assert(sizeof(DoBeforeNewOrLoadCompletedEvent) == 0x01);

	struct InventoryItemDisplayData
	{
	public:
		InventoryItemDisplayData(
			const ObjectRefHandle a_inventoryRef,
			const InventoryUserUIInterfaceEntry& a_entry)
		{
			ctor(a_inventoryRef, a_entry);
		}

		void PopulateFlashObject(Scaleform::GFx::Value& a_flashObject)
		{
			a_flashObject.SetMember("text"sv, itemName.c_str());
			a_flashObject.SetMember("count"sv, itemCount);
			a_flashObject.SetMember("equipState"sv, equipState);
			a_flashObject.SetMember("filterFlag"sv, filterFlag);
			a_flashObject.SetMember("isLegendary"sv, isLegendary);
			a_flashObject.SetMember("favorite"sv, isFavorite);
			a_flashObject.SetMember("taggedForSearch"sv, isTaggedForSearch);
			a_flashObject.SetMember("isBetterThanEquippedItem"sv, isBetterThanEquippedItem);
		}

		// members
		BSFixedStringCS itemName;                // 00
		std::uint32_t itemCount{ 0 };            // 08
		std::uint32_t equipState{ 0 };           // 0C
		std::uint32_t filterFlag{ 0 };           // 10
		bool isLegendary{ false };               // 14
		bool isFavorite{ false };                // 15
		bool isTaggedForSearch{ false };         // 16
		bool isBetterThanEquippedItem{ false };  // 17

	private:
		InventoryItemDisplayData* ctor(
			const ObjectRefHandle a_inventoryRef,
			const InventoryUserUIInterfaceEntry& a_entry)
		{
			using func_t = decltype(&InventoryItemDisplayData::ctor);
			REL::Relocation<func_t> func{ REL::ID(679373, 2222612) };
			return func(this, a_inventoryRef, a_entry);
		}
	};
	static_assert(sizeof(InventoryItemDisplayData) == 0x18);

	class LocksPicked
	{
	public:
		struct Event
		{
		public:
		};

		[[nodiscard]] static BSTEventSource<LocksPicked::Event>* GetEventSource()
		{
			using func_t = decltype(&LocksPicked::GetEventSource);
			REL::Relocation<func_t> func{ REL::ID(594991, 2249292) };
			return func();
		}
	};
	static_assert(std::is_empty_v<LocksPicked>);
	static_assert(std::is_empty_v<LocksPicked::Event>);

	class MenuModeChangeEvent
	{
	public:
		// members
		BSFixedString menuName;  // 00
		bool enteringMenuMode;   // 08
	};
	static_assert(sizeof(MenuModeChangeEvent) == 0x10);

	class MenuModeCounterChangedEvent
	{
	public:
		// members
		BSFixedString menuName;  // 00
		bool incrementing;       // 08
	};
	static_assert(sizeof(MenuModeCounterChangedEvent) == 0x10);

	class MenuOpenCloseEvent
	{
	public:
		// members
		BSFixedString menuName;  // 00
		bool opening;            // 08
	};
	static_assert(sizeof(MenuOpenCloseEvent) == 0x10);

	class PerkPointIncreaseEvent
	{
	private:
		using EventSource_t = BSTGlobalEvent::EventSource<PerkPointIncreaseEvent>;

	public:
		PerkPointIncreaseEvent(std::uint8_t a_perkCount) :
			perkCount(a_perkCount)
		{}

		[[nodiscard]] static EventSource_t* GetEventSource()
		{
			REL::Relocation<EventSource_t**> singleton{ REL::ID(685859, 4804734) };
			if (!*singleton) {
				*singleton = new EventSource_t(&BSTGlobalEvent::GetSingleton()->eventSourceSDMKiller);
			}
			return *singleton;
		}

		// members
		std::uint8_t perkCount{ 0 };  // 00
	};
	static_assert(sizeof(PerkPointIncreaseEvent) == 0x1);

	class PipboyLightEvent :
		public BSTValueEvent<bool>
	{
	private:
		using EventSource_t = BSTGlobalEvent::EventSource<PipboyLightEvent>;

	public:
		[[nodiscard]] static EventSource_t* GetEventSource()
		{
			REL::Relocation<EventSource_t**> singleton{ REL::ID(1140080, 4803571) };
			if (!*singleton) {
				*singleton = new EventSource_t(&BSTGlobalEvent::GetSingleton()->eventSourceSDMKiller);
			}
			return *singleton;
		}
	};
	static_assert(sizeof(PipboyLightEvent) == 0x02);

	struct PlayerAmmoCounts
	{
	public:
		// members
		std::uint32_t clipAmmo;     // 00
		std::uint32_t reserveAmmo;  // 04
	};
	static_assert(sizeof(PlayerAmmoCounts) == 0x08);

	class PlayerAmmoCountEvent :
		public BSTValueEvent<PlayerAmmoCounts>
	{
	public:
	};
	static_assert(sizeof(PlayerAmmoCountEvent) == 0x0C);

	class PlayerWeaponReloadEvent :
		public BSTValueEvent<bool>
	{
	public:
	};
	static_assert(sizeof(PlayerWeaponReloadEvent) == 0x02);

	struct QuickContainerStateData
	{
	public:
		// members
		BSTSmallArray<InventoryItemDisplayData, 5> itemData;      // 00
		ObjectRefHandle containerRef;                             // 88
		ObjectRefHandle inventoryRef;                             // 8C
		BSFixedStringCS aButtonText;                              // 90
		BSFixedString containerName;                              // 98
		BSFixedStringCS perkButtonText;                           // A0
		std::int32_t selectedClipIndex;                           // A8
		stl::enumeration<QuickContainerMode, std::int32_t> mode;  // AC
		bool perkButtonEnabled;                                   // B0
		bool isNewContainer;                                      // B1
		bool addedDroppedItems;                                   // B2
		bool isLocked;                                            // B3
		bool buttonAEnabled;                                      // B4
		bool buttonXEnabled;                                      // B5
		bool refreshContainerSize;                                // B6
		bool containerActivated;                                  // B7
	};
	static_assert(sizeof(QuickContainerStateData) == 0xB8);

	class QuickContainerStateEvent :
		public BSTValueEvent<QuickContainerStateData>  // 00
	{
	public:
	};
	static_assert(sizeof(QuickContainerStateEvent) == 0xC0);

	struct TESActivateEvent
	{
	public:
		[[nodiscard]] static BSTEventSource<TESActivateEvent>* GetEventSource()
		{
			using func_t = decltype(&TESActivateEvent::GetEventSource);
			REL::Relocation<func_t> func{ REL::ID(166230, 2201819) };
			return func();
		}

		// members
		NiPointer<TESObjectREFR> objectActivated;  // 00
		NiPointer<TESObjectREFR> actionRef;        // 08
	};
	static_assert(sizeof(TESActivateEvent) == 0x10);

	//possible IDs, 204197, 1329866
	struct TESStealEvent
	{
	public:
		[[nodiscard]] static BSTEventSource<TESStealEvent>* GetEventSource()
		{
			using func_t = decltype(&TESStealEvent::GetEventSource);
			REL::Relocation<func_t> func{ REL::ID(204197, 2233732) };
			return func();
		}

		// members
		std::uint32_t count;  // 00
		bool unk1;   // 00
		TESObjectREFR* refr;   // 00
	};
	//static_assert(sizeof(TESActivateEvent) == 0x10);

		//possible IDs, 204197, 1329866
	struct TESFireEvent
	{
	public:
		// members
		std::uint32_t count;  // 00
		bool unk1;            // 00
		TESObjectREFR* refr;  // 00
	};
	//static_assert(sizeof(TESActivateEvent) == 0x10);

	struct TESContainerChangedEvent
	{
	public:
		[[nodiscard]] static BSTEventSource<TESContainerChangedEvent>* GetEventSource()
		{
			using func_t = decltype(&TESContainerChangedEvent::GetEventSource);
			REL::Relocation<func_t> func{ REL::ID(242538, 2201832) };
			return func();
		}

		// members
		std::uint32_t oldContainerFormID;  // 00
		std::uint32_t newContainerFormID;  // 04
		std::uint32_t baseObjectFormID;    // 08
		std::int32_t itemCount;            // 0C
		std::uint32_t referenceFormID;     // 10
		std::uint16_t uniqueID;            // 14
	};
	static_assert(sizeof(TESContainerChangedEvent) == 0x18);

	struct TESDeathEvent
	{
	public:
		[[nodiscard]] static BSTEventSource<TESDeathEvent>* GetEventSource()
		{
			using func_t = decltype(&TESDeathEvent::GetEventSource);
			REL::Relocation<func_t> func{ REL::ID(1465690, 2201833) };
			return func();
		}

		// members
		NiPointer<TESObjectREFR> actorDying;   // 00
		NiPointer<TESObjectREFR> actorKiller;  // 08
		bool dead;                             // 10
	};
	static_assert(sizeof(TESDeathEvent) == 0x18);

	struct TESFurnitureEvent
	{
	public:
		enum class FurnitureEventType : std::int32_t
		{
			kEnter,
			kExit
		};

		[[nodiscard]] static BSTEventSource<TESFurnitureEvent>* GetEventSource()
		{
			using func_t = decltype(&TESFurnitureEvent::GetEventSource);
			REL::Relocation<func_t> func{ REL::ID(678665, 2201844) };
			return func();
		}

		// members
		NiPointer<TESObjectREFR> actor;                           // 00
		NiPointer<TESObjectREFR> targetFurniture;                 // 08
		stl::enumeration<FurnitureEventType, std::int32_t> type;  // 10
	};
	static_assert(sizeof(TESFurnitureEvent) == 0x18);

	struct TESMagicEffectApplyEvent
	{
	public:
		[[nodiscard]] static BSTEventSource<TESMagicEffectApplyEvent>* GetEventSource()
		{
			using func_t = decltype(&TESMagicEffectApplyEvent::GetEventSource);
			REL::Relocation<func_t> func{ REL::ID(1327824, 2201851) };
			return func();
		}

		// members
		NiPointer<TESObjectREFR> target;  // 00
		NiPointer<TESObjectREFR> caster;  // 08
		std::uint32_t magicEffectFormID;  // 10
	};
	static_assert(sizeof(TESMagicEffectApplyEvent) == 0x18);

	class TutorialEvent
	{
	public:
		// members
		BSFixedString eventName;     // 00
		const BGSMessage* assocMsg;  // 08
	};
	static_assert(sizeof(TutorialEvent) == 0x10);

	class UserEventEnabledEvent
	{
	public:
		// members
		stl::enumeration<UserEvents::USER_EVENT_FLAG, std::int32_t> newUserEventFlag;  // 0
		stl::enumeration<UserEvents::USER_EVENT_FLAG, std::int32_t> oldUserEventFlag;  // 4
		stl::enumeration<UserEvents::SENDER_ID, std::int32_t> senderID;                // 8
	};
	static_assert(sizeof(UserEventEnabledEvent) == 0xC);

	namespace CellAttachDetachEventSource
	{
		struct CellAttachDetachEventSourceSingleton :
			public BSTSingletonImplicit<CellAttachDetachEventSourceSingleton>
		{
		public:
			[[nodiscard]] static CellAttachDetachEventSourceSingleton& GetSingleton()
			{
				using func_t = decltype(&CellAttachDetachEventSourceSingleton::GetSingleton);
				REL::Relocation<func_t> func{ REL::ID(862142, 2192250) };
				return func();
			}

			// members
			BSTEventSource<CellAttachDetachEvent> source;  // 00
		};
		static_assert(sizeof(CellAttachDetachEventSourceSingleton) == 0x58);
	}

	struct TESObjectLoadedEvent
	{
	public:
		[[nodiscard]] static BSTEventSource<TESObjectLoadedEvent>* GetEventSource()
		{
			using func_t = decltype(&TESObjectLoadedEvent::GetEventSource);
			REL::Relocation<func_t> func{ REL::ID(609604, 2201853) };
			return func();
		}

		// members
		std::uint32_t formID;  // 00
		bool loaded;           // 04
	};
	static_assert(sizeof(TESObjectLoadedEvent) == 0x8);

	struct TESEquipEvent
	{
	public:
		[[nodiscard]] static BSTEventSource<TESEquipEvent>* GetEventSource()
		{
			using func_t = decltype(&TESEquipEvent::GetEventSource);
			REL::Relocation<func_t> func{ REL::ID(1251703, 2201838) };
			return func();
		}

		// members
		NiPointer<TESObjectREFR> actor;         // 00
		std::uint32_t baseObject;               // 08
		std::uint32_t originalRefr;             // 0C
		std::uint16_t uniqueID;                 // 10
		bool equipped;                          // 12
	};
	static_assert(sizeof(TESEquipEvent) == 0x18);

	struct BSTransformDeltaEvent
	{
		NiMatrix3 deltaRotation;
		NiPoint4 deltaTranslation;
		NiPoint4 previousTranslation;
		NiPoint4 previousRotation;
		NiPoint4 previousScale;
		NiPoint4 currentTranslation;
		NiPoint4 currentRotation;
		NiPoint4 currentScale;
	};

	struct alignas(0x10) DamageImpactData
	{
	public:
		static constexpr std::size_t OG_SIZE = 0x38;
		static constexpr std::size_t AE_SIZE = 0x40;

		[[nodiscard]] static constexpr std::size_t GetRuntimeSize(const REL::Version& a_version) noexcept
		{
			return REL::Relocate(a_version, OG_SIZE, AE_SIZE);
		}

		[[nodiscard]] static std::size_t GetRuntimeSize() noexcept
		{
			return REL::Relocate(OG_SIZE, AE_SIZE);
		}

		// members
		NiPoint3A location;                             // 00
		NiPoint3A normal;                               // 10
		NiPoint3A velocity;                             // 20
		NiPointer<bhkNPCollisionObject> collisionObject;  // 30
	};
	static_assert(sizeof(DamageImpactData) == 0x40);

	class VATSCommand;
	class HitData
	{
	public:
		static constexpr std::size_t OG_SIZE = 0xD8;
		static constexpr std::size_t AE_SIZE = 0xE0;

		[[nodiscard]] static constexpr std::size_t GetRuntimeSize(const REL::Version& a_version) noexcept
		{
			return REL::Relocate(a_version, OG_SIZE, AE_SIZE);
		}

		[[nodiscard]] static std::size_t GetRuntimeSize() noexcept
		{
			return REL::Relocate(OG_SIZE, AE_SIZE);
		}

		enum class Flag : std::uint32_t
		{
			kBlocked = 1U << 0,
			kBlockWithWeapon = 1U << 1,
			kBlockCandidate = 1U << 2,
			kCritical = 1U << 3,
			kCriticalOnDeath = 1U << 4,
			kFatal = 1U << 5,
			kDismemberLimb = 1U << 6,
			kExplodeLimb = 1U << 7,
			kCrippleLimb = 1U << 8,
			kDisarm = 1U << 9,
			kDisableWeapon = 1U << 10,
			kSneakAttack = 1U << 11,
			kIgnoreCritical = 1U << 12,
			kPredictDamage = 1U << 13,
			kPredictBaseDamage = 1U << 14,
			kBash = 1U << 15,
			kTimedBash = 1U << 16,
			kPowerAttack = 1U << 17,
			kMeleeAttack = 1U << 18,
			kRicochet = 1U << 19,
			kExplosion = 1U << 20
		};

		void SetAllDamageToZero()
		{
			flags.reset(
				Flag::kCritical,
				Flag::kCriticalOnDeath,
				Flag::kFatal,
				Flag::kDismemberLimb,
				Flag::kExplodeLimb,
				Flag::kCrippleLimb);
			healthDamage = 0.0F;
			targetedLimbDamage = 0.0F;
			resistedPhysicalDamage = 0.0F;
			stagger = static_cast<STAGGER_MAGNITUDE>(0);
		}

		// members
		DamageImpactData impactData;                                             // 00
		ActorHandle aggressor;                                                   // 40
		ActorHandle target;                                                      // 44
		ObjectRefHandle sourceRef;                                                // 48
		NiPointer<BGSAttackData> attackData;                                      // 50
		BGSObjectInstanceT<TESObjectWEAP> weapon;                                 // 58
		SpellItem* criticalEffect;                                                // 68
		SpellItem* hitEffect;                                                     // 70
		BSTSmartPointer<VATSCommand> vatsCommand;                                 // 78
		const TESAmmo* ammo;                                                      // 80
		BSTArray<BSTTuple<TESForm*, BGSTypedFormValuePair::SharedVal>>* damageTypes;  // 88
		float healthDamage;                                                       // 90
		float totalDamage;                                                        // 94
		float physicalDamage;                                                     // 98
		float targetedLimbDamage;                                                 // 9C
		float percentBlocked;                                                     // A0
		float resistedPhysicalDamage;                                             // A4
		float resistedTypedDamage;                                                // A8
		stl::enumeration<STAGGER_MAGNITUDE, std::uint32_t> stagger;               // AC
		float sneakAttackBonus;                                                   // B0
		float bonusHealthDamageMult;                                              // B4
		float pushBack;                                                           // B8
		float reflectedDamage;                                                    // BC
		float criticalDamageMult;                                                 // C0
		stl::enumeration<Flag, std::uint32_t> flags;                              // C4
		BGSEquipIndex equipIndex;                                                 // C8
		std::uint32_t material;                                                   // D0
		stl::enumeration<BGSBodyPartDefs::LIMB_ENUM, std::uint32_t> damageLimb;    // D4
	};
	static_assert(sizeof(HitData) == 0xE0);

	class TESHitEvent
	{
	public:
		static constexpr std::size_t OG_SIZE = 0x108;
		static constexpr std::size_t AE_SIZE = 0x110;

		[[nodiscard]] static constexpr std::size_t GetRuntimeSize(const REL::Version& a_version) noexcept
		{
			return REL::Relocate(a_version, OG_SIZE, AE_SIZE);
		}

		[[nodiscard]] static std::size_t GetRuntimeSize() noexcept
		{
			return REL::Relocate(OG_SIZE, AE_SIZE);
		}

		[[nodiscard]] static BSTEventSource<TESHitEvent>* GetEventSource()
		{
			using func_t = decltype(&TESHitEvent::GetEventSource);
			REL::Relocation<func_t> func{ REL::ID(1411899, 2201886) };
			return func();
		}

		// members
		HitData hitData;                    // 000
		NiPointer<TESObjectREFR> target;     // 0E0
		NiPointer<TESObjectREFR> cause;      // 0E8
		BSFixedString material;              // 0F0
		std::uint32_t sourceFormID;          // 0F8
		std::uint32_t projectileFormID;      // 0FC
		bool usesHitData;                    // 100
	};
	static_assert(sizeof(TESHitEvent) == 0x110);

	class HitEventSource : public BSTEventSource<TESHitEvent>
	{
	public:
		[[nodiscard]] static HitEventSource* GetSingleton()
		{
			return reinterpret_cast<HitEventSource*>(TESHitEvent::GetEventSource());
		}
	};

	class ObjectLoadedEventSource : public BSTEventSource<TESObjectLoadedEvent>
	{
	public:
		[[nodiscard]] static ObjectLoadedEventSource* GetSingleton()
		{
			return reinterpret_cast<ObjectLoadedEventSource*>(TESObjectLoadedEvent::GetEventSource());
		}
	};

	class EquipEventSource : public BSTEventSource<TESEquipEvent>
	{
	public:
		[[nodiscard]] static EquipEventSource* GetSingleton()
		{
			return reinterpret_cast<EquipEventSource*>(TESEquipEvent::GetEventSource());
		}
	};

	class MGEFApplyEventSource : public BSTEventSource<TESMagicEffectApplyEvent>
	{
	public:
		[[nodiscard]] static MGEFApplyEventSource* GetSingleton()
		{
			return reinterpret_cast<MGEFApplyEventSource*>(TESMagicEffectApplyEvent::GetEventSource());
		}
	};
}
