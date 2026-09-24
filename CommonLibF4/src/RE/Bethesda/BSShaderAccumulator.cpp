#include "RE/Bethesda/BSShaderAccumulator.h"

namespace RE
{
	BSShaderAccumulator::BSShaderAccumulator(BATCHRENDERER_CREATION_MODE a_creationMode)
	{
		using func_t = void(BSShaderAccumulator*, BATCHRENDERER_CREATION_MODE);
		REL::Relocation<func_t> func{ REL::ID(690952, 2317851) };
		func(this, a_creationMode);
	}
}
