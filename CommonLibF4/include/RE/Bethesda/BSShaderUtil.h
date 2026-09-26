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
		// a_useJitter applies the TAA sub-pixel jitter to the camera state.
		void RenderScene(NiCamera* a_camera, BSShaderAccumulator* a_accumulator, bool a_useJitter);
	}
}
