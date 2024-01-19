#pragma once

namespace RE
{
	namespace BSModelDB
	{
		void* Demand(const char* modelPath, NiPointer<NiNode>& result, const int64_t& flags = 3) {
			using func_t = decltype(&Demand);
			REL::Relocation<func_t> func{ REL::ID(1225688) };
			return func(modelPath, result, flags);
		}
	}
}
