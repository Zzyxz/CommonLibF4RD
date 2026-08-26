#pragma once

namespace RE
{
	enum class LOCK_LEVEL;

	namespace GamePlayFormulas
	{
		inline bool CanPickLockGateCheck(LOCK_LEVEL a_lockLevel)
		{
			using func_t = decltype(&GamePlayFormulas::CanPickLockGateCheck);
			REL::Relocation<func_t> func{ REL::ID(1160841, 2209066) };
			return func(a_lockLevel);
		}

		inline float GetLockXPReward(LOCK_LEVEL a_lockLevel)
		{
			using func_t = decltype(&GamePlayFormulas::GetLockXPReward);
			REL::Relocation<func_t> func{ REL::ID(880926, 2209070) };
			return func(a_lockLevel);
		}
	}
}
