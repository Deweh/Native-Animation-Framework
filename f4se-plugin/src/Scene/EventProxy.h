#pragma once

namespace Scene
{
	class EventProxy :
		public Singleton<EventProxy>,
		public Data::EventListener<EventProxy>,
		public RE::BSTEventSink<RE::TESHitEvent>,
		public RE::BSTEventSink<RE::TESDeathEvent>,
		public RE::BSTEventSink<RE::TESActorLocationChangeEvent>
	{
	public:
		void VisitAllScenesWithRef(RE::TESObjectREFR* refr, std::function<void(IScene*, RE::Actor*)> func) {
			if (refr != nullptr && refr->HasKeyword(Data::Forms::NAFInSceneKW)) {
				if (auto a = refr->As<RE::Actor>(); a) {
					auto hndl = a->GetActorHandle();
					SceneManager::VisitAllScenes([&](IScene* scn) {
						if (scn && scn->actors.contains(hndl)) {
							func(scn, a);
						}
					});
				}
			}
		}

		virtual RE::BSEventNotifyControl ProcessEvent(const RE::TESHitEvent& a_event, RE::BSTEventSource<RE::TESHitEvent>*) override
		{
			VisitAllScenesWithRef(a_event.victim, [&](IScene* scn, RE::Actor* a) {
				if (scn)
					scn->OnActorHit(a, a_event);
			});
			return RE::BSEventNotifyControl::kContinue;
		}

		virtual RE::BSEventNotifyControl ProcessEvent(const RE::TESDeathEvent& a_event, RE::BSTEventSource<RE::TESDeathEvent>*) override
		{
			VisitAllScenesWithRef(a_event.actorDying.get(), [&](IScene* scn, RE::Actor* a) {
				if (scn)
					scn->OnActorDeath(a);
			});
			return RE::BSEventNotifyControl::kContinue;
		}

		virtual RE::BSEventNotifyControl ProcessEvent(const RE::TESActorLocationChangeEvent& a_event, RE::BSTEventSource<RE::TESActorLocationChangeEvent>*) override
		{
			VisitAllScenesWithRef(a_event.refr.get(), [&](IScene* scn, RE::Actor* a) {
				if (scn)
					scn->OnActorLocationChange(a, a_event.newLocation);
			});
			return RE::BSEventNotifyControl::kContinue;
		}

		void OnGameDataReady(Data::Events::event_type, Data::Events::EventData&) {
			RE::TESObjectREFR_Events::RegisterForHit(this);
			RE::TESObjectREFR_Events::RegisterForDeath(this);
			RE::TESObjectREFR_Events::RegisterForActorLocationChange(this);
		}
	protected:
		friend class Singleton;
		EventProxy() {
			RegisterListener(Data::Events::GAME_DATA_READY, &EventProxy::OnGameDataReady);
		}
	};
}
