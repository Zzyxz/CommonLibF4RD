#pragma once

#include "RE/NetImmerse/NiPoint3.h"

namespace RE
{
	class Actor;
	class ActorStance;
	class TESCombatStyle;

	class CombatBehaviorContextGrenade
	{
	public:
		[[nodiscard]] bool StartThrow()
		{
			using func_t = decltype(&CombatBehaviorContextGrenade::StartThrow);
			REL::Relocation<func_t> func{ REL::ID(134522, 2216044) };
			return func(this);
		}

		[[nodiscard]] bool CheckShouldThrow() const
		{
			using func_t = decltype(&CombatBehaviorContextGrenade::CheckShouldThrow);
			REL::Relocation<func_t> func{ REL::ID(905024, 2216045) };
			return func(this);
		}

		[[nodiscard]] float CalcThrowChance() const
		{
			using func_t = decltype(&CombatBehaviorContextGrenade::CalcThrowChance);
			REL::Relocation<func_t> func{ REL::ID(798257, 2216046) };
			return func(this);
		}
	};

	class CombatBehaviorContextFlankingMovement
	{
	public:
		[[nodiscard]] bool CheckShouldStalk() const
		{
			using func_t = decltype(&CombatBehaviorContextFlankingMovement::CheckShouldStalk);
			REL::Relocation<func_t> func{ REL::ID(1367868, 2242175) };
			return func(this);
		}

		[[nodiscard]] bool CheckShouldStop(float a_distance)
		{
			using func_t = decltype(&CombatBehaviorContextFlankingMovement::CheckShouldStop);
			REL::Relocation<func_t> func{ REL::ID(729475, 2242176) };
			return func(this, a_distance);
		}

		[[nodiscard]] bool CheckShouldDistract() const
		{
			using func_t = decltype(&CombatBehaviorContextFlankingMovement::CheckShouldDistract);
			REL::Relocation<func_t> func{ REL::ID(262881, 2242177) };
			return func(this);
		}
	};

	class CombatController
	{
	public:
		[[nodiscard]] bool IsActorACombatTarget(Actor* a_actor) const
		{
			using func_t = decltype(&CombatController::IsActorACombatTarget);
			REL::Relocation<func_t> func{ REL::ID(518009, 2216394) };
			return func(this, a_actor);
		}

		[[nodiscard]] bool IsFighting() const
		{
			using func_t = decltype(&CombatController::IsFighting);
			REL::Relocation<func_t> func{ REL::ID(131174, 2216397) };
			return func(this);
		}

		[[nodiscard]] bool IsFleeing() const
		{
			using func_t = decltype(&CombatController::IsFleeing);
			REL::Relocation<func_t> func{ REL::ID(1447512, 2216398) };
			return func(this);
		}

		[[nodiscard]] bool IsSearching() const
		{
			using func_t = decltype(&CombatController::IsSearching);
			REL::Relocation<func_t> func{ REL::ID(303881, 2216399) };
			return func(this);
		}

		void SetTarget(Actor* a_target)
		{
			using func_t = decltype(&CombatController::SetTarget);
			REL::Relocation<func_t> func{ REL::ID(369646, 2216401) };
			func(this, a_target);
		}

		[[nodiscard]] bool CanChangeTarget() const
		{
			using func_t = decltype(&CombatController::CanChangeTarget);
			REL::Relocation<func_t> func{ REL::ID(342075, 2216402) };
			return func(this);
		}

		void UpdateCurrentCombatStyle()
		{
			using func_t = decltype(&CombatController::UpdateCurrentCombatStyle);
			REL::Relocation<func_t> func{ REL::ID(646649, 2216407) };
			func(this);
		}

		void ClearStance()
		{
			using func_t = decltype(&CombatController::ClearStance);
			REL::Relocation<func_t> func{ REL::ID(1137081, 2216411) };
			func(this);
		}

		[[nodiscard]] bool CheckLOS(const ActorStance& a_stance, const NiPoint3& a_target) const
		{
			using func_t = bool (CombatController::*)(const ActorStance&, const NiPoint3&) const;
			REL::Relocation<func_t> func{ REL::ID(209471, 2216431) };
			return func(this, a_stance, a_target);
		}

		[[nodiscard]] bool CheckLOS(const ActorStance& a_stance, const NiPoint3& a_source, const NiPoint3& a_target) const
		{
			using func_t = bool (CombatController::*)(const ActorStance&, const NiPoint3&, const NiPoint3&) const;
			REL::Relocation<func_t> func{ REL::ID(1166354, 2216432) };
			return func(this, a_stance, a_source, a_target);
		}

		[[nodiscard]] bool ChooseBestMovementDirection(const NiPoint3& a_direction, const NiPoint3& a_target, float a_distance) const
		{
			using func_t = decltype(&CombatController::ChooseBestMovementDirection);
			REL::Relocation<func_t> func{ REL::ID(504458, 2216453) };
			return func(this, a_direction, a_target, a_distance);
		}

		[[nodiscard]] TESCombatStyle* GetCurrentCombatStyle() const noexcept
		{
			return *stl::adjust_pointer<TESCombatStyle*>(this, 0x80);
		}
	};
}
