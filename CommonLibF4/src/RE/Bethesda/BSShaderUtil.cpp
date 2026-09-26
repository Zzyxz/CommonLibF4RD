#include "RE/Bethesda/BSShaderUtil.h"

#include "RE/Bethesda/BSCullingProcess.h"
#include "RE/Bethesda/BSShaderAccumulator.h"
#include "RE/NetImmerse/NiAVObject.h"
#include "RE/NetImmerse/NiCamera.h"

namespace RE::BSShaderUtil
{
	void AccumulateScene(NiCamera* a_camera, NiAVObject* a_scene, BSCullingProcess& a_cullingProcess, bool a_doAccumulation)
	{
		using func_t = void(NiCamera*, NiAVObject*, BSCullingProcess&, bool);
		REL::Relocation<func_t> func{ REL::ID(1551978, 2317574) };
		func(a_camera, a_scene, a_cullingProcess, a_doAccumulation);
	}

	void RenderScene(NiCamera* a_camera, BSShaderAccumulator* a_accumulator, bool a_useJitter)
	{
		using func_t = void(NiCamera*, BSShaderAccumulator*, bool);
		REL::Relocation<func_t> func{ REL::ID(1310228, 2317576) };
		func(a_camera, a_accumulator, a_useJitter);
	}
}
