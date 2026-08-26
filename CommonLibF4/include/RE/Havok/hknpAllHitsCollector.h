#pragma once

#include "RE/Havok/hkArray.h"
#include "RE/Havok/hknpCollisionQueryCollector.h"
#include "RE/Havok/hknpCollisionResult.h"

namespace RE
{
	class __declspec(novtable) hknpAllHitsCollector :
		public hknpCollisionQueryCollector  // 000
	{
	public:
		static constexpr auto RTTI{ RTTI::hknpAllHitsCollector };
		static constexpr auto VTABLE{ VTABLE::hknpAllHitsCollector };

		hknpAllHitsCollector()
		{
			stl::emplace_vtable(this);
			hints = 0;
			hits._data = reinterpret_cast<hknpCollisionResult*>(reinterpret_cast<std::uintptr_t>(this) + 0x30);
			hits._size = 0;
			hits._capacityAndFlags = static_cast<std::int32_t>(0x8000000A);
			Reset();
		}

		// override (hknpCollisionQueryCollector)
		void Reset() override                                // 01
		{
			using func_t = void(hknpAllHitsCollector*);
			static REL::Relocation<func_t> func{ REL::ID(1360564, 2189457) };
			return func(this);
		}
		void AddHit(const hknpCollisionResult&) override;     // 02
		bool HasHit() const override                          // 03
		{
			using func_t = bool(const hknpAllHitsCollector*);
			static REL::Relocation<func_t> func{ REL::ID(1336136, 2189455) };
			return func(this);
		}
		std::int32_t GetNumHits() const override              // 04
		{
			using func_t = std::int32_t(const hknpAllHitsCollector*);
			static REL::Relocation<func_t> func{ REL::ID(738949, 2189453) };
			return func(this);
		}
		const hknpCollisionResult* GetHits() const override   // 05
		{
			using func_t = const hknpCollisionResult*(const hknpAllHitsCollector*);
			static REL::Relocation<func_t> func{ REL::ID(1270689, 2714317) };
			return func(this);
		}

		// members
		hkInplaceArray<hknpCollisionResult, 10> hits;  // 020
	};
	static_assert(sizeof(hknpAllHitsCollector) == 0x3F0);
}
