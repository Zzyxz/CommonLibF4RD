#pragma once

namespace RE
{
	class NiFrustumPlanes
	{
	public:
		std::byte data[0x70];
	};
	static_assert(sizeof(NiFrustumPlanes) == 0x70);
}
