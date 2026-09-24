#pragma once

#include "REL/Relocation.h"

namespace RE
{
	class ShadowSceneNode;

	class BSShaderManager
	{
	public:
		struct State
		{
			ShadowSceneNode* shadowSceneNode[5];
		};

		[[nodiscard]] static ShadowSceneNode* GetShadowSceneNode(std::int32_t a_index);
		static void SetRenderMode(std::uint32_t a_renderMode);
	};

	static_assert(offsetof(BSShaderManager::State, shadowSceneNode) == 0);
}
