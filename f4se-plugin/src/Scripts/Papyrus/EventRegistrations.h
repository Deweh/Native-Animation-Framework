#pragma once

namespace Papyrus
{
	using namespace RE::BSScript;

	//UNUSED - placeholder for possibly more convienent Papyrus event registrations.
	class EventRegistrations
	{
		
		struct Registration
		{
			uint64_t handle;
			RE::BSFixedString scriptName;
		};

		struct PersistentState
		{
			std::unordered_map<std::string, std::vector<Registration>> registrations;
		};

		inline static std::unique_ptr<PersistentState> state = std::make_unique<PersistentState>();

		inline static std::optional<Registration> GetRegistrationInfoFromStack(IVirtualMachine& a_VM, uint32_t a_stackID) {
			auto iVM = static_cast<Internal::VirtualMachine*>(&a_VM);
			RE::BSTSmartPointer<Stack> stck;
			if (iVM->GetStackByID(a_stackID, stck) && stck && stck->top && stck->top->previousFrame) {
				if (const auto obj = get<Object>(stck->top->previousFrame->self); obj != nullptr && obj->type != nullptr) {
					Registration result{ obj->GetHandle(), obj->type->name };
					return result;
				}
			}

			return std::nullopt;
		}

		inline static void Register(const Registration& reg, const std::string& evntName) {
			state->registrations[evntName].push_back(reg);
		}

		inline static void Unregister(const Registration& reg, const std::string& evntName) {
			auto iter = state->registrations.find(evntName);
			if (iter != state->registrations.end()) {
				for (auto i = iter->second.begin(); i != iter->second.end();) {
					if (i->handle == reg.handle && i->scriptName == reg.scriptName) {
						i = iter->second.erase(i);
						break;
					} else {
						i++;
					}
				}
			}
		}

		template <class... Args>
		inline static void Dispatch(const std::string& evntName, Args... _args)
		{
			auto iter = state->registrations.find(evntName);
			if (iter != state->registrations.end()) {
				auto& regs = iter->second;
				auto vm = RE::GameVM::GetSingleton()->GetVM();
				auto scrapFunc = (Papyrus::FunctionArgs{ vm.get(), _args... }).get();

				for (auto& r : regs) {
					vm->DispatchMethodCall(r.handle, r.scriptName, evntName, scrapFunc, nullptr);
				}
			}
		}
	};
}
