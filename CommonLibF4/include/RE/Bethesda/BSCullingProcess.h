#pragma once

#include "RE/NetImmerse/NiAccumulator.h"
#include "RE/NetImmerse/NiCullingProcess.h"
#include "RE/NetImmerse/NiSmartPointer.h"

namespace RE
{
	class BSMultiBound;
	class BSOcclusionPlane;
	class BSPortalGraphEntry;
	class BSCompoundFrustum;

	class __declspec(novtable) alignas(0x08) BSCullingProcess :
		public NiCullingProcess  // 00
	{
	public:
		static constexpr auto RTTI{ RTTI::BSCullingProcess };
		static constexpr auto VTABLE{ VTABLE::BSCullingProcess };
		static constexpr auto Ni_RTTI{ Ni_RTTI::BSCullingProcess };

		enum class CullingType : std::uint32_t
		{
			kNormal = 0x0,
			kAllPass = 0x1,
			kAllFail = 0x2,
			kIgnoreMultiBounds = 0x3,
			kForceMultiBoundsNoUpdate = 0x4
		};

		BSCullingProcess(NiVisibleArray* a_visibleSet);
		~BSCullingProcess() override;

		virtual void AppendNonAccum(NiAVObject*) {}                    // 1C
		virtual void TestBaseVisibility(BSMultiBound*) {}              // 1D
		virtual void TestBaseVisibility(BSOcclusionPlane*) {}          // 1E
		virtual void TestBaseVisibility(const NiBound*) {}              // 1F

		void SetAccumulator(NiAccumulator* a_accumulator);

		// members
		std::byte roomSharedMap[0x30];           // 120
		BSPortalGraphEntry* portalGraphEntry;    // 150
		CullingType cullMode;                   // 158
		CullingType typeStack[10];               // 15C
		std::uint32_t CTStackIndex;              // 184
		BSCompoundFrustum* compoundFrustum;      // 188
		NiPointer<NiAccumulator> accumulator;    // 190
		bool recurseToGeometry;                  // 198
		std::byte padding[0x7];                  // 199
	};
	static_assert(sizeof(BSCullingProcess) == 0x1A0);
	static_assert(offsetof(BSCullingProcess, accumulator) == 0x190);
	static_assert(offsetof(BSCullingProcess, recurseToGeometry) == 0x198);
}
