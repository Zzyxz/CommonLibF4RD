#pragma once

#include "RE/Scaleform/Kernel/SF_Memory.h"

namespace RE::Scaleform
{
	template <std::int32_t>
	class AllocatorBaseGH
	{
	public:
	};
	//static_assert(std::is_empty_v<AllocatorBaseGH<0>>);

	template <std::int32_t>
	class AllocatorBaseLH
	{
	public:
	};
	//static_assert(std::is_empty_v<AllocatorBaseLH<0>>);

	template <class T>
	class ConstructorMov
	{
	public:
	};
	//static_assert(std::is_empty_v<ConstructorMov<void*>>);

	template <class T>
	class ConstructorPagedMovCC :
		public ConstructorMov<T>
	{
	public:
	};
	//static_assert(std::is_empty_v<ConstructorPagedMovCC<void*>>);

	template <class T, std::int32_t STAT>
	class AllocatorPagedCC :
		public AllocatorBaseLH<STAT>,
		public ConstructorPagedMovCC<T>
	{
	private:
		// members
		std::byte gap;  // 00
	};
	// static_assert(sizeof(AllocatorPagedCC<void*, 0>) == 0x01);

	template <class T, std::int32_t SID = 2>
	struct AllocatorGH :
		public AllocatorBaseGH<SID>,  // 0
		public ConstructorMov<T>      // 1
	{
	public:
	};
	//static_assert(std::is_empty_v<AllocatorGH<void*>>);
}
