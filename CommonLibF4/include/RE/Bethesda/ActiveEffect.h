#pragma once

#include "RE/Bethesda/ActiveEffectReferenceEffectController.h"
#include "RE/Bethesda/BSSoundHandle.h"
#include "RE/Bethesda/BSTList.h"
#include "RE/Bethesda/TESForms.h"
#include "RE/NetImmerse/NiSmartPointer.h"

namespace RE
{
	class EffectItem;
	class MagicItem;
	class MagicTarget;
	class NiNode;
	class ReferenceEffect;
	class TESBoundObject;
	class TESForm;
	class TESObjectREFR;

	namespace MagicSystem
	{
		enum class CastingSource : std::int32_t;
	}

	class __declspec(novtable) ActiveEffect :
		public BSIntrusiveRefCounted
	{
	public:
		static constexpr auto RTTI{ RTTI::ActiveEffect };
		static constexpr auto VTABLE{ VTABLE::ActiveEffect };
		static constexpr auto FORM_ID{ ENUM_FORM_ID::kActiveEffect };

		bool CheckDisplacementSpellOnTarget()
		{
			using func_t = decltype(&ActiveEffect::CheckDisplacementSpellOnTarget);
			REL::Relocation<func_t> func{ REL::ID(1415178, 2226001) };
			return func(this);
		}

		enum class Flags : std::uint32_t
		{
			kNone = 0,
			kNoHitShader = 1U << 1,
			kNoHitEffectArt = 1U << 2,
			kNoInitialFlare = 1U << 4,
			kApplyingHitEffects = 1U << 5,
			kApplyingSounds = 1U << 6,
			kHasConditions = 1U << 7,
			kRecover = 1U << 9,
			kDualCasted = 1U << 12,
			kInactive = 1U << 15,
			kAppliedEffects = 1U << 16,
			kRemovedEffects = 1U << 17,
			kDispelled = 1U << 18,
			kWornOff = 1U << 31
		};

		enum class ConditionStatus : std::uint32_t
		{
			kNotAvailable = static_cast<std::underlying_type_t<ConditionStatus>>(-1),
			kFalse = 0,
			kTrue = 1
		};

		virtual ~ActiveEffect();

		[[nodiscard]] ReferenceEffectController& GetHitEffectController() noexcept
		{
			return static_cast<ReferenceEffectController&>(hitEffectController);
		}

		[[nodiscard]] const ReferenceEffectController& GetHitEffectController() const noexcept
		{
			return static_cast<const ReferenceEffectController&>(hitEffectController);
		}

		// members
		ActiveEffectReferenceEffectController hitEffectController;  // 10
		BSSoundHandle persistentSound;                               // 30
		ActorHandle caster;                                          // 38
		NiPointer<NiNode> sourceNode;                                // 40
		MagicItem* spell;                                            // 48
		EffectItem* effect;                                          // 50
		MagicTarget* target;                                         // 58
		TESBoundObject* source;                                      // 60
		BSSimpleList<ReferenceEffect*>* hitEffects;                  // 68
		MagicItem* displacementSpell;                                // 70
		float elapsedSeconds;                                        // 78
		float duration;                                              // 7C
		float magnitude;                                             // 80
		stl::enumeration<Flags, std::uint32_t> flags;                // 84
		stl::enumeration<ConditionStatus, std::uint32_t> conditionStatus;  // 88
		std::uint16_t uniqueID;                                      // 8C
		MagicSystem::CastingSource castingSource;                    // 90
	};
	static_assert(sizeof(ActiveEffect) == 0x98);
}
