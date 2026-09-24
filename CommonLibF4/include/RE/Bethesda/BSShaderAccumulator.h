#pragma once

#include "RE/Bethesda/MemoryManager.h"
#include "RE/NetImmerse/NiAlphaAccumulator.h"
#include "RE/NetImmerse/NiColor.h"
#include "RE/NetImmerse/NiPoint3.h"
#include "RE/NetImmerse/NiSmartPointer.h"

namespace RE
{
	enum class BATCHRENDERER_CREATION_MODE : std::uint32_t
	{
		kSelectedObjectMask = 0x63
	};

	class ShadowSceneNode;

	class __declspec(novtable) BSShaderAccumulator :
		public NiAlphaAccumulator  // 000
	{
	public:
		static constexpr auto RTTI{ RTTI::BSShaderAccumulator };
		static constexpr auto VTABLE{ VTABLE::BSShaderAccumulator };
		static constexpr auto Ni_RTTI{ Ni_RTTI::BSShaderAccumulator };

		// The game uses 0x63 for the selected-object mask accumulator.
		inline static constexpr auto kSelectedObjectMaskCreationMode{ BATCHRENDERER_CREATION_MODE::kSelectedObjectMask };

		virtual void FinishAccumulatingPreResolveDepth() {}   // 2E
		virtual void FinishAccumulatingPostResolveDepth() {}  // 2F

		// The constructor initializes the complete game-owned object, including its
		// vtable. Keep instances in NiPointer and let the game vtable destroy them.
		BSShaderAccumulator(BATCHRENDERER_CREATION_MODE a_creationMode);

		F4_HEAP_REDEFINE_NEW(BSShaderAccumulator);

		// members
		std::uint32_t sunPixelCount;                 // 058
		bool waitingForSunQuery;                     // 05C
		std::byte padding5D[0x3];                    // 05D
		float percentSunOccludedStored;              // 060
		std::byte sunTests[0x48];                    // 068
		bool firstPerson;                            // 0B0
		bool zPrePass;                               // 0B1
		std::byte paddingB2[0x2];                    // 0B2
		NiColorA silhouetteColor;                    // 0B4
		bool renderDecals;                           // 0C4
		std::byte paddingC5[0x3];                    // 0C5
		std::byte batchRenderer[0x480];              // 0C8
		std::uint32_t currentPass;                   // 548
		std::uint32_t currentBucket;                 // 54C
		bool currentActiveA;                         // 550
		std::byte padding551[0x7];                   // 551
		ShadowSceneNode* activeShadowSceneNode;      // 558
		std::uint32_t renderMode;                    // 560
		std::byte padding564[0x4];                   // 564
		NiPointer<NiRefObject> shadowLight;          // 568 (game BSLight pointer)
		NiPoint3A eyePosition;                       // 570
		std::uint32_t depthPassIndex;                // 580
		std::byte padding584[0xC];                   // 584
	};
	static_assert(sizeof(BSShaderAccumulator) == 0x590);
	static_assert(offsetof(BSShaderAccumulator, shadowLight) == 0x568);
	static_assert(offsetof(BSShaderAccumulator, depthPassIndex) == 0x580);
}
