#pragma once

#include "RE/NetImmerse/NiAVObject.h"
#include "RE/NetImmerse/NiFrustum.h"
#include "RE/NetImmerse/NiRect.h"

namespace RE
{
	class __declspec(novtable) NiCamera :
		public NiAVObject  // 00
	{
	public:
		static constexpr auto RTTI{ RTTI::NiCamera };
		static constexpr auto VTABLE{ VTABLE::NiCamera };
		static constexpr auto Ni_RTTI{ Ni_RTTI::NiCamera };

		virtual ~NiCamera();  // 00

		// override
		virtual const NiRTTI* GetRTTI() const override;                         // 02
		virtual NiObject* CreateClone(NiCloningProcess& a_cloneData) override;  // 1A
		virtual void LoadBinary(NiStream& a_stream) override;                   // 1B
		virtual void LinkObject(NiStream& a_stream) override;                   // 1C
		virtual bool RegisterStreamables(NiStream& a_stream) override;          // 1D
		virtual void SaveBinary(NiStream& a_stream) override;                   // 1E
		virtual bool IsEqual(NiObject* a_object) override;                      // 1F
		virtual void UpdateWorldBound() override;                               // 33
		virtual void UpdateWorldData(NiUpdateData* a_data) override;            // 34

		// Projects a world point through worldToCam into the camera port (0..1 with
		// the default port, y up).  Fails for points at or behind a_zeroTolerance
		// depth and for points outside the view frustum.
		bool WorldPtToScreenPt(const NiPoint3& a_point, float& a_x, float& a_y, float a_zeroTolerance = 0.0F) const
		{
			using func_t = decltype(&NiCamera::WorldPtToScreenPt);
			REL::Relocation<func_t> func{ REL::ID(855369, 2270341) };
			return func(this, a_point, a_x, a_y, a_zeroTolerance);
		}

		// members
		float worldToCam[4][4];  // 120
		NiFrustum viewFrustum;   // 160
		float minNearPlaneDist;  // 17C
		float maxFarNearRatio;   // 180
		NiRect<float> port;      // 184
		float lodAdjust;         // 194
	};
	static_assert(sizeof(NiCamera) == 0x1A0);
}
