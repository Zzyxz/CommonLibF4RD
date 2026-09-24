#include "RE/Bethesda/BSShaderManager.h"

namespace RE
{
	ShadowSceneNode* BSShaderManager::GetShadowSceneNode(const std::int32_t a_index)
	{
		if (REL::Module::get().is_og()) {
			using func_t = ShadowSceneNode* (*)(std::int32_t);
			static REL::Relocation<func_t> func{
				REL::ID(100851, REL::ID::INVALID_ID, REL::ID::INVALID_ID) };
			return func(a_index);
		}

		static REL::Relocation<State*> state{
			REL::ID(REL::ID::INVALID_ID, 2712479, 2712479) };
		return state->shadowSceneNode[a_index];
	}

	void BSShaderManager::SetRenderMode(const std::uint32_t a_renderMode)
	{
		using func_t = void (*)(std::uint32_t);
		static REL::Relocation<func_t> func{
			REL::ID(1440976, 2316551, 2316551) };
		func(a_renderMode);
	}
}
