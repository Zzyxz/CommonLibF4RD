#pragma once

#include "RE/Scaleform/Kernel/SF_Allocator.h"

namespace RE::Scaleform
{
	struct ArrayDefaultPolicy;

	struct ArrayDefaultPolicy
	{
	public:
		~ArrayDefaultPolicy() noexcept {}  // NOLINT(modernize-use-equals-default)

		// members
		std::size_t capacity;  // 0
	};
	static_assert(sizeof(ArrayDefaultPolicy) == 0x8);

	template <std::int32_t = 0, std::int32_t = 4, bool = false>
	struct ArrayConstPolicy
	{
	public:
		~ArrayConstPolicy() noexcept {}  // NOLINT(modernize-use-equals-default)

		// members
		std::size_t capacity;  // 0
	};
	static_assert(sizeof(ArrayConstPolicy<>) == 0x8);

	template <class T, class Allocator, class SizePolicy>
	struct ArrayDataBase
	{
	public:
		// members
		T* data;            // 00
		std::size_t size;   // 08
		SizePolicy policy;  // 10
	};

	template <class T, class Allocator, class SizePolicy>
	struct ArrayData :
		public ArrayDataBase<T, Allocator, SizePolicy>  // 0
	{
	public:
	};

	template <class ArrayData>
	class ArrayBase
	{
	public:
		// membrs
		ArrayData data;  // 0
	};

	template <class T, std::int32_t SID = 2, class SizePolicy = ArrayDefaultPolicy>
	class Array :
		public ArrayBase<ArrayData<T, AllocatorGH<T, SID>, SizePolicy>>  // 0
	{
	public:
	};

	template <class T, std::int32_t STAT1 = 0, std::int32_t STAT2 = 0, class Alloc = AllocatorPagedCC<T, 0>>
	class ArrayPagedBase :
		public Alloc
	{
	public:
		// members
		std::uint64_t size;      // 08
		std::uint64_t numPages;  // 10
		std::uint64_t maxPages;  // 18
		T** pages;               // 20
	};
	static_assert(sizeof(ArrayPagedBase<void*>) == 0x28);

	template <class T, std::int32_t STAT1 = 0, std::int32_t STAT2 = 0, std::int32_t STAT3 = 0>
	class ArrayPagedCC :
		public ArrayPagedBase<T, STAT1, STAT2, AllocatorPagedCC<T, STAT3>>
	{
	public:
		T defaultValue;  // 28
	};
	static_assert(sizeof(ArrayPagedCC<void*>) == 0x30);
}
