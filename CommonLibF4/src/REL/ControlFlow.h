#pragma once

#include "REL/Reachability.h"
#include "InstructionDecoder.h"

#include <algorithm>
#include <limits>
#include <optional>
#include <vector>

namespace REL::detail
{
	struct CodeRange
	{
		std::uint32_t rva;
		std::span<const std::uint8_t> bytes;
	};

	// Follow paths from one entry, not bytes after a terminal jump/return. Calls
	// are assumed capable of returning; callees and exception edges are not walked.
	// Unknown indirect edges never prove that an unvisited site is unreachable.
	// The reader should only return validated, readable in-image pointer slots.
	template <class PointerReader>
	[[nodiscard]] Reachability instruction_reachability(
		std::span<const CodeRange> a_ranges,
		std::uint32_t a_entry,
		std::uint32_t a_site,
		std::uint64_t a_imageBase,
		PointerReader&& a_readPointer)
	{
		constexpr std::size_t maxBytes = 256u * 1024u;
		if (a_ranges.empty() || a_ranges.size() > 1024) return Reachability::kInvalid;
		std::size_t total{};
		std::uint64_t previousEnd{};
		for (const auto& range : a_ranges) {
			const auto end = static_cast<std::uint64_t>(range.rva) + range.bytes.size();
			if (range.bytes.empty() || range.bytes.size() > maxBytes - total ||
				range.rva < previousEnd || end > std::uint64_t{ UINT32_MAX } + 1) {
				return Reachability::kInvalid;
			}
			previousEnd = end;
			total += range.bytes.size();
		}
		const auto rangeAt = [&](std::uint32_t rva) -> std::size_t {
			for (std::size_t i = 0; i < a_ranges.size(); ++i) {
				if (rva >= a_ranges[i].rva &&
					static_cast<std::uint64_t>(rva - a_ranges[i].rva) < a_ranges[i].bytes.size()) return i;
			}
			return a_ranges.size();
		};
		if (rangeAt(a_entry) == a_ranges.size() || rangeAt(a_site) == a_ranges.size()) {
			return Reachability::kInvalid;
		}
		ZydisDecoder decoder{};
		if (ZYAN_FAILED(ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64))) {
			return Reachability::kInvalid;
		}
		std::vector<std::vector<std::uint8_t>> visited;
		visited.reserve(a_ranges.size());
		for (const auto& range : a_ranges) visited.emplace_back(range.bytes.size(), 0);
		std::vector<std::uint32_t> pending{ a_entry };
		bool unknownEdge{};
		while (!pending.empty()) {
			const auto pc = pending.back();
			pending.pop_back();
			const auto ri = rangeAt(pc);
			if (ri == a_ranges.size()) { unknownEdge = true; continue; }
			const auto offset = static_cast<std::size_t>(pc - a_ranges[ri].rva);
			auto& marks = visited[ri];
			if (marks[offset] == 1) continue;
			if (marks[offset] == 2) { unknownEdge = true; continue; }
			ZydisDecodedInstruction instruction{};
			ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT]{};
			const auto code = a_ranges[ri].bytes.subspan(offset);
			if (ZYAN_FAILED(ZydisDecoderDecodeFull(&decoder, code.data(), code.size(), &instruction, operands)) ||
				instruction.length == 0 || instruction.length > code.size()) {
				marks[offset] = 2;
				unknownEdge = true;
				continue;
			}
			bool overlap{};
			for (std::size_t i = 1; i < instruction.length; ++i) overlap = overlap || marks[offset + i] != 0;
			if (overlap) { marks[offset] = 2; unknownEdge = true; continue; }
			marks[offset] = 1;
			std::fill_n(marks.begin() + offset + 1, instruction.length - 1, std::uint8_t{ 2 });
			if (pc == a_site) return Reachability::kReachable;

			const auto next = static_cast<std::uint64_t>(pc) + instruction.length;
			const auto category = instruction.meta.category;
			const auto pushInternal = [&](std::int64_t rva) {
				if (rva >= 0 && rva <= UINT32_MAX && rangeAt(static_cast<std::uint32_t>(rva)) != a_ranges.size()) {
					pending.push_back(static_cast<std::uint32_t>(rva));
				}
			};
			if (category == ZYDIS_CATEGORY_RET) continue;
			if (category == ZYDIS_CATEGORY_UNCOND_BR || category == ZYDIS_CATEGORY_COND_BR) {
				if (instruction.operand_count_visible != 0 && operands[0].type == ZYDIS_OPERAND_TYPE_IMMEDIATE && operands[0].imm.is_relative) {
					// x86-64 direct branch displacements are at most signed 32-bit.
					pushInternal(static_cast<std::int64_t>(next) + operands[0].imm.value.s);
				} else if (category == ZYDIS_CATEGORY_UNCOND_BR && instruction.operand_count_visible != 0 &&
					operands[0].type == ZYDIS_OPERAND_TYPE_MEMORY && operands[0].size == 64 &&
					operands[0].mem.base == ZYDIS_REGISTER_RIP && operands[0].mem.index == ZYDIS_REGISTER_NONE &&
					!(instruction.attributes & (ZYDIS_ATTRIB_HAS_SEGMENT_FS | ZYDIS_ATTRIB_HAS_SEGMENT_GS))) {
					const auto slot = static_cast<std::int64_t>(next) + operands[0].mem.disp.value;
					const auto pointer = slot >= 0 && slot <= UINT32_MAX ?
						a_readPointer(static_cast<std::uint32_t>(slot)) : std::optional<std::uint64_t>{};
					if (!pointer) unknownEdge = true;
					else if (*pointer >= a_imageBase && *pointer - a_imageBase <= UINT32_MAX) {
						pushInternal(static_cast<std::int64_t>(*pointer - a_imageBase));
					}
				} else {
					unknownEdge = true;
				}
				if (category == ZYDIS_CATEGORY_UNCOND_BR) continue;
			} else if (category == ZYDIS_CATEGORY_INTERRUPT || instruction.mnemonic == ZYDIS_MNEMONIC_UD2 ||
				instruction.mnemonic == ZYDIS_MNEMONIC_HLT) {
				unknownEdge = true;
				continue;
			}
			if (next <= UINT32_MAX && rangeAt(static_cast<std::uint32_t>(next)) != a_ranges.size()) {
				pending.push_back(static_cast<std::uint32_t>(next));
			} else {
				unknownEdge = true;
			}
		}
		return unknownEdge ? Reachability::kIndeterminate : Reachability::kUnreachable;
	}
}
