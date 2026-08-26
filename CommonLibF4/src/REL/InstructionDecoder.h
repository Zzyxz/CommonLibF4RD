#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

#ifdef _MSC_VER
#	pragma warning(push, 0)
#endif
#include <Zydis/Zydis.h>
#ifdef _MSC_VER
#	pragma warning(pop)
#endif

namespace REL::detail
{
	template <class Callback>
	[[nodiscard]] bool for_each_direct_relative_branch(
		std::span<const std::uint8_t> a_code,
		Callback&& a_callback) noexcept
	{
		ZydisDecoder decoder{};
		if (ZYAN_FAILED(ZydisDecoderInit(
				std::addressof(decoder),
				ZYDIS_MACHINE_MODE_LONG_64,
				ZYDIS_STACK_WIDTH_64))) {
			return false;
		}

		std::size_t offset{};
		while (offset < a_code.size()) {
			ZydisDecodedInstruction instruction{};
			if (ZYAN_FAILED(ZydisDecoderDecodeInstruction(
					std::addressof(decoder),
					nullptr,
					a_code.data() + offset,
					a_code.size() - offset,
					std::addressof(instruction))) ||
				instruction.length == 0 || instruction.length > a_code.size() - offset) {
				return false;
			}

			const auto opcode = a_code[offset];
			if (instruction.length == 5 && (opcode == 0xE8 || opcode == 0xE9)) {
				std::invoke(a_callback, offset, opcode);
			}
			offset += instruction.length;
		}
		return true;
	}
}