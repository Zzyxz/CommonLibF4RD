#pragma once

#include <RE/Bethesda/BSTHashMap.h>

namespace RE
{
	class NiCloningProcess
	{
	public:
		BSTHashMap<void*, void*> unk00;  // 00
		void* unk30;                     // 30
		std::uint64_t unk38;             // 38
		std::uint64_t unk40;             // 40
		void* unk48;                     // 48 - DEADBEEF
		void* unk50;                     // 50 - bucket
		std::uint64_t unk58;             // 58
		std::uint32_t unk60;             // 60 - copytype? 0, 1, 2
		std::uint32_t unk64;             // 64
	};
}
