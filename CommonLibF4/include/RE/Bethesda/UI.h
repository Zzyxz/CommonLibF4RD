#pragma once

#include "RE/Bethesda/Atomic.h"
#include "RE/Bethesda/BSFixedString.h"
#include "RE/Bethesda/BSInputEventReceiver.h"
#include "RE/Bethesda/BSLock.h"
#include "RE/Bethesda/BSTArray.h"
#include "RE/Bethesda/BSTEvent.h"
#include "RE/Bethesda/BSTHashMap.h"
#include "RE/Bethesda/BSTSingleton.h"
#include "RE/Bethesda/BSTimer.h"
#include "RE/Bethesda/IMenu.h"
#include "RE/Scaleform/GFx/GFx_Player.h"

namespace RE
{
	enum class UI_MESSAGE_TYPE;

	class MenuModeChangeEvent;
	class MenuModeCounterChangedEvent;
	class MenuOpenCloseEvent;
	class TutorialEvent;
	class UIMessage;

	struct UIMenuEntry
	{
	public:
		using Create_t = IMenu*(const UIMessage&);
		using StaticUpdate_t = void();

		// members
		Scaleform::Ptr<IMenu> menu;               // 00
		Create_t* create;                         // 08
		StaticUpdate_t* staticUpdate{ nullptr };  // 10
	};
	static_assert(sizeof(UIMenuEntry) == 0x18);

	class __declspec(novtable) UI :
		public BSInputEventReceiver,                         // 000
		public BSTSingletonSDM<UI>,                          // 010
		public BSTEventSource<MenuOpenCloseEvent>,           // 018
		public BSTEventSource<MenuModeChangeEvent>,          // 070
		public BSTEventSource<MenuModeCounterChangedEvent>,  // 0C8
		public BSTEventSource<TutorialEvent>                 // 120
	{
	public:
		static constexpr std::size_t OG_SIZE = 0x250;
		static constexpr std::size_t NG_SIZE = 0x250;
		static constexpr std::size_t AE_SIZE = 0x260;
		static constexpr std::size_t OG_RUNTIME_DATA_OFFSET = 0x248;
		static constexpr std::size_t NG_RUNTIME_DATA_OFFSET = 0x248;
		static constexpr std::size_t AE_RUNTIME_DATA_OFFSET = 0x258;
		static constexpr auto RTTI{ RTTI::UI };
		static constexpr auto VTABLE{ VTABLE::UI };

		using Create_t = typename UIMenuEntry::Create_t;
		using StaticUpdate_t = typename UIMenuEntry::StaticUpdate_t;

		class RuntimeData
		{
		public:
			bool menuSystemVisible;           // 00
			bool closingAllMenus;             // 01
			bool freezeFrameScreenshotReady;  // 02
		};
		static_assert(sizeof(RuntimeData) == 0x3);

		// add
		virtual ~UI() = default;  // 01

		[[nodiscard]] static constexpr std::size_t GetRuntimeDataOffset(const REL::Version& a_version) noexcept
		{
			return REL::Relocate(
				a_version,
				OG_RUNTIME_DATA_OFFSET,
				NG_RUNTIME_DATA_OFFSET,
				AE_RUNTIME_DATA_OFFSET);
		}

		[[nodiscard]] static std::size_t GetRuntimeDataOffset() noexcept
		{
			return GetRuntimeDataOffset(REL::Module::get().version());
		}

		[[nodiscard]] static constexpr std::size_t GetRuntimeSize(const REL::Version& a_version) noexcept
		{
			return REL::Relocate(a_version, OG_SIZE, NG_SIZE, AE_SIZE);
		}

		[[nodiscard]] static std::size_t GetRuntimeSize() noexcept
		{
			return GetRuntimeSize(REL::Module::get().version());
		}

		[[nodiscard]] RuntimeData& GetRuntimeData() noexcept
		{
			return REL::RelocateMember<RuntimeData>(
				this,
				OG_RUNTIME_DATA_OFFSET,
				NG_RUNTIME_DATA_OFFSET,
				AE_RUNTIME_DATA_OFFSET);
		}

		[[nodiscard]] const RuntimeData& GetRuntimeData() const noexcept
		{
			return REL::RelocateMember<RuntimeData>(
				this,
				OG_RUNTIME_DATA_OFFSET,
				NG_RUNTIME_DATA_OFFSET,
				AE_RUNTIME_DATA_OFFSET);
		}

		template <class T>
		[[nodiscard]] BSTEventSource<T>* GetEventSource()
		{
			return static_cast<RE::BSTEventSource<T>*>(this);
		}

		[[nodiscard]] static BSReadWriteLock& GetMenuMapRWLock()
		{
			REL::Relocation<BSReadWriteLock*> menuMapRWLock{ REL::ID(578487, 2707105) };
			return *menuMapRWLock;
		}

		[[nodiscard]] static UI* GetSingleton()
		{
			REL::Relocation<UI**> singleton{ REL::ID(548587, 4796314) };
			return *singleton;
		}

		[[nodiscard]] Scaleform::Ptr<IMenu> GetMenu(const BSFixedString& a_name) const
		{
			BSAutoReadLock l{ GetMenuMapRWLock() };
			const auto it = menuMap.find(a_name);
			return it != menuMap.end() ? it->second.menu : nullptr;
		}

		template <class T>
		[[nodiscard]] Scaleform::Ptr<T> GetMenu() const  //
			requires(requires { T::MENU_NAME; })
		{
			const auto ptr = GetMenu(T::MENU_NAME);
			return Scaleform::Ptr{ static_cast<T*>(ptr.get()) };
		}

		[[nodiscard]] bool GetMenuOpen(const BSFixedString& a_name) const
		{
			const auto menu = GetMenu(a_name);
			return menu ? menu->OnStack() : false;
		}

		void RefreshCursor()
		{
			using func_t = decltype(&UI::RefreshCursor);
			REL::Relocation<func_t> func{ REL::ID(1436639, 2284772) };
			return func(this);
		}

		void RegisterMenu(const char* a_menu, Create_t* a_create, StaticUpdate_t* a_staticUpdate = nullptr)
		{
			using func_t = decltype(&UI::RegisterMenu);
			REL::Relocation<func_t> func{ REL::ID(1519575, 2284766) };
			return func(this, a_menu, a_create, a_staticUpdate);
		}

		template <class T>
		void RegisterSink(BSTEventSink<T>* a_sink)
		{
			GetEventSource<T>()->RegisterSink(a_sink);
		}

		void UpdateControllerType()
		{
			using func_t = decltype(&UI::UpdateControllerType);
			REL::Relocation<func_t> func{ REL::ID(175796, 2284768) };
			return func(this);
		}

		template <class T>
		void UnregisterSink(BSTEventSink<T>* a_sink)
		{
			GetEventSource<T>()->UnregisterSink(a_sink);
		}

		// members
		BSTArray<BSFixedString> releasedMovies;            // 178
		BSTArray<Scaleform::Ptr<IMenu>> menuStack;         // 190
		BSTHashMap<BSFixedString, UIMenuEntry> menuMap;    // 1A8
		BSSpinLock processMessagesLock;                    // 1D8
		std::uint32_t menuMode;                            // 1E0
		BSTAtomicValue<std::uint32_t> itemMenuMode;        // 1E4
		BSTAtomicValue<std::uint32_t> pauseMenuDisableCt;  // 1E8
		std::uint32_t freezeFrameMenuBG;                   // 1EC
		std::uint32_t freezeFramePause;                    // 1F0
		std::uint32_t savingDisabled;                      // 1F4
		std::uint32_t disablesCompanion;                   // 1F8
		std::uint32_t largeCacheRenderModeCount;           // 1FC
		std::uint32_t movementToDirectionalCount;          // 200
		BSTimer uiTimer;                                   // 208
	};
	static_assert(sizeof(UI) == 0x228);

	namespace UIUtils
	{
		inline void UpdateGamepadDependentButtonCodes(bool a_usingGamepad)
		{
			using func_t = decltype(&UpdateGamepadDependentButtonCodes);
			REL::Relocation<func_t> func{ REL::ID(190238, 4483350) };
			return func(a_usingGamepad);
		}
	}
}
