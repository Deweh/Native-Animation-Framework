#pragma once

namespace PackageOverride
{
	using namespace Serialization::General;
	using PackageEndFunctor = Tasks::DataTaskFunctor<SerializableActorHandle>;

	struct PackageInfo
	{
		SerializableForm<RE::TESPackage> package = nullptr;
		bool reserved = false;
		std::optional<std::unique_ptr<PackageEndFunctor>> doneCallback = std::nullopt;

		template <class Archive>
		void serialize(Archive& ar, const uint32_t)
		{
			ar(package, reserved, doneCallback);
		}
	};

	struct PersistentState
	{
		std::unordered_map<SerializableActorHandle, PackageInfo> packages;

		template <class Archive>
		void save(Archive& ar, const uint32_t) const
		{
			ar(packages);
		}

		template <class Archive>
		void load(Archive& ar, const uint32_t)
		{
			ar(packages);
			for (auto iter = packages.begin(); iter != packages.end();) {
				if (iter->second.package != nullptr && SetPackage(iter->first, iter->second.package)) {
					iter++;
				} else {
					iter = packages.erase(iter);
				}
			}
		}

		static bool SetPackage(RE::ActorHandle hndl, RE::TESPackage* pkg) {
			auto a = hndl.get();

			if (pkg == nullptr || a == nullptr || a->currentProcess == nullptr) {
				return false;
			}

			a->currentProcess->ClearCurrentPackage(a.get());
			a->currentProcess->ClearCurrentDataForProcess(a.get());
			a->PutCreatedPackage(pkg, false, false, true);
			a->EvaluatePackage(false, false);
			return true;
		}
	};

	std::unique_ptr<PersistentState> state = std::make_unique<PersistentState>();
	std::shared_mutex lock;

	class PkgEventListener :
		public Singleton<PkgEventListener>,
		public RE::BSTEventSink<RE::TESPackageEvent>,
		public Data::EventListener<PkgEventListener>
	{
		virtual RE::BSEventNotifyControl ProcessEvent(const RE::TESPackageEvent& a_event, RE::BSTEventSource<RE::TESPackageEvent>*) override {
			if (a_event.type == RE::TESPackageEvent::End && a_event.refr != nullptr) {
				std::unique_ptr<PackageEndFunctor> queuedCallback = nullptr;
				{
					std::shared_lock l{ lock };

					for (auto& info : state->packages) {
						if (auto a = info.first.get().get(); a && a->formID == a_event.refr->formID
							&& info.second.package.get() != nullptr && info.second.package.get()->formID == a_event.formId
							&& info.second.doneCallback.has_value()) {
							queuedCallback = std::move(info.second.doneCallback.value());
							queuedCallback->data = info.first;
							info.second.doneCallback = std::nullopt;
						}
					}
				}
				if (queuedCallback != nullptr) {
					queuedCallback->Run();
				}
			}
			return RE::BSEventNotifyControl::kContinue;
		}

		void OnGameDataReady(Data::Events::event_type, Data::Events::EventData&) {
			RE::TESObjectREFR_Events::RegisterForPackage(this);
		}
	protected:
		friend class Singleton;
		PkgEventListener() {
			RegisterListener(Data::Events::GAME_DATA_READY, &PkgEventListener::OnGameDataReady);
		}
	};

	bool IsEvalAllowed(RE::Actor* a_actor)
	{
		if (!g_gameDataReady || !a_actor) {
			return true;
		}

		std::shared_lock l{ lock };
		return !state->packages.contains(a_actor->GetActorHandle());
	}

	bool Set(RE::ActorHandle a_hndl, RE::TESPackage* a_pkg, bool reserved = false, std::optional<std::unique_ptr<PackageEndFunctor>> doneCallback = std::nullopt)
	{
		std::unique_lock l{ lock };

		if (auto itm = state->packages.find(a_hndl); !reserved && itm != state->packages.end() && itm->second.reserved) {
			return false;
		}

		if (!PersistentState::SetPackage(a_hndl, a_pkg)) {
			return false;
		}

		state->packages[a_hndl] = { a_pkg, reserved, std::move(doneCallback) };
		return true;
	}

	void Clear(RE::ActorHandle a_hndl, bool reserved = false)
	{
		std::unique_lock l{ lock };
		auto itm = state->packages.find(a_hndl);
		if (itm != state->packages.end() && (reserved || !itm->second.reserved)) {
			state->packages.erase(itm);
		}
	}

	void Reset() {
		std::unique_lock l{ lock };
		state->packages.clear();
	}
}

CEREAL_REGISTER_TYPE(PackageOverride::PackageEndFunctor);
