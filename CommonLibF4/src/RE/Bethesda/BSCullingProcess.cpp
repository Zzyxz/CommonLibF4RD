#include "RE/Bethesda/BSCullingProcess.h"

namespace RE
{
	BSCullingProcess::BSCullingProcess(NiVisibleArray* a_visibleSet)
	{
		using func_t = void(BSCullingProcess*, NiVisibleArray*);
		REL::Relocation<func_t> func{ REL::ID(423200, 2275928) };
		func(this, a_visibleSet);
	}

	BSCullingProcess::~BSCullingProcess()
	{
		using func_t = void(BSCullingProcess*);
		REL::Relocation<func_t> func{ REL::ID(769036, 2275929) };
		func(this);

		// The game destructor has already released the field. Prevent the C++
		// member destructor from releasing that game-owned reference a second time.
		*reinterpret_cast<NiAccumulator**>(std::addressof(accumulator)) = nullptr;
	}

	void BSCullingProcess::SetAccumulator(NiAccumulator* a_accumulator)
	{
		using func_t = void(BSCullingProcess*, NiAccumulator*);
		REL::Relocation<func_t> func{ REL::ID(236955, 2275930) };
		func(this, a_accumulator);
	}
}
