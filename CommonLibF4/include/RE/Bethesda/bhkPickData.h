#pragma once

#include "RE/Bethesda/bhkCharacterController.h"
#include "RE/Bethesda/CFilter.h"
#include "RE/Bethesda/MemoryManager.h"
#include "RE/Havok/hkVector4.h"
#include "RE/Havok/hknpCollisionResult.h"
#include "RE/Havok/hknpShape.h"
#include "RE/NetImmerse/NiAVObject.h"
#include "RE/NetImmerse/NiPoint3.h"
#include "REL/Relocation.h"

namespace RE
{
	class hknpBody;
	class hknpBSWorld;
	class hknpCollisionQueryCollector;
	class NiAVObject;

	using hknpRayCastQueryResult = hknpCollisionResult;

	class bhkPickData
	{
	public:
		bhkPickData()
		{
			using func_t = bhkPickData*(bhkPickData*);
			static REL::Relocation<func_t> func{ REL::ID(526783, 2230668) };
			func(this);
		}

		~bhkPickData()
		{
			using func_t = void(bhkPickData*);
			static REL::Relocation<func_t> func{ REL::ID(109425, 2228517) };
			func(this);
		}

		bhkPickData(const bhkPickData&) = delete;
		bhkPickData(bhkPickData&&) = delete;
		bhkPickData& operator=(const bhkPickData&) = delete;
		bhkPickData& operator=(bhkPickData&&) = delete;

		void SetStartEnd(const NiPoint3& a_start, const NiPoint3& a_end)
		{
			using func_t = void(bhkPickData*, const NiPoint3&, const NiPoint3&);
			static REL::Relocation<func_t> func{ REL::ID(747470, 2236622) };
			func(this, a_start, a_end);
		}

		void Reset()
		{
			using func_t = void(bhkPickData*);
			static REL::Relocation<func_t> func{ REL::ID(438299, 2277761) };
			func(this);
		}

		[[nodiscard]] hknpBody* GetBody()
		{
			using func_t = hknpBody*(bhkPickData*);
			static REL::Relocation<func_t> func{ REL::ID(1223055, 2277762) };
			return func(this);
		}

		[[nodiscard]] NiAVObject* GetNiAVObject()
		{
			using func_t = NiAVObject*(bhkPickData*);
			static REL::Relocation<func_t> func{ REL::ID(863406, 2277763) };
			return func(this);
		}

		[[nodiscard]] NiAVObject* GetNiAVObject(const hknpRayCastQueryResult& a_result)
		{
			using func_t = NiAVObject*(bhkPickData*, const hknpRayCastQueryResult&);
			static REL::Relocation<func_t> func{ REL::ID(597770, 2277764) };
			return func(this, a_result);
		}

		[[nodiscard]] std::int32_t GetAllCollectorRayHitSize()
		{
			using func_t = std::int32_t(bhkPickData*);
			static REL::Relocation<func_t> func{ REL::ID(1288513, 2277765) };
			return func(this);
		}

		[[nodiscard]] bool GetAllCollectorRayHitAt(std::uint32_t a_index, hknpRayCastQueryResult& a_result)
		{
			using func_t = bool(bhkPickData*, std::uint32_t, hknpRayCastQueryResult&);
			static REL::Relocation<func_t> func{ REL::ID(583997, 2277766) };
			return func(this, a_index, a_result);
		}

		void SortAllCollectorHits()
		{
			using func_t = void(bhkPickData*);
			static REL::Relocation<func_t> func{ REL::ID(1274842, 2277767) };
			func(this);
		}

		void AddAllCollectorRayHit(const hknpCollisionResult& a_result)
		{
			using func_t = void(bhkPickData*, const hknpCollisionResult&);
			static REL::Relocation<func_t> func{ REL::ID(570194, 2277768) };
			func(this, a_result);
		}

		void SetHitFraction(float a_fraction)
		{
			using func_t = void(bhkPickData*, float);
			static REL::Relocation<func_t> func{ REL::ID(303954, 2277769) };
			func(this, a_fraction);
		}

		[[nodiscard]] bool HasHit() const
		{
			using func_t = bool(const bhkPickData*);
			static REL::Relocation<func_t> func{ REL::ID(1181584, 2277770) };
			return func(this);
		}

		[[nodiscard]] std::uint32_t GetHitMaterialID()
		{
			if (!HasHit()) {
				return 0;
			}

			auto* object = GetNiAVObject();
			auto* collisionObject = object ? object->collisionObject.get() : nullptr;
			auto* bhkObject = collisionObject ? collisionObject->IsbhkNPCollisionObject() : nullptr;
			auto* shape = bhkObject ? bhkObject->GetShape() : nullptr;

			return shape ? shape->GetMaterialID(result.hitBodyInfo.m_shapeKey.storage) : 0;
		}

		[[nodiscard]] float GetHitFraction() const
		{
			using func_t = float(const bhkPickData*);
			static REL::Relocation<func_t> func{ REL::ID(476687, 2277772) };
			return func(this);
		}

		[[nodiscard]] bool IsHitTriggerVolume(const hknpRayCastQueryResult& a_result) const
		{
			using func_t = bool(const bhkPickData*, const hknpRayCastQueryResult&);
			static REL::Relocation<func_t> func{ REL::ID(117459, 2277773) };
			return func(this, a_result);
		}

		[[nodiscard]] bool IsMotionDynamic(const hknpRayCastQueryResult& a_result) const
		{
			using func_t = bool(const bhkPickData*, const hknpRayCastQueryResult&);
			static REL::Relocation<func_t> func{ REL::ID(901806, 2277774) };
			return func(this, a_result);
		}

		void InitializeWithCollisionFilter(
			std::uint32_t a_collisionFilter,
			NiPoint3& a_location,
			NiPoint3& a_aimVector,
			float a_range)
		{
			using func_t = decltype(&bhkPickData::InitializeWithCollisionFilter);
			REL::Relocation<func_t> func{ REL::ID(1464124, 2201310) };
			func(this, a_collisionFilter, a_location, a_aimVector, a_range);
		}

		void bhkPickData_CFilter(
			std::uint32_t a_collisionFilter,
			NiPoint3& a_location,
			NiPoint3& a_aimVector,
			float a_range)
		{
			InitializeWithCollisionFilter(a_collisionFilter, a_location, a_aimVector, a_range);
		}

		[[nodiscard]] static std::uint32_t ReadU32(const void* a_data) noexcept
		{
			std::uint32_t value;
			std::memcpy(std::addressof(value), a_data, sizeof(value));
			return value;
		}

		[[nodiscard]] std::uint32_t GetHitCollisionFilterInfo() noexcept
		{
			auto* body = GetBody();
			if (!body) {
				return 0;
			}

			const auto* base = reinterpret_cast<const std::uint8_t*>(body);
			return ReadU32(base + 0x44);
		}

		[[nodiscard]] std::uint8_t GetHitCollisionLayer() noexcept
		{
			return static_cast<std::uint8_t>(GetHitCollisionFilterInfo() & 0xFF);
		}

		[[nodiscard]] std::uint32_t GetHitCollisionFilterInfo_FO4() noexcept
		{
			return GetHitCollisionFilterInfo();
		}

		[[nodiscard]] std::uint8_t GetHitCollisionLayer8_FO4() noexcept
		{
			return GetHitCollisionLayer();
		}

		F4_HEAP_REDEFINE_NEW(bhkPickData);

		// members
		std::uint64_t field_00;                        // 00
		std::uint16_t field_08;                        // 08
		CFilter collisionFilter;                       // 0C
		std::uint64_t field_10;                        // 10
		std::uint32_t field_18;                        // 18
		hkVector4f rayOrigin;                          // 20
		hkVector4f rayDestination;                     // 30
		std::byte field_40[0x10];                      // 40
		std::int32_t field_50;                         // 50
		hknpRayCastQueryResult result;                 // 60
		hknpBSWorld* castWorld;                        // C0
		std::uint64_t customCollideLayers;             // C8
		hknpCollisionQueryCollector* collector;        // D0
		std::int32_t collectorType;                    // D8
		std::int16_t field_DC;                         // DC
		bool allowFailedPicks;                          // DE
		bool pickFailed;                                // DF
	};
	static_assert(sizeof(bhkPickData) == 0xE0);
}
