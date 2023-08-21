#pragma once

namespace Data
{
	class Uid
	{
	public:

		struct PersistentState
		{
			uint64_t nextUid = 1;
			uint32_t nextUiUid = 1;

			template <class Archive>
			void serialize(Archive& ar, const uint32_t)
			{
				ar(nextUid, nextUiUid);
			}
		};

		inline static std::unique_ptr<PersistentState> state = std::make_unique<PersistentState>();
		inline static std::mutex lock;

		static uint64_t Get()
		{
			std::unique_lock l{ lock };
			if (state->nextUid == UINT64_MAX) {
				state->nextUid = 1;
			}
			return state->nextUid++;
		}

		static uint32_t GetUI() {
			std::unique_lock l{ lock };
			if (state->nextUiUid == UINT32_MAX) {
				state->nextUiUid = 1;
			}
			return state->nextUiUid++;
		}

		static void Reset() {
			state->nextUid = 1;
			state->nextUiUid = 1;
		}
	};
}
