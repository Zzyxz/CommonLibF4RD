#pragma once

#include "RE/Bethesda/BGSInventoryInterface.h"
#include "RE/Bethesda/BSFixedString.h"
#include "RE/Bethesda/BSInputEventUser.h"
#include "RE/Bethesda/BSPointerHandle.h"
#include "RE/Bethesda/BSSoundHandle.h"
#include "RE/Bethesda/BSTEvent.h"
#include "RE/Bethesda/BSTPoint.h"
#include "RE/Bethesda/BSTSingleton.h"
#include "RE/Bethesda/Inventory3DManager.h"
#include "RE/NetImmerse/NiSmartPointer.h"

namespace RE
{
	class BGSNote;
	class BGSTerminal;
	class BSTriShape;
	class TESBoundObject;
	class TESObjectREFR;

	struct BSAnimationGraphEvent;

	class IsPipboyActiveEvent :
		public BSTValueEvent<bool>  // 00
	{
	public:
	};
	static_assert(sizeof(IsPipboyActiveEvent) == 0x02);

	class PipboyRadioController
	{
	public:
		// members
		float chaseStation;  // 00
	};
	static_assert(sizeof(PipboyRadioController) == 0x04);

	class __declspec(novtable) PipboyManager :
		public BSTEventSink<BSAnimationGraphEvent>,  // 00
		public BSInputEventUser,                     // 08
		public BSTSingletonSDM<PipboyManager>        // 18
	{
	public:
		static constexpr auto RTTI{ RTTI::PipboyManager };
		static constexpr auto VTABLE{ VTABLE::PipboyManager };

		enum class LOWER_REASON
		{
			kNone = 0,
			kBook = 1,
			kPerkGrid = 2,
			kInspect = 3
		};

		virtual ~PipboyManager();  // 00

		// override
		virtual BSEventNotifyControl ProcessEvent(const BSAnimationGraphEvent& a_event, BSTEventSource<BSAnimationGraphEvent>* a_source) override;  // 01
		virtual bool ShouldHandleEvent(const InputEvent*) override;                                                                                 // 02
		virtual void HandleEvent(const ButtonEvent*) override;                                                                                    // 09
		virtual void HandleEvent(const MouseMoveEvent*) override;                                                                              // 07
		virtual void HandleEvent(const ThumbstickEvent*) override;                                                                            // 05

		[[nodiscard]] static PipboyManager* GetSingleton()
		{
			REL::Relocation<PipboyManager**> singleton{ REL::ID(553234, 4799238) };
			return *singleton;
		}

		void LowerPipboy(LOWER_REASON a_reason)
		{
			using func_t = decltype(&PipboyManager::LowerPipboy);
			REL::Relocation<func_t> func{ REL::ID(1444875, 2225454) };
			return func(this, a_reason);
		}

		void RaisePipboy()
		{
			using func_t = decltype(&PipboyManager::RaisePipboy);
			REL::Relocation<func_t> func{ REL::ID(726763, 2225455) };
			return func(this);
		}

		void RefreshPipboyRenderSurface()
		{
			using func_t = decltype(&PipboyManager::RefreshPipboyRenderSurface);
			REL::Relocation<func_t> func{ REL::ID(81339, 2225478) };
			return func(this);
		}

		void UpdateCursorConstraint(bool a_enable)
		{
			using func_t = decltype(&PipboyManager::UpdateCursorConstraint);
			REL::Relocation<func_t> func{ REL::ID(900802, 2225488) };
			return func(this, a_enable);
		}

		// members
		BSSoundHandle pipboyHumSound;                                 // 01C
		BSTPoint2<float> inputVector;                                 // 024
		BSTPoint2<float> newInput;                                    // 02C
		NiPointer<BSTriShape> debugModeGeom;                          // 038
		Inventory3DManager modelManager;                              // 040
		BGSNote* holotapeToLoad;                                      // 180
		BGSTerminal* terminalToLoad;                                  // 188
		BSFixedString menuToOpen;                                     // 190
		BSFixedString openAnimEvent;                                  // 198
		BSFixedString closeAnimEvent;                                 // 1A0
		stl::enumeration<LOWER_REASON, std::int32_t> loweringReason;  // 1A8
		PipboyRadioController radioController;                        // 1AC
		TESBoundObject* itemAnimOnClose;                              // 1B0
		ObjectRefHandle fastTravelLocation;                           // 1B8
		InventoryInterface::Handle inspectRequestItem;                // 1BC
		std::uint32_t inspectRequestStack;                            // 1C0
		bool pipboyExamineMode;                                       // 1C4
		bool pipboyExamineDirtyFlag;                                  // 1C5
		bool pipboyOpening;                                           // 1C6
		bool pipboyClosing;                                           // 1C7
		BSTValueEventSource<IsPipboyActiveEvent> pipboyActive;        // 1C8
		bool ignoreOpeningFlag;                                       // 1E0
		bool autoSaveOnClose;                                         // 1E1
		bool pipboyMenuCloseQueued;                                   // 1E2
		bool pipboyRaising;                                           // 1E3
		bool wasMotionBlurActive;                                     // 1E4
		bool wasPipboyLightActive;                                    // 1E5
	};
	static_assert(sizeof(PipboyManager) == 0x1F0);
}
