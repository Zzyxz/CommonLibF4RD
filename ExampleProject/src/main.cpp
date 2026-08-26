#include "Plugin.h"

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se)
{
	if (!Plugin::Initialize(a_f4se)) {
		return false;
	}

	return true;
}
