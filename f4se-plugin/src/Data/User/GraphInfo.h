#pragma once

namespace Data
{
	class GraphInfo : public IdentifiableObject
	{
	public:
		struct IKChain
		{
			std::vector<std::string> nodes;
			std::optional<std::string> effectorNode = std::nullopt;
			std::optional<std::string> poleNode = std::nullopt;
			std::string poleParent = "";
			RE::NiPoint3 poleStartPos;
			bool controlsTranslation = false;
		};

		std::vector<std::string> nodeList;
		std::map<std::string, IKChain> chains;
		std::optional<std::string> basePoseFile = std::nullopt;
		std::vector<RE::NiTransform> skeletonPose;

		static bool Parse(XMLUtil::Mapper& m, GraphInfo& out)
		{
			out.ParseID(m);
			std::string idStr;
			m.GetArray([&](XMLUtil::Mapper& m) {
				m(&idStr, ""s, false, true, "Graph node has no 'id' attribute!", "id");

				if (m) {
					out.nodeList.push_back(idStr);
				}
				
				return m;
			}, "node", "Graph has no nodes!");

			std::string ikStr;
			m.GetArray([&](XMLUtil::Mapper& m) {
				IKChain c;
				m(&idStr, ""s, false, true, "Graph chain has no 'id' attribute!", "id");
				m.GetArray([&](XMLUtil::Mapper& m) {
					m(&ikStr, ""s, false, true, "Graph chain node has no 'id' attribute!", "id");

					if (m) {
						c.nodes.push_back(ikStr);
					}
					
					return m;
				}, "node", "Graph chain has no nodes!");

				if (c.nodes.size() > UINT16_MAX) {
					m.CustomFail(std::format("Graph chain cannot have more than {} nodes!", UINT16_MAX));
				}

				m(&c.effectorNode, std::optional<std::string>(std::nullopt), false, false, "", "effectorNode");
				m(&c.poleNode, std::optional<std::string>(std::nullopt), false, false, "", "poleNode");
				m(&c.poleParent, ""s, true, false, "", "poleParent");
				m(&c.poleStartPos.x, 0.0f, true, false, "", "poleStartX");
				m(&c.poleStartPos.y, 0.0f, true, false, "", "poleStartY");
				m(&c.poleStartPos.z, 0.0f, true, false, "", "poleStartZ");
				m(&c.controlsTranslation, false, true, false, "", "controlsTranslation");

				if (m) {
					out.chains[idStr] = c;
				}
				
				return m;
			}, "chain", "", false);

			m(&out.basePoseFile, std::optional<std::string>(std::nullopt), false, false, "", "basePoseFile");

			return m;
		}
	};
}
