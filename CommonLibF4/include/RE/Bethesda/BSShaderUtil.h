#pragma once

namespace RE
{
	class BSCullingProcess;
	class BSShaderAccumulator;
	class NiAVObject;
	class NiCamera;

	namespace BSShaderUtil
	{
		void AccumulateScene(NiCamera* a_camera, NiAVObject* a_scene, BSCullingProcess& a_cullingProcess, bool a_doAccumulation);
		void RenderScene(NiCamera* a_camera, BSShaderAccumulator* a_accumulator, bool a_doRender);
	}
}
