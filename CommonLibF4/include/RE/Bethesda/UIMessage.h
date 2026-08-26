#pragma once

#include "RE/Bethesda/BSFixedString.h"
#include "RE/Bethesda/BSStringT.h"
#include "RE/Bethesda/MemoryManager.h"

namespace RE
{
	namespace Scaleform::GFx
	{
		class Event;
	}

	enum class UI_MESSAGE_RESULTS
	{
		kHandled,
		kIgnore,
		kPassOn
	};

	enum class UI_MESSAGE_TYPE
	{
		kUpdate,
		kShow,
		kReshow,
		kHide,
		kForceHide,
		kScaleformEvent,
		kUserEvent,
		kInventoryUpdate,
		kUserProfileChange,
		kMUStatusChange,
		kResumeCaching,
		kUpdateController,
		kChatterEvent,
		kTotal
	};

	class IUIMessageData;

	class __declspec(novtable) UIMessage
	{
	public:
		static constexpr auto RTTI{ RTTI::UIMessage };
		static constexpr auto VTABLE{ VTABLE::UIMessage };

		virtual ~UIMessage() = default;  // 00

		virtual IUIMessageData* QData() { return nullptr; }              // 02
		virtual const IUIMessageData* QData() const { return nullptr; }  // 01

		F4_HEAP_REDEFINE_NEW(UIMessage);

		// members
		BSFixedString menu;                                                               // 08
		stl::enumeration<UI_MESSAGE_TYPE, std::int32_t> type{ UI_MESSAGE_TYPE::kTotal };  // 10
	};
	static_assert(sizeof(UIMessage) == 0x18);

	class __declspec(novtable) IUIMessageData :
		public UIMessage  // 00
	{
	public:
		static constexpr auto RTTI{ RTTI::IUIMessageData };
		static constexpr auto VTABLE{ VTABLE::IUIMessageData };

		// override (UIMessage)
		IUIMessageData* QData() override { return this; }              // 02
		const IUIMessageData* QData() const override { return this; }  // 01
	};
	static_assert(sizeof(IUIMessageData) == 0x18);

	class __declspec(novtable) BSUIMessageData :
		public IUIMessageData  // 00
	{
	public:
		static constexpr auto RTTI{ RTTI::BSUIMessageData };
		static constexpr auto VTABLE{ VTABLE::BSUIMessageData };

		static void SendUIMessage(const BSFixedString& a_menuName, UI_MESSAGE_TYPE a_type, std::uint32_t a_data)
		{
			using func_t = decltype(&BSUIMessageData::SendUIMessage);
			REL::Relocation<func_t> func{ REL::ID(141019, 2284976) };
			return func(a_menuName, a_type, a_data);
		}

		static void SendUIStringMessage(const BSFixedString& a_menuName, UI_MESSAGE_TYPE a_type, const BSFixedString& a_fixedString)
		{
			using func_t = decltype(&BSUIMessageData::SendUIStringMessage);
			REL::Relocation<func_t> func{ REL::ID(1270833, 2284977) };
			return func(a_menuName, a_type, a_fixedString);
		}

		static void SendUIStringBoolMessage(const BSFixedString& a_menuName, UI_MESSAGE_TYPE a_type, const BSFixedString& a_fixedString, bool a_data)
		{
			using func_t = decltype(&BSUIMessageData::SendUIStringBoolMessage);
			REL::Relocation<func_t> func{ REL::ID(1257084, 2284978) };
			return func(a_menuName, a_type, a_fixedString, a_data);
		}

		static void SendUIStringUIntMessage(const BSFixedString& a_menuName, UI_MESSAGE_TYPE a_type, const BSFixedString& a_fixedString, std::uint32_t a_data)
		{
			using func_t = decltype(&BSUIMessageData::SendUIStringUIntMessage);
			REL::Relocation<func_t> func{ REL::ID(99795, 2284979) };
			return func(a_menuName, a_type, a_fixedString, a_data);
		}

		static void SendUIStringFloatMessage(const BSFixedString& a_menuName, UI_MESSAGE_TYPE a_type, const BSFixedString& a_fixedString, float a_data)
		{
			using func_t = decltype(&BSUIMessageData::SendUIStringFloatMessage);
			REL::Relocation<func_t> func{ REL::ID(524803, 2284980) };
			return func(a_menuName, a_type, a_fixedString, a_data);
		}

		static void SendUIBoolMessage(const BSFixedString& a_menuName, UI_MESSAGE_TYPE a_type, bool a_value)
		{
			using func_t = decltype(&BSUIMessageData::SendUIBoolMessage);
			REL::Relocation<func_t> func{ REL::ID(1388308, 2284981) };
			return func(a_menuName, a_type, a_value);
		}

		static void SendUIPtrMessage(const BSFixedString& a_menuName, UI_MESSAGE_TYPE a_type, void* a_data)
		{
			using func_t = decltype(&BSUIMessageData::SendUIPtrMessage);
			REL::Relocation<func_t> func{ REL::ID(1374542, 2284982) };
			return func(a_menuName, a_type, a_data);
		}

		// members
		BSStringT<char>* string{ nullptr };  // 18
		BSFixedString fixedString;           // 20
		union
		{
			std::uint32_t u;
			void* p{ nullptr };
			float f;
			bool b;
		} data;  // 28
	};
	static_assert(sizeof(BSUIMessageData) == 0x30);

	class __declspec(novtable) BSUIScaleformData :
		public IUIMessageData  // 00
	{
	public:
		static constexpr auto RTTI{ RTTI::BSUIScaleformData };
		static constexpr auto VTABLE{ VTABLE::BSUIScaleformData };

		// members
		Scaleform::GFx::Event* scaleformEvent{ nullptr };  // 18
	};
	static_assert(sizeof(BSUIScaleformData) == 0x20);
}
