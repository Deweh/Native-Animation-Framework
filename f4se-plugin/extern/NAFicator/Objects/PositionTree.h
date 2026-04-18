#pragma once
#include "ParsedObject.h"
namespace NAFicator
{
	class PositionTree :
		public ParsedObject
	{
	public:
		struct Node
		{
			std::string id;
			std::string position;
			std::weak_ptr<Node> parent;
			std::vector<std::shared_ptr<Node>> children;
			float duration = 0.0f;
			bool forceComplete = false;
		};

		struct NodeParseData
		{
			pugi::xml_node xmlNode;
			std::shared_ptr<Node> dataNode;
		};

		std::shared_ptr<Node> tree;
		std::stack<NodeParseData> pendingNodes;

		PositionTree(Data::XMLUtil::Mapper& m) :
			ParsedObject(kPositionTree) { parse(m); };

		bool parse(Data::XMLUtil::Mapper& m) override;
		Data::XMLUtil::Mapper& parse_id(Data::XMLUtil::Mapper& m) override;
		std::ostringstream& ssprint(std::ostringstream& s) const override;
		bool is_valid() override;

		std::ostringstream& ssprint_branch(std::ostringstream& s, const std::shared_ptr<Node>& node, int depth) const;
		std::vector<std::shared_ptr<Node>> ToFlat() const;
		void FromFlat(std::vector<std::shared_ptr<Node>>& in);

		std::shared_ptr<Node> find_position(const std::string& positionId);
		void traverse_tree(const std::function<void(const std::shared_ptr<Node>&)>& visit, const std::shared_ptr<Node>& tree) const;
	};
}
