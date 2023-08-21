#pragma once
#include "Misc/Easing.h"

namespace FaceAnimation
{
	double ConvertEyeCoordRange(double coord, bool isX, bool userToInternal) {
		if (userToInternal) {
			if (isX) {
				return coord * -0.25;
			} else {
				return coord * 0.2;
			}
		} else {
			if (isX) {
				return coord * -4.0;
			} else {
				return coord * 5.0;
			}
		}
	}

	double lerp(double a, double b, double t)
	{
		return a + t * (b - a);
	}

	struct EyeVector
	{
		double u;
		double v;

		void ConvertRange(bool userToInternal) {
			u = ConvertEyeCoordRange(u, true, userToInternal);
			v = ConvertEyeCoordRange(v, false, userToInternal);
		}

		template <class Archive>
		void serialize(Archive& ar, const uint32_t)
		{
			ar(u, v);
		}
	};

	struct Keyframe
	{
		float value = 0;
		Easing::Function ease = Easing::Function::InOutCubic;
		EyeVector eyesValue;

		template <class Archive>
		void serialize(Archive& ar, const uint32_t)
		{
			ar(value, ease, eyesValue);
		}
	};

	struct AnimationTimeline
	{
		uint8_t morph;
		bool isEyes = false;
		std::map<double, Keyframe> keys;
		std::map<double, Keyframe>::iterator cachedNextIter;
		std::map<double, Keyframe>::iterator cachedPrevIter;

		AnimationTimeline()
		{
		}

		AnimationTimeline(std::map<double, Keyframe> _keys, uint8_t _morph, bool _isEyes) :
			keys(_keys), morph(_morph), isEyes(_isEyes)
		{
			cachedNextIter = keys.begin();
			cachedPrevIter = keys.begin();
		}

		template <class Archive>
		void save(Archive& ar, const uint32_t) const
		{
			ar(morph, isEyes, keys);
		}

		template <class Archive>
		void load(Archive& ar, const uint32_t)
		{
			ar(morph, isEyes, keys);
			cachedNextIter = keys.begin();
			cachedPrevIter = keys.begin();
		}

		float GetValueAtTime(double t)
		{
			if (cachedNextIter->first < t || cachedPrevIter->first > t) {
				auto nextKey = keys.lower_bound(t);

				if (nextKey == keys.end()) {
					return std::prev(nextKey)->second.value;
				} else if (nextKey == keys.begin() || keys.size() < 2) {
					return nextKey->second.value;
				} else {
					cachedNextIter = nextKey;
					cachedPrevIter = std::prev(nextKey);
				}
			}

			auto totalDiff = cachedNextIter->first - cachedPrevIter->first;
			auto currentDiff = t - cachedPrevIter->first;

			auto normalizedTime = currentDiff * (1 / totalDiff);
			return static_cast<float>(std::lerp(cachedPrevIter->second.value, cachedNextIter->second.value, Easing::Ease(normalizedTime, cachedPrevIter->second.ease)));
		}

		EyeVector GetEyesValueAtTime(double t)
		{
			if (cachedNextIter->first < t || cachedPrevIter->first > t) {
				auto nextKey = keys.lower_bound(t);

				if (nextKey == keys.end()) {
					return std::prev(nextKey)->second.eyesValue;
				} else if (nextKey == keys.begin() || keys.size() < 2) {
					return nextKey->second.eyesValue;
				} else {
					cachedNextIter = nextKey;
					cachedPrevIter = std::prev(nextKey);
				}
			}

			auto totalDiff = cachedNextIter->first - cachedPrevIter->first;
			auto currentDiff = t - cachedPrevIter->first;

			auto normalizedTime = currentDiff * (1 / totalDiff);
			EyeVector interpVec;
			interpVec.u = std::lerp(cachedPrevIter->second.eyesValue.u, cachedNextIter->second.eyesValue.u, Easing::Ease(normalizedTime, cachedPrevIter->second.ease));
			interpVec.v = std::lerp(cachedPrevIter->second.eyesValue.v, cachedNextIter->second.eyesValue.v, Easing::Ease(normalizedTime, cachedPrevIter->second.ease));
			return interpVec;
		}
	};

	struct AnimationData
	{
		std::vector<AnimationTimeline> timelines;
		double duration = 0.00001;

		template <class Archive>
		void serialize(Archive& ar, const uint32_t)
		{
			ar(timelines, duration);
		}
	};

	struct FrameBasedTimeline
	{
		uint8_t morph;
		bool isEyes = false;
		std::map<int32_t, Keyframe> keys;
	};

	struct XMLKeyInfo
	{
		uint8_t morph;
		bool isEyes = false;
		Keyframe key;
	};

	struct FrameBasedAnimData
	{
		std::vector<FrameBasedTimeline> timelines;
		int32_t duration = 60;
		int32_t frameRate = 30;

		FrameBasedTimeline* MakeTimeline(uint8_t morph, bool isEyes = false) {
			timelines.push_back(FrameBasedTimeline{ morph, isEyes });
			return &timelines[timelines.size() - 1];
		}

		FrameBasedTimeline* GetTimeline(uint8_t morph, bool isEyes = false, bool create = true) {
			FrameBasedTimeline* res = nullptr;

			if (!isEyes) {
				for (auto& tl : timelines) {
					if (tl.morph == morph && !tl.isEyes) {
						res = &tl;
						break;
					}
				}
			} else {
				for (auto& tl : timelines) {
					if (tl.isEyes == true) {
						res = &tl;
						break;
					}
				}
			}

			if (!res && create) {
				timelines.push_back(FrameBasedTimeline{ morph, isEyes });
				res = &timelines[timelines.size() - 1];
			}

			return res;
		}

		void RemoveEmptyTimelines() {
			for (auto it = timelines.begin(); it != timelines.end();) {
				if (it->keys.size() < 1) {
					it = timelines.erase(it);
				} else {
					it++;
				}
			}
		}

		AnimationData ToRuntimeData() {
			AnimationData res;
			ToRuntimeData(&res);
			return res;
		}

		void ToRuntimeData(AnimationData* d)
		{
			d->timelines.clear();
			d->duration = static_cast<double>(duration) / static_cast<double>(frameRate);
			std::map<double, Keyframe> convertedFrames;
			for (auto& tl : timelines) {
				convertedFrames.clear();
				for (auto& k : tl.keys) {
					convertedFrames.insert({ (static_cast<double>(k.first) / static_cast<double>(frameRate)) / d->duration, k.second });
				}
				d->timelines.push_back(AnimationTimeline(convertedFrames, tl.morph, tl.isEyes));
			}
		}

		static FrameBasedAnimData FromRuntimeData(const AnimationData& runtimeData, int32_t frameRate = 30)
		{
			FrameBasedAnimData res;
			res.frameRate = frameRate;
			res.duration = static_cast<int32_t>(runtimeData.duration * static_cast<double>(frameRate));
			std::map<int32_t, Keyframe> convertedFrames;
			for (auto& tl : runtimeData.timelines) {
				convertedFrames.clear();
				for (auto& k : tl.keys) {
					convertedFrames.insert({ static_cast<int32_t>((k.first * runtimeData.duration) * static_cast<double>(frameRate)), k.second });
				}
				res.timelines.push_back(FrameBasedTimeline{ tl.morph, tl.isEyes, convertedFrames });
			}
			return res;
		}

		void ToXML(const std::string& id, pugi::xml_document& outDoc)
		{
			auto topNode = outDoc.append_child("faceAnim");
			topNode.append_attribute("id").set_value(id.c_str());
			topNode.append_attribute("frames").set_value(duration);

			std::map<int32_t, std::vector<XMLKeyInfo>> keysMap;
			for (auto& tl: timelines) {
				for (auto& k : tl.keys) {
					keysMap[k.first].push_back({ tl.morph, tl.isEyes, k.second });
				}
			}

			for (auto& k : keysMap) {
				auto keyNode = topNode.append_child("key");
				keyNode.append_attribute("frame").set_value(k.first);
				for (auto& info : k.second) {
					if (!info.isEyes) {
						auto m = keyNode.append_child("morph");
						m.append_attribute("id").set_value(info.morph);
						m.append_attribute("value").set_value(static_cast<int32_t>((info.key.value * 100) + 0.1f));
					} else {
						auto e = keyNode.append_child("eyes");
						e.append_attribute("x").set_value(std::format("{:.2f}", ConvertEyeCoordRange(info.key.eyesValue.u, true, false)).c_str());
						e.append_attribute("y").set_value(std::format("{:.2f}", ConvertEyeCoordRange(info.key.eyesValue.v, false, false)).c_str());
					}
				}
			}
		}
	};
}
