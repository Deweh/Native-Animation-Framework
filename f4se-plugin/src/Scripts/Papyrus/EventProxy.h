#pragma once
#include "Structs.h"

namespace Papyrus
{
	using namespace NAF;

	class EventProxy : public Singleton<EventProxy>, public Data::EventListener<EventProxy>
	{
	public:
		using Events = Data::Events;

		void OnSceneStart(Events::event_type, Events::EventData& data) {
			if (auto u64 = std::any_cast<uint64_t>(&data); u64) {
				GameUtil::SendPapyrusEvent(PEVENT_SCENE_START, PackSceneId(*u64));
			}
		}

		void OnSceneEnd(Events::event_type, Events::EventData& data)
		{
			if (auto d = std::any_cast<Events::SceneData>(&data); d) {
				GameUtil::SendPapyrusEvent(PEVENT_SCENE_END, PackSceneId(d->id));

				std::vector<RE::Actor*> actors;
				actors.reserve(d->actors.size());
				for (const auto& a : d->actors) {
					actors.push_back(a.get());
				}

				GameUtil::SendPapyrusEvent(PEVENT_SCENE_END_DATA, PackSceneId(d->id), actors);
			}
		}

		void OnScenePosChange(Events::event_type, Events::EventData& data)
		{
			if (auto sData = std::any_cast<Events::ScenePositionData>(&data); sData && sData->successful) {
				GameUtil::SendPapyrusEvent(PEVENT_SCENE_POS_CHANGE, PackSceneId(sData->id), sData->newPosition);
			}
		}

		void OnTreePosChange(Events::event_type, Events::EventData& data) {
			if (auto sData = std::any_cast<Events::TreePositionData>(&data); sData && sData->successful) {
				GameUtil::SendPapyrusEvent(PEVENT_TREE_POS_CHANGE, PackSceneId(sData->id), sData->newPosition, sData->treeId);
			}
		}

	protected:
		friend class Singleton;
		EventProxy() {
			RegisterListener(Events::SCENE_START, &EventProxy::OnSceneStart);
			RegisterListener(Events::SCENE_END, &EventProxy::OnSceneEnd);
			RegisterListener(Events::SCENE_POS_CHANGE, &EventProxy::OnScenePosChange);
			RegisterListener(Events::TREE_POS_CHANGE, &EventProxy::OnTreePosChange);
		}
	};
}
