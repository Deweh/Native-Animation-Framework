#pragma once
#include "FaceAnimation/AnimationData.h"

namespace FaceAnimation
{
	struct FaceAnimation
	{
		std::mutex lock;
		AnimationData data;
		double timeElapsed = 0.00001;
		bool loop = false;
		bool havokSync = false;
		bool paused = false;

		FaceAnimation(AnimationData _data) {
			data = _data;
		}

		FaceAnimation()
		{
		}

		bool LoadData(const std::string& id)
		{
			auto targetAnim = Data::GetFaceAnim(id);

			if (targetAnim == nullptr) {
				logger::warn("Cannot load face animation '{}', no such animation id exists.", id);
				return false;
			}

			std::istringstream buffer(Data::AnimCache::GetFile(targetAnim->fileName), std::ios::binary);

			try {
				cereal::BinaryInputArchive inArchive(buffer);
				inArchive(data);
			} catch (std::exception ex) {
				logger::warn("Failed to load AnimationData. Full message: {}", ex.what());
				return false;
			}

			return true;
		}

		void SetDuration(double durationMs) {
			data.duration = durationMs / 1000;
		}

		void SetStartNow() {
			timeElapsed = 0.00001;
		}

		bool Update(RE::BSFaceGenAnimationData* animData, RE::BSGeometry* eyeGeo, float timeDelta) {
			std::scoped_lock l{ lock };

			if (!paused)
				timeElapsed += timeDelta;

			if (timeElapsed > data.duration) {
				if (loop) {
					SetStartNow();
				} else {
					return false;
				}
			}

			UpdateNoDelta(animData, eyeGeo);
			return true;
		}

		void UpdateNoDelta(RE::BSFaceGenAnimationData* animData, RE::BSGeometry* eyeGeo)
		{
			double timeDeltaNormalized = timeElapsed / data.duration;
			for (auto& tl : data.timelines) {
				if (!tl.isEyes) {
					if (tl.morph < 54) {
						animData->finalExp.exp[tl.morph] = std::clamp(tl.GetValueAtTime(timeDeltaNormalized), 0.001f, 0.999f);
					} else if (tl.morph == 100) {
						GameUtil::SetEmissiveMult(eyeGeo, tl.GetValueAtTime(timeDeltaNormalized));
					}
				} else {
					auto val = tl.GetEyesValueAtTime(timeDeltaNormalized);
					GameUtil::SetEyeCoords(eyeGeo, static_cast<float>(val.u), static_cast<float>(val.v));
				}
			}
		}
	};
}
