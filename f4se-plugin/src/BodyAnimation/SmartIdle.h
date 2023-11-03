#pragma once
#include "Scene/DynamicIdle.h"

namespace BodyAnimation
{
	class SmartIdle
	{
	public:
		enum IType : uint8_t
		{
			kForm = 0,
			kHKX = 1,
			kNAF = 2
		};

		void SetIdleForm(RE::TESIdleForm* _form) {
			_type = kForm;
			if (_form == nullptr) {
				_data = "";
				_id = "";
			} else {
				_data = _form->GetFormEditorID();
				_id = _form->animFileName;
			}
		}

		void SetHKXPath(std::string _path) {
			_type = kHKX;
			_data = _path;
			_id = _path;
		}

		void SetNAFPath(std::string _path, std::string id) {
			_type = kNAF;
			_data = _path;
			_id = id;
		}

		bool Play(RE::Actor* targetActor) const
		{
			if (!targetActor)
				return false;

			std::optional<std::string> animEvent = std::nullopt;
			std::optional<std::string> graph = std::nullopt;
			RE::BGSAction* resetGraph = RE::BGSAnimationSystemUtils::GetDefaultObjectForActionInitializeToBaseState();
			RE::TESActionData action(RE::ActionInput::ACTIONPRIORITY::kTry, targetActor, resetGraph);

			switch (_type) {
			case kForm:
				BodyAnimation::GraphHook::StopAnimation(targetActor, 0.6f);
				if (targetActor->currentProcess) {
					auto idl = RE::TESForm::GetFormByEditorID<RE::TESIdleForm>(_data);
					if (idl == nullptr)
						idl = Data::Forms::LooseIdleStop;
					targetActor->currentProcess->PlayIdle(targetActor, 0x35, idl);
					return true;
				}
				return false;
			case kHKX:
				BodyAnimation::GraphHook::StopAnimation(targetActor, 0.6f);
				if (auto r = Data::GetRace(targetActor); r != nullptr) {
					animEvent = r->startEvent;
					graph = r->graph;
				}
				Scene::DynamicIdle::Play(targetActor, _data, animEvent, graph);
				return true;
			case kNAF:
				RE::BGSAnimationSystemUtils::RunActionOnActor(targetActor, action);
				return BodyAnimation::GraphHook::LoadAndPlayAnimation(targetActor, USERDATA_DIR + _data, 0.6f, _id);
			default:
				return false;
			}
		}

		static void Stop(RE::Actor* targetActor, const std::optional<std::string>& stopEvent = std::nullopt) {
			if (!targetActor)
				return;

			BodyAnimation::GraphHook::StopAnimation(targetActor, 0.6f);
			RE::BGSAction* resetGraph = RE::BGSAnimationSystemUtils::GetDefaultObjectForActionInitializeToBaseState();
			RE::TESActionData action(RE::ActionInput::ACTIONPRIORITY::kTry, targetActor, resetGraph);
			RE::BGSAnimationSystemUtils::RunActionOnActor(targetActor, action);
			if (stopEvent.has_value())
				targetActor->NotifyAnimationGraphImpl(stopEvent.value());
		}

		static bool GetGraphTime(RE::Actor* a, GameUtil::GraphTime& result)
		{
			if (!a)
				return false;

			bool managed = BodyAnimation::GraphHook::VisitGraph(
				a, [&](NodeAnimationGraph* g) {
					result.current = g->generator.localTime;
					result.total = g->generator.animData->duration;
				},
				false, true);

			if (!managed) {
				return GameUtil::GetAnimationGraphTime(a, result);
			}

			return true;
		}

		static bool SetGraphTime(RE::Actor* a, float t)
		{
			if (!a)
				return false;

			bool managed = BodyAnimation::GraphHook::VisitGraph(
				a, [&](NodeAnimationGraph* g) {
					g->generator.localTime = t;
				},
				false, true);

			if (!managed) {
				return GameUtil::SetAnimationGraphTime(a, t);
			}
			
			return true;
		}

		static bool IsInTransition(RE::Actor* a) {
			bool managed = false;
			BodyAnimation::GraphHook::VisitGraph(
				a, [&](NodeAnimationGraph* g) {
					managed = g->state == NodeAnimationGraph::kTransition;
				},
				false);
			
			if (managed)
				return true;

			return RE::BGSAnimationSystemUtils::IsActiveGraphInTransition(a);
		}

		bool IsIdleLoading(RE::Actor* a) const
		{
			switch (_type) {
			case kForm:
			case kHKX:
				return RE::BGSAnimationSystemUtils::IsIdleLoading(a, _id);
			case kNAF:
				return BodyAnimation::GraphHook::IsAnimationLoading(a, _data, _id);
			default:
				return false;
			}
		}
		
		std::string GetFilePath() const
		{
			switch (_type) {
			case kForm:
			case kHKX:
				return _id;
			case kNAF:
				return _data;
			default:
				return "";
			}
		}

		template <class Archive>
		void serialize(Archive& ar, const uint32_t)
		{
			ar(_data, _id, _type);
		}

	private:
		std::string _data = "";
		std::string _id = "";
		IType _type = kForm;
	};
}
