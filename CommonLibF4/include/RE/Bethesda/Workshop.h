#pragma once

#include "RE/Bethesda/BSExtraData.h"
#include "RE/Bethesda/BSTArray.h"
#include "RE/Bethesda/MemoryManager.h"
#include "RE/NetImmerse/NiPoint3.h"

namespace RE
{
	template <class, class>
	class BSPointerHandleManagerInterface;

	template <class>
	class BSPointerHandleSmartPointer;

	class BGSConstructibleObject;
	class BGSKeyword;
	class TESBoundObject;
	class TESForm;
	class TESObjectCELL;
	class TESWorldSpace;

	template <class, class>
	struct BSTTuple;

	namespace PowerUtils
	{
		class PowerGrid;
	}

	namespace Workshop
	{
		struct BuildableAreaEvent;
		struct PlacementStatusEvent;

		struct ContextData
		{
		public:
			ContextData(Actor* a_actor)
			{
				// lookDir.y = 1.0f;
				zAngle = a_actor->data.angle.z;
				parentCell = a_actor->parentCell;
				worldSpace = a_actor->parentCell->worldSpace;
				a_actor->GetEyeVector(lookPos, lookDir, false);
			}

			// members
			NiPoint3 lookPos;           // 00
			NiPoint3 lookDir;           // 0C
			float zAngle;               // 18
			TESObjectCELL* parentCell;  // 20
			TESWorldSpace* worldSpace;  // 28
		};
		static_assert(sizeof(ContextData) == 0x30);

		struct DeletedItemInfo
		{
		public:
			~DeletedItemInfo() noexcept {}  // NOLINT(modernize-use-equals-default)

			// members
			std::uint32_t formID;  // 0
			std::uint32_t count;   // 4
		};
		static_assert(sizeof(DeletedItemInfo) == 0x8);

		class __declspec(novtable) ExtraData :
			public BSExtraData  // 00
		{
		public:
			static constexpr auto RTTI{ RTTI::Workshop__ExtraData };
			static constexpr auto VTABLE{ VTABLE::Workshop__ExtraData };
			static constexpr auto TYPE{ EXTRA_DATA_TYPE::kWorkshop };

			// members
			PowerUtils::PowerGrid* currentPowerGrid;            // 18
			BSTArray<PowerUtils::PowerGrid*> powerGrid;         // 20
			BSTArray<Workshop::DeletedItemInfo*> deletedItems;  // 38
			std::int32_t powerRating;                           // 50
			bool offGridItems;                                  // 54
		};
		static_assert(sizeof(ExtraData) == 0x58);

		struct ItemDestroyedEvent
		{
		public:
			// members
			NiPointer<TESObjectREFR> workshop;         // 00
			NiPointer<TESObjectREFR> objectDestroyed;  // 08
		};
		static_assert(sizeof(ItemDestroyedEvent) == 0x10);

		struct ItemMovedEvent
		{
		public:
			// members
			NiPointer<TESObjectREFR> workshop;   // 00
			NiPointer<TESObjectREFR> movedItem;  // 08
		};
		static_assert(sizeof(ItemMovedEvent) == 0x10);

		struct ItemPlacedEvent
		{
		public:
			// members
			NiPointer<TESObjectREFR> workshop;    // 00
			NiPointer<TESObjectREFR> placedItem;  // 08
		};
		static_assert(sizeof(ItemPlacedEvent) == 0x10);

		class alignas(0x10) PlacementItemData
		{
		public:
			void Set(TESObjectREFR& a_refr)
			{
				using func_t = decltype(&PlacementItemData::Set);
				REL::Relocation<func_t> func{ REL::ID(1460935, 2195643) };
				return func(this, a_refr);
			}

			// members
			ObjectRefHandle placementItem;         // 00
			BSTArray<hknpShape*> dropProxy;        // 08
			BSTArray<bhkNPCollisionObject*> body;  // 20
			std::byte pad38[0x80 - 0x38];          // 38 - TODO
			float sinkDepth;                       // 80
			float zOffset;                         // 84
			float intersectTolerance;              // 88
			float snapPointQueryRadius;            // 8C
			float forwardCastZRot;                 // 90
			float xyBoundOverride;                 // 94
			float xBoundMinOverride;               // 98
			float xBoundMaxOverride;               // 9C
			float yBoundMinOverride;               // A0
			float yBoundMaxOverride;               // A4
			float zBoundMinOverride;               // A8
			float zBoundMaxOverride;               // AC
			std::uint32_t dropProxyIndex;          // B0
			bool anythingIsGround;                 // B4
			bool clampUp;                          // B5
			bool forwardCast;                      // B6
			bool recenter;                         // B7
			bool ignoreSimpleIntersection;         // B8
			bool allowUnsupportedStacking;         // B9
			bool isSet;                            // BA
			bool mustSnap;                         // BB
			bool stacksWhenSnapped;                // BC
		};
		static_assert(sizeof(PlacementItemData) == 0xC0);

		class WorkshopMenuNode
		{
		public:
			~WorkshopMenuNode() { Clear(); }

			void Clear()
			{
				filterKeyword = nullptr;
				parent = nullptr;
				recipe = nullptr;
				row = 0;
				column = 0;
				selected = false;
				children.clear();
			}

			bool FindAndSetSelectedNode(std::uint16_t a_row, std::uint32_t a_crc, std::uint16_t& a_outRow)
			{
				using func_t = decltype(&WorkshopMenuNode::FindAndSetSelectedNode);
				REL::Relocation<func_t> func{ REL::ID(1309368, 2195022) };
				return func(this, a_row, a_crc, a_outRow);
			}

			F4_HEAP_REDEFINE_NEW(WorkshopMenuNode);

			// members
			BGSKeyword* filterKeyword{ nullptr };                      // 00
			WorkshopMenuNode* parent{ nullptr };                       // 08
			BSTArray<msvc::unique_ptr<WorkshopMenuNode>> children;     // 10
			BGSConstructibleObject* recipe{ nullptr };                 // 28
			BGSConstructibleObject* sourceFormListRecipe{ nullptr };   // 30
			TESForm* form{ nullptr };                                  // 38
			std::uint32_t uniqueID{ static_cast<std::uint32_t>(-1) };  // 40
			std::uint16_t row{ 0 };                                    // 44
			std::uint16_t column{ 0 };                                 // 46
			bool selected{ false };                                    // 48
		};
		static_assert(sizeof(WorkshopMenuNode) == 0x50);

		struct WorkshopModeEvent
		{
		public:
			// members
			NiPointer<TESObjectREFR> workshop;  // 00
			bool start;                         // 08
		};
		static_assert(sizeof(WorkshopModeEvent) == 0x10);

		[[nodiscard]] inline TESObjectREFR* FindNearestValidWorkshop(const TESObjectREFR& a_refr)
		{
			using func_t = decltype(&Workshop::FindNearestValidWorkshop);
			REL::Relocation<func_t> func{ REL::ID(905705, 2194970) };
			return func(a_refr);
		}

		[[nodiscard]] inline bool FreeBuild()
		{
			using func_t = decltype(&Workshop::FreeBuild);
			REL::Relocation<func_t> func{ REL::ID(1583365, 2194924) };
			return func();
		}

		[[nodiscard]] inline WorkshopMenuNode* GetSelectedWorkshopMenuNode(std::uint32_t a_row, std::uint32_t& a_column)
		{
			using func_t = decltype(&Workshop::GetSelectedWorkshopMenuNode);
			REL::Relocation<func_t> func{ REL::ID(763948, 2195024) };
			return func(a_row, a_column);
		}

		inline void InitializePlacementReference(const ContextData& a_context, TESBoundObject& a_object)
		{
			using func_t = decltype(&Workshop::InitializePlacementReference);
			REL::Relocation<func_t> func{ REL::ID(1577199, 2195201) };
			return func(a_context, a_object);
		}

		[[nodiscard]] inline bool IsLocationWithinBuildableArea(const TESObjectREFR& a_workshop, const NiPoint3& a_location)
		{
			using func_t = decltype(&Workshop::IsLocationWithinBuildableArea);
			REL::Relocation<func_t> func{ REL::ID(990965, 2194956) };
			return func(a_workshop, a_location);
		}

		inline bool PlaceCurrentReference(const ContextData& a_context)
		{
			using func_t = decltype(&Workshop::PlaceCurrentReference);
			REL::Relocation<func_t> func{ REL::ID(1058211, 2195155) };
			return func(a_context);
		}

		inline void RegisterForItemDestroyed(BSTEventSink<ItemDestroyedEvent>* a_sink)
		{
			using func_t = decltype(&Workshop::RegisterForItemDestroyed);
			REL::Relocation<func_t> func{ REL::ID(1367004, 2194930) };
			return func(a_sink);
		}

		inline void RegisterForItemMoved(BSTEventSink<ItemMovedEvent>* a_sink)
		{
			using func_t = decltype(&Workshop::RegisterForItemMoved);
			REL::Relocation<func_t> func{ REL::ID(835323, 2194932) };
			return func(a_sink);
		}

		inline void RegisterForItemPlaced(BSTEventSink<ItemPlacedEvent>* a_sink)
		{
			using func_t = decltype(&Workshop::RegisterForItemPlaced);
			REL::Relocation<func_t> func{ REL::ID(849008, 2194926) };
			return func(a_sink);
		}

		inline void RegisterForWorkshopModeEvent(BSTEventSink<WorkshopModeEvent>* a_sink)
		{
			using func_t = decltype(&Workshop::RegisterForWorkshopModeEvent);
			REL::Relocation<func_t> func{ REL::ID(275798, 2194946) };
			return func(a_sink);
		}

		inline void RequestExitWorkshop(bool a_allowReEntry)
		{
			using func_t = decltype(&Workshop::RequestExitWorkshop);
			REL::Relocation<func_t> func{ REL::ID(209891, 2195135) };
			return func(a_allowReEntry);
		}

		inline void ScrapReference(const ContextData& a_context, BSPointerHandleSmartPointer<BSPointerHandleManagerInterface<TESObjectREFR, HandleManager>>& a_scrapRef, BSTArray<BSTTuple<TESBoundObject*, std::uint32_t>>* a_rewards)
		{
			using func_t = decltype(&Workshop::ScrapReference);
			REL::Relocation<func_t> func{ REL::ID(636327, 2195125) };
			return func(a_context, a_scrapRef, a_rewards);
		}

		inline void SetSelectedEditItem(ObjectRefHandle a_refr)
		{
			using func_t = decltype(&Workshop::SetSelectedEditItem);
			REL::Relocation<func_t> func{ REL::ID(2562, 2195119) };
			return func(a_refr);
		}

		inline void StartWorkshop(TESObjectREFR* a_workshopRef)
		{
			using func_t = decltype(&Workshop::StartWorkshop);
			REL::Relocation<func_t> func{ REL::ID(171771, 2195134) };
			return func(a_workshopRef);
		}

		inline void ToggleEditMode(const ContextData& a_context)
		{
			using func_t = decltype(&Workshop::ToggleEditMode);
			REL::Relocation<func_t> func{ REL::ID(1243386, 2195138) };
			return func(a_context);
		}

		inline void UnregisterForItemDestroyed(BSTEventSink<ItemDestroyedEvent>* a_sink)
		{
			using func_t = decltype(&Workshop::UnregisterForItemDestroyed);
			REL::Relocation<func_t> func{ REL::ID(1101379, 2194931) };
			return func(a_sink);
		}

		inline void UnregisterForItemMoved(BSTEventSink<ItemMovedEvent>* a_sink)
		{
			using func_t = decltype(&Workshop::UnregisterForItemMoved);
			REL::Relocation<func_t> func{ REL::ID(569432, 2194933) };
			return func(a_sink);
		}

		inline void UnregisterForItemPlaced(BSTEventSink<ItemPlacedEvent>* a_sink)
		{
			using func_t = decltype(&Workshop::UnregisterForItemPlaced);
			REL::Relocation<func_t> func{ REL::ID(583255, 2194927) };
			return func(a_sink);
		}

		inline void UnregisterForWorkshopModeEvent(BSTEventSink<WorkshopModeEvent>* a_sink)
		{
			using func_t = decltype(&Workshop::UnregisterForWorkshopModeEvent);
			REL::Relocation<func_t> func{ REL::ID(9236, 2194947) };
			return func(a_sink);
		}

		inline void UpdateActiveEdit(const ContextData& a_context, bool a_multiselect = false)
		{
			using func_t = decltype(&Workshop::UpdateActiveEdit);
			REL::Relocation<func_t> func{ REL::ID(69261, 2195152) };
			return func(a_context, a_multiselect);
		}

		[[nodiscard]] inline bool WorkshopCanShowRecipe(BGSConstructibleObject* a_recipe, BGSKeyword* a_filter)
		{
			using func_t = decltype(&Workshop::WorkshopCanShowRecipe);
			REL::Relocation<func_t> func{ REL::ID(239190, 2194978) };
			return func(a_recipe, a_filter);
		}

		inline REL::Relocation<PlacementItemData*> CurrentPlacementItemData{ REL::ID(1279207, 4797261) };
		inline REL::Relocation<std::uint16_t*> CurrentRow{ REL::ID(833923, 4797160) };
		inline REL::Relocation<ObjectRefHandle*> PlacementItem{ REL::ID(526727, 4797235) };
	}
}
