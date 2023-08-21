#pragma once
#include "IScene.h"
#include "SceneManager.h"

namespace Scene
{
	template <typename T>
	class DelegateFunctor : public SceneFunctor
	{
	public:
		SCENE_FUNCTOR();

		virtual void Run() override
		{
			F4SE::GetTaskInterface()->AddTask(new T(sceneId));
		}
	};
}
