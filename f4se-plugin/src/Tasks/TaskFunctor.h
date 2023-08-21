#pragma once

namespace Tasks
{
	class TaskFunctor
	{
	public:
		TaskFunctor(){}
		virtual void Run() { logger::warn("Default TaskFunctor called."); }
		virtual void Finalize(){}
		template <class Archive>
		void serialize(Archive&) const{}
	};

	template <typename T>
	class DataTaskFunctor : public TaskFunctor
	{
	public:
		T data;
		template <class Archive>
		void serialize(Archive& ar, const uint32_t) const
		{
			ar(cereal::base_class<TaskFunctor>(this));
		}
	};
}
