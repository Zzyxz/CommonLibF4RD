#pragma once

#include "RE/NetImmerse/NiFlags.h"
#include "RE/NetImmerse/NiProperty.h"

namespace RE
{
	class __declspec(novtable) NiAlphaProperty :
		public NiProperty  // 00
	{
	public:
		static constexpr auto RTTI{ RTTI::NiAlphaProperty };
		static constexpr auto VTABLE{ VTABLE::NiAlphaProperty };
		static constexpr auto Ni_RTTI{ Ni_RTTI::NiAlphaProperty };

		enum class AlphaFunction
		{
			kOne,
			kZero,
			kSrcColor,
			kInvSrcColor,
			kDestColor,
			kInvDestColor,
			kSrcAlpha,
			kInvSrcAlpha,
			kDestAlpha,
			kInvDestAlpha,
			kSrcAlphaTest
		};

		enum class TestFunction
		{
			kAlways,
			kLess,
			kEqual,
			kLessEqual,
			kGreater,
			kNotEqual,
			kGreaterEqual,
			kNever
		};

		NiAlphaProperty()
		{
			stl::emplace_vtable(this);
			NiTFlags<std::uint16_t, NiProperty> f;
			f.flags = 0xEC;
			flags = f;
			alphaTestRef = 0;
		}
		virtual ~NiAlphaProperty();

		virtual std::int32_t Type()
		{
			return 0;
		}

		void SetDestBlendMode(AlphaFunction f)
		{
			flags.flags = static_cast<std::uint16_t>(
				(flags.flags & 0xFE1F) |
				(static_cast<std::uint16_t>(stl::to_underlying(f)) << 5));
		}

		void SetSrcBlendMode(AlphaFunction f)
		{
			flags.flags = static_cast<std::uint16_t>(
				(flags.flags & 0xFFE1) |
				(static_cast<std::uint16_t>(stl::to_underlying(f)) << 1));
		}

		void SetTestMode(TestFunction f)
		{
			flags.flags = static_cast<std::uint16_t>(
				(flags.flags & 0xE3FF) |
				(static_cast<std::uint16_t>(stl::to_underlying(f)) << 10));
		}

		void SetAlphaBlending(bool b)
		{
			if (b) {
				flags.flags |= 0x1;
			} else {
				flags.flags &= 0xFFFE;
			}
		}

		void SetAlphaTesting(bool b)
		{
			if (b) {
				flags.flags |= 0x200;
			} else {
				flags.flags &= 0xFDFF;
			}
		}

		// members
		NiTFlags<std::uint16_t, NiProperty> flags;  // 28
		std::int8_t alphaTestRef;                   // 2A

		F4_HEAP_REDEFINE_ALIGNED_NEW(NiAlphaProperty);
	};
	static_assert(sizeof(NiAlphaProperty) == 0x30);
}
