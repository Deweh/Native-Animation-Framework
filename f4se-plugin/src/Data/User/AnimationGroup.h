#pragma once

namespace Data
{
	class AnimationGroup : public IdentifiableObject
	{
	public:
		struct Stage
		{
			std::string animation;
			uint32_t loopMin = 1;
			uint32_t loopMax = 1;
			uint32_t weight = 1;

			void Parse(XMLUtil::Mapper& m) {
				m(&animation, ""s, false, true, "AnimationGroup stage has no 'animation' attribute!", "animation");
				std::string loops = "";
				m(&loops, ""s, false, false, "", "loops");
				if (loops.size() > 0) {

					size_t delimiterPos = loops.find("-");
					if (delimiterPos == std::string::npos) {
						if (auto num = Utility::StringToUInt32(loops); num.has_value()) {
							loopMin = num.value();
							loopMax = num.value();
						}
					} else {
						if (auto minNum = Utility::StringToUInt32(loops.substr(0, delimiterPos)); minNum.has_value()) {
							loopMin = minNum.value();
						}
						delimiterPos += 1;
						if (auto maxNum = Utility::StringToUInt32(loops.substr(delimiterPos, loops.size() - delimiterPos)); maxNum.has_value()) {
							loopMax = maxNum.value();
						}
						if (loopMax < loopMin)
							loopMax = loopMin;
					}
				}
				m(&weight, 1ui32, false, false, "", "weight");
			}

			template <class Archive>
			void serialize(Archive& ar, const uint32_t)
			{
				ar(animation, loopMin, loopMax, weight);
			}
		};

		std::shared_ptr<const Animation> GetBaseAnimation() const {
			if (stages.size() < 1) {
				return nullptr;
			}
			return Data::GetAnimation(stages[0].animation);
		}

		static bool Parse(XMLUtil::Mapper& m, AnimationGroup& out) {
			out.ParseID(m);
			m(&out.sequential, false, true, false, "", "sequential");

			out.stages = m.ParseArray<Stage>("stage", "AnimationGroup has no stages!");

			return m;
		}

		bool sequential;
		std::vector<Stage> stages;

		template <class Archive>
		void serialize(Archive& ar, const uint32_t)
		{
			ar(sequential, stages);
		}
	};
}
