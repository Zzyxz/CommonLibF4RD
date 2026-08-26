#pragma once

#include "RE/Bethesda/TESObjectREFRs.h"

namespace RE
{
	class Actor;
	class BGSAttackData;

	namespace CombatAIFormulas
	{
		[[nodiscard]] inline float CalcFlankDistance(Actor* a_actor, Actor* a_target)
		{
			using func_t = decltype(&CombatAIFormulas::CalcFlankDistance);
			REL::Relocation<func_t> func{ REL::ID(626213, 2248148) };
			return func(a_actor, a_target);
		}

		[[nodiscard]] inline float CalcDistractChance(Actor* a_actor, Actor* a_target)
		{
			using func_t = decltype(&CombatAIFormulas::CalcDistractChance);
			REL::Relocation<func_t> func{ REL::ID(1410041, 2248149) };
			return func(a_actor, a_target);
		}

		[[nodiscard]] inline float CalcDistractDistance(Actor* a_actor, Actor* a_target)
		{
			using func_t = decltype(&CombatAIFormulas::CalcDistractDistance);
			REL::Relocation<func_t> func{ REL::ID(1316903, 2248150) };
			return func(a_actor, a_target);
		}

		[[nodiscard]] inline float CalcFlipThrowProbability(Actor* a_actor)
		{
			using func_t = decltype(&CombatAIFormulas::CalcFlipThrowProbability);
			REL::Relocation<func_t> func{ REL::ID(425596, 2248153) };
			return func(a_actor);
		}

		[[nodiscard]] inline std::uint32_t CalcThrowMaxTargets(Actor* a_actor)
		{
			using func_t = decltype(&CombatAIFormulas::CalcThrowMaxTargets);
			REL::Relocation<func_t> func{ REL::ID(771453, 2248157) };
			return func(a_actor);
		}

		[[nodiscard]] inline float CalcCoverFlankChance(Actor* a_actor)
		{
			using func_t = decltype(&CombatAIFormulas::CalcCoverFlankChance);
			REL::Relocation<func_t> func{ REL::ID(488364, 2248171) };
			return func(a_actor);
		}

		[[nodiscard]] inline float CalcMeleeAttackChance(Actor* a_actor, Actor* a_target)
		{
			using func_t = decltype(&CombatAIFormulas::CalcMeleeAttackChance);
			REL::Relocation<func_t> func{ REL::ID(641654, 2248191) };
			return func(a_actor, a_target);
		}

		[[nodiscard]] inline float CalcAttackChance(Actor* a_actor, Actor* a_target, BGSAttackData* a_attackData)
		{
			using func_t = decltype(&CombatAIFormulas::CalcAttackChance);
			REL::Relocation<func_t> func{ REL::ID(972966, 2248192) };
			return func(a_actor, a_target, a_attackData);
		}

		[[nodiscard]] inline float CalcRangedThrowChance(Actor* a_actor)
		{
			using func_t = decltype(&CombatAIFormulas::CalcRangedThrowChance);
			REL::Relocation<func_t> func{ REL::ID(39037, 2248230) };
			return func(a_actor);
		}

		[[nodiscard]] inline float CalcRangedGrenadeThrowChance(Actor* a_actor)
		{
			using func_t = decltype(&CombatAIFormulas::CalcRangedGrenadeThrowChance);
			REL::Relocation<func_t> func{ REL::ID(1355235, 2248231) };
			return func(a_actor);
		}
	}

	namespace CombatFormulas
	{
		[[nodiscard]] inline float GetWeaponDisplayAccuracy(const BGSObjectInstanceT<TESObjectWEAP>& a_weapon, Actor* a_actor)
		{
			using func_t = decltype(&CombatFormulas::GetWeaponDisplayAccuracy);
			REL::Relocation<func_t> func{ REL::ID(1137654, 2209049) };
			return func(a_weapon, a_actor);
		}

		[[nodiscard]] inline float GetWeaponDisplayDamage(const BGSObjectInstanceT<TESObjectWEAP>& a_weapon, const TESAmmo* a_ammo, float a_condition)
		{
			using func_t = decltype(&CombatFormulas::GetWeaponDisplayDamage);
			REL::Relocation<func_t> func{ REL::ID(1431014, 2209046) };
			return func(a_weapon, a_ammo, a_condition);
		}

		[[nodiscard]] inline float GetWeaponDisplayRange(const BGSObjectInstanceT<TESObjectWEAP>& a_weapon)
		{
			using func_t = decltype(&CombatFormulas::GetWeaponDisplayRange);
			REL::Relocation<func_t> func{ REL::ID(1324037, 2209047) };
			return func(a_weapon);
		}

		[[nodiscard]] inline float GetWeaponDisplayRateOfFire(const TESObjectWEAP& a_weapon, const TESObjectWEAP::InstanceData* a_data)
		{
			using func_t = decltype(&CombatFormulas::GetWeaponDisplayRateOfFire);
			REL::Relocation<func_t> func{ REL::ID(1403591, 2209048) };
			return func(a_weapon, a_data);
		}

		[[nodiscard]] inline float calcResistedPercentage(ActorValueInfo* av, float a2, float a3)
		{
			using func_t = decltype(&CombatFormulas::calcResistedPercentage);
			REL::Relocation<func_t> func{ REL::ID(420470, 2209007) };
			return func(av, a2, a3);
		}
	}
}
