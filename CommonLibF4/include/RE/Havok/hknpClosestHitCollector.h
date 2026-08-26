#pragma once

#include "RE/Havok/hkArray.h"
#include "RE/Havok/hknpCollisionQueryCollector.h"
#include "RE/Havok/hknpCollisionResult.h"

namespace RE
{
	class __declspec(novtable) hknpClosestHitCollector :
		public hknpCollisionQueryCollector  // 000
	{
	public:
		static constexpr auto RTTI{ RTTI::hknpClosestHitCollector };
		static constexpr auto VTABLE{ VTABLE::hknpClosestHitCollector };

		hknpClosestHitCollector()
		{
			stl::emplace_vtable(this);
			hints = 0;
			earlyOutThreshold.real = _mm_castsi128_ps(_mm_set1_epi32(0x5F7FFFF0));
			result.position.quad = _mm_setzero_ps();
			result.normal.quad = _mm_setzero_ps();
			result.fraction.storage = std::bit_cast<float>(0x7F7FFFEEu);
			result.queryBodyInfo.m_bodyId.value = 0x7FFFFFFFu;
			result.queryBodyInfo.m_shapeMaterialId.value = 0xFFFFu;
			result.queryBodyInfo.m_shapeKey.storage = 0xFFFFFFFFu;
			result.queryBodyInfo.m_shapeCollisionFilterInfo.storage = 0;
			result.queryBodyInfo.m_shapeUserData.storage = 0;
			result.hitBodyInfo.m_bodyId.value = 0x7FFFFFFFu;
			result.hitBodyInfo.m_shapeMaterialId.value = 0xFFFFu;
			result.hitBodyInfo.m_shapeKey.storage = 0xFFFFFFFFu;
			result.hitBodyInfo.m_shapeCollisionFilterInfo.storage = 0;
			result.hitBodyInfo.m_shapeUserData.storage = 0;
			result.hitResult.storage = 0;
			hasHit = false;
		}

		// override (hknpCollisionQueryCollector)
		void Reset() override;                                // 01
		void AddHit(const hknpCollisionResult&) override;     // 02
		bool HasHit() const override;                         // 03
		std::int32_t GetNumHits() const override;             // 04
		const hknpCollisionResult* GetHits() const override;  // 05

		// members
		hknpCollisionResult result;  //0x20
		bool hasHit;                 //0x80
		int8_t pad[15];
	};
	static_assert(sizeof(hknpClosestHitCollector) == 0x90);
}
