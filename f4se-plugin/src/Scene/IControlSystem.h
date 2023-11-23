#pragma once

namespace Scene
{
	class IScene;

	class IControllable
	{
	public:
		virtual void ApplyMorphSet(const std::string& id) = 0;
		virtual void TransitionToAnimation(std::shared_ptr<const Data::Animation> anim) = 0;
		virtual bool PushQueuedControlSystem() = 0;
		virtual uint64_t QUID() = 0;
		virtual bool QAutoAdvance() = 0;
		virtual bool HasPlayer() = 0;
		virtual void StartTimer(uint16_t id, double durationMs) = 0;
		virtual void StopTimer(uint16_t id) = 0;
		virtual void SoftEnd() = 0;
		virtual double QDuration() { return -1.0; };
		virtual void ReportSystemCompletion() {};

		template <class Archive>
		void serialize(Archive&)
		{
		}
	};

	class IControlSystem
	{
	public:
		virtual void SetInfo(Data::Position::ControlSystemInfo*) {}
		virtual void OnAnimationLoop(IControllable*) {}
		virtual void OnBegin(IControllable*, std::string_view) {}
		virtual void OnEnd(IControllable*, std::string_view) {}
		virtual std::string QAnimationID() { return ""; }
		virtual std::string QSystemID() { return ""; }
		virtual void Notify(const std::any&) {}
		virtual void OnTimer(uint16_t) {}
		virtual std::string_view QTypeName() { return "None"; }

		virtual ~IControlSystem(){};

		template <class Archive>
		void serialize(Archive&)
		{
		}
	};
}
