#pragma once

#include "RE/Bethesda/BSTEvent.h"

namespace RE
{
	class BGSConstructibleObject;
	class TESFurniture;

	namespace ItemCrafted
	{
		struct Event
		{
		public:
			// members
			const TESFurniture* workbench;
			const BGSConstructibleObject* recipe;
		};

		inline void NotifyOfItemCrafted(const TESFurniture* a_workbench, const BGSConstructibleObject* a_recipe)
		{
			using func_t = decltype(&NotifyOfItemCrafted);
			REL::Relocation<func_t> func{ REL::ID(788895, 2232809) };
			return func(a_workbench, a_recipe);
		}

		inline void RegisterSink(BSTEventSink<Event>* a_sink)
		{
			using func_t = decltype(&RegisterSink);
			REL::Relocation<func_t> func{ REL::ID(1320496, 2232807) };
			return func(a_sink);
		}

		inline void UnregisterSink(BSTEventSink<Event>* a_sink)
		{
			using func_t = decltype(&UnregisterSink);
			REL::Relocation<func_t> func{ REL::ID(1054567, 2232808) };
			return func(a_sink);
		}
	}
}
