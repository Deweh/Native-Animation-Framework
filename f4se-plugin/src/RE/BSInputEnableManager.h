#pragma once

namespace RE
{
	class BSInputEnableManager
	{
	public:
		static BSInputEnableManager* GetSingleton() {
			REL::Relocation<BSInputEnableManager**> singleton{ REL::ID(781703) };
			return *singleton;
		}

		void ClearForcedState()
		{
			using func_t = decltype(&BSInputEnableManager::ClearForcedState);
			REL::Relocation<func_t> func{ REL::ID(920956) };
			return func(this);
		}

		void ForceUserEventEnabled(UserEvents::USER_EVENT_FLAG a_event, bool a_enabled) {
			using func_t = decltype(&BSInputEnableManager::ForceUserEventEnabled);
			REL::Relocation<func_t> func{ REL::ID(230159) };
			return func(this, a_event, a_enabled);
		}
	};
}
