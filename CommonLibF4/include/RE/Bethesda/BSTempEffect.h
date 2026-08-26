#pragma once

#include "RE/NetImmerse/NiObject.h"
class BGSSaveGameBuffer;

enum class TEMP_EFFECT_TYPE : std::uint32_t
{

};

namespace RE
{
	class __declspec(novtable) BSTempEffect :
		public NiObject  // 00
	{
	public:
		static constexpr auto RTTI{ RTTI::BSTempEffect };
		static constexpr auto VTABLE{ VTABLE::BSTempEffect };
		static constexpr auto Ni_RTTI{ Ni_RTTI::BSTempEffect };

		// add
		virtual void Initialize() { return; };
		virtual void Attach() { return; };
		virtual void Detach() { return; };
		virtual bool Update(float) { return false; };
		virtual NiAVObject* Get3D() { return nullptr; };
		virtual bool GetManagerHandlesSaveLoad() { return false; };
		virtual bool GetClearWhenCellIsUnloaded() { return false; };
		virtual TEMP_EFFECT_TYPE GetType() { return (TEMP_EFFECT_TYPE)7; };
		virtual void SaveGame(BGSSaveGameBuffer*) { return; };
		virtual void LoadGame(BGSLoadGameBuffer*) { return; };
		virtual void FinishLoadGame(BGSLoadGameBuffer*) { return; };
		virtual bool IsInterfaceEffect() { return false; };
		virtual void SetInterfaceEffect() { return; };
		virtual bool GetStackable() { return false; };
		virtual bool GetStackableMatch(BSTempEffect*) { return false; };
		virtual void Push() { return; };
		virtual void Pop() { return; };
		virtual void HandleDeferredDeleteImpl() { return; };

		// members
		std::uint32_t unk10;
		std::uint32_t pad14;
		std::uint64_t unk18;
		std::uint32_t unk20;
		bool active;
		std::uint32_t unk28;
		std::uint64_t unk30;
		std::uint64_t unk38;
	};
	static_assert(sizeof(BSTempEffect) == 0x40);
}
