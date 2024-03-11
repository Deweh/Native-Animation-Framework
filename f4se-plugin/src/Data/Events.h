#pragma once
#include <any>

namespace Data
{
	class Events
	{
	public:
		typedef uint16_t event_type;

		enum : event_type
		{
			SCENE_START,
			SCENE_FAILED,
			SCENE_END,
			SCENE_POS_CHANGE,
			TREE_POS_CHANGE,
			HUD_INIT,
			HUD_UP_KEY_DOWN,
			HUD_DOWN_KEY_DOWN,
			HUD_LEFT_KEY_DOWN,
			HUD_RIGHT_KEY_DOWN,
			SHUD_TREE_ITEM_CHANGED,
			GAME_DATA_READY,
			SCENE_SPEED_CHANGE,
			SCENE_SYNC_STATUS_CHANGE,
			SCENE_ANIM_LOOP,
			SCENE_ANIM_CHANGE,
			HUD_UP_KEY_UP,
			HUD_DOWN_KEY_UP,
			HUD_LEFT_KEY_UP,
			HUD_RIGHT_KEY_UP,
			HUD_Q_KEY_DOWN,
			HUD_Q_KEY_UP,
			HUD_E_KEY_DOWN,
			HUD_E_KEY_UP,
			SETTINGS_CHANGED
		};

		struct SceneData
		{
			uint64_t id;
			std::vector<RE::NiPointer<RE::Actor>> actors;
		};

		struct ScenePositionData
		{
			uint64_t id;
			std::string newPosition;
			bool successful;
		};

		struct TreePositionData : public ScenePositionData
		{
			std::string treeId;
		};

		typedef std::any EventData;
		typedef std::function<void(event_type, EventData&)> EventFunctor;
		typedef std::list<EventFunctor>::iterator EventRegistration;

		static EventRegistration Subscribe(event_type evnt, EventFunctor receiver) {
			std::unique_lock l{ lock };
			registrations[evnt].push_back(receiver);
			return --registrations[evnt].end();
		}

		static void Unsubscribe(event_type evnt, EventRegistration reg)
		{
			std::unique_lock l{ lock };
			if (registrations.contains(evnt)) {
				for (auto iter = registrations[evnt].begin(); iter != registrations[evnt].end();) {
					if (std::addressof(*iter) == std::addressof(*reg)) {
						iter = registrations[evnt].erase(iter);
						break;
					} else {
						iter++;
					}
				}
			}
		}

		static void SendMutable(event_type evnt, EventData& data)
		{
			std::unique_lock l{ lock };
			if (registrations.contains(evnt)) {
				for (auto& f : registrations[evnt]) {
					f(evnt, data);
				}
			}
		}

		static void Send(event_type evnt, const EventData& data = false) {
			EventData e = data;
			SendMutable(evnt, e);
		}
	private:
		inline static std::unordered_map<event_type, std::list<EventFunctor>> registrations;
		inline static safe_mutex lock;
	};

	template <typename T>
	class EventListener
	{
	public:
		struct RegContainer
		{
			Events::EventRegistration reg;
			Events::event_type evnt;

			~RegContainer() {
				Events::Unsubscribe(evnt, reg);
			}
		};

		template <typename F>
		void RegisterListener(Events::event_type evnt, F receiver)
		{
			std::unique_lock l{ eventRegistrationlock };
			auto container = std::make_unique<RegContainer>();
			container->evnt = evnt;
			container->reg = Events::Subscribe(evnt, Events::EventFunctor(std::bind(receiver, static_cast<T*>(this), std::placeholders::_1, std::placeholders::_2)));
			eventRegistrations[evnt].push_back(std::move(container));
		}

		void UnregisterListener(Events::event_type evnt) {
			std::unique_lock l{ eventRegistrationlock };
			if (eventRegistrations.contains(evnt)) {
				eventRegistrations.erase(evnt);
			}
		}
	protected:
		std::unordered_map<Events::event_type, std::vector<std::unique_ptr<RegContainer>>> eventRegistrations;
		safe_mutex eventRegistrationlock;
	};
}
