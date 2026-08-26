#pragma once

#include "RE/Bethesda/BSFixedString.h"
#include "RE/Bethesda/BSTArray.h"
#include "RE/Bethesda/BSTTuple.h"
#include "RE/Scaleform/GFx/GFx_Player.h"

namespace RE
{
	class EffectItem;
	class MagicItem;
	class PlayerCharacter;
	class TESBoundObject;
	class BGSInventoryItem;
	struct InventoryUserUIInterfaceEntry;

	class HUDModeType;

	namespace InventoryUserUIUtils
	{
		inline void GetTotalDamageTypesInfo(Scaleform::GFx::Value& a_value)
		{
			using func_t = decltype(&GetTotalDamageTypesInfo);
			REL::Relocation<func_t> func{ REL::ID(688537, 2222627) };
			return func(a_value);
		}

		inline void GetTotalResistTypesInfo(Scaleform::GFx::Value& a_value, Scaleform::GFx::Value& a_resistTypes)
		{
			using func_t = decltype(&GetTotalResistTypesInfo);
			REL::Relocation<func_t> func{ REL::ID(1099574, 2222628) };
			return func(a_value, a_resistTypes);
		}

		inline void GetItemHPGain(const InventoryUserUIInterfaceEntry& a_entry, Scaleform::GFx::Value& a_value)
		{
			using func_t = decltype(&GetItemHPGain);
			REL::Relocation<func_t> func{ REL::ID(1483577, 2222629) };
			return func(a_entry, a_value);
		}

		namespace detail
		{
			inline void AddItemCardInfoEntry(
				Scaleform::GFx::Value& a_array,
				Scaleform::GFx::Value& a_newEntry,
				const BSFixedStringCS& a_textID,
				Scaleform::GFx::Value& a_value,
				float a_difference = 0.0F,
				float a_totalDamage = FLT_MAX,
				float a_compareDamage = FLT_MAX)
			{
				using func_t = decltype(&detail::AddItemCardInfoEntry);
				REL::Relocation<func_t> func{ REL::ID(489521, 2222648) };
				return func(a_array, a_newEntry, a_textID, a_value, a_difference, a_totalDamage, a_compareDamage);
			}
		}

		inline void AddItemCardInfoEntry(Scaleform::GFx::Value& a_array, Scaleform::GFx::Value& a_entry, const char* a_name, Scaleform::GFx::Value a_value, float a_difference, float a_totalValue = FLT_MAX, float a_comparisonValue = FLT_MAX)
		{
			detail::AddItemCardInfoEntry(a_array, a_entry, a_name, a_value, a_difference, a_totalValue, a_comparisonValue);
		}

		inline void AddItemCardInfoEntry(Scaleform::GFx::Value& a_array, Scaleform::GFx::Value& a_entry, const char* a_name = "", Scaleform::GFx::Value a_value = 0)
		{
			detail::AddItemCardInfoEntry(a_array, a_entry, a_name, a_value);
		}

		inline void AddItemCardInfoEntry(Scaleform::GFx::Value& a_array, const char* a_name, Scaleform::GFx::Value a_value)
		{
			RE::Scaleform::GFx::Value entry;
			detail::AddItemCardInfoEntry(a_array, entry, a_name, a_value);
		}
	}

	namespace StatsMenuUtils
	{
		inline void GetEffectDisplayInfo(MagicItem* a_item, EffectItem* a_effect, float& a_magnitude, float& a_duration)
		{
			using func_t = decltype(&StatsMenuUtils::GetEffectDisplayInfo);
			REL::Relocation<func_t> func{ REL::ID(294691, 2224586) };
			return func(a_item, a_effect, a_magnitude, a_duration);
		}
	}

	namespace UIUtils
	{
		using ComparisonItems = BSScrapArray<BSTTuple<const BGSInventoryItem*, std::uint32_t>>;

		inline void GetComparisonItems(const TESBoundObject* a_object, ComparisonItems& a_comparisonItems)
		{
			struct Context
			{
				const TESBoundObject** object;
				ComparisonItems* comparisonItems;
			};

			a_comparisonItems.clear();
			if (!a_object) {
				return;
			}

			REL::Relocation<PlayerCharacter**> player{ REL::ID(303410, 2698073) };
			if (!*player) {
				return;
			}

			const TESBoundObject* object = a_object;
			Context context{ std::addressof(object), std::addressof(a_comparisonItems) };
			using func_t = void(PlayerCharacter*, Context*, bool);
			REL::Relocation<func_t> func{ REL::ID(593818, 2222651) };
			func(*player, std::addressof(context), false);
		}

		inline void PlayMenuSound(const char* a_soundName)
		{
			using func_t = decltype(&PlayMenuSound);
			REL::Relocation<func_t> func{ REL::ID(1227993, 2249707) };
			return func(a_soundName);
		}
	}
}
