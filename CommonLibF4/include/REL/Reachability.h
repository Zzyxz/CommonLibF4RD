#pragma once

#include <cstdint>

namespace REL
{
	// Result of a bounded control-flow check.
	enum class Reachability : std::uint8_t
	{
		kReachable,     // A decoded path exists within the owner's unwind scopes.
		kUnreachable,   // No such path; external detours are not followed.
		kIndeterminate, // Unknown indirect/exception/invalid-code edge encountered.
		kInvalid        // Invalid owner, site or scope metadata.
	};
}
