#pragma once

namespace RE
{
	class MovementControllerNPC
	{
	public:
		void SetMovementSpeed(float a_speed)
		{
			using func_t = decltype(&MovementControllerNPC::SetMovementSpeed);
			REL::Relocation<func_t> func{ REL::ID(356321) };
			return func(this, a_speed);
		}
	};
}
