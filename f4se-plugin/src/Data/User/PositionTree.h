#pragma once

namespace Data
{
	class PositionTree : public IdentifiableObject
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

			//do not serialize children -- may cause potential recursion issues.
			//instead, the PositionTree class converts the tree to a flat representation
			//and back using the parent node.
			template <class Archive>
			void serialize(Archive& ar, const uint32_t)
			{
				ar(id, position, parent, duration, forceComplete);
			}
		};

		struct NodeParseData
		{
			pugi::xml_node xmlNode;
			std::shared_ptr<Node> dataNode;
		};

		static bool Parse(XMLUtil::Mapper& m, PositionTree& out) {
			out.ParseID(m);

			m.DownNode("branch", "PositionTree has no root branch!");
			out.tree = std::make_shared<Node>();
			m(&out.tree->id, ""s, false, false, "", "id");
			m(&out.tree->position, ""s, false, true, "PositionTree branch has no 'positionID' attribute!", "positionID");
			m(&out.tree->duration, 0.0f, false, false, "", "time");
			m(&out.tree->forceComplete, false, false, false, "", "forceComplete");

			if (m && out.tree->id.size() < 1) {
				out.tree->id = "Unknown";
			}

			std::stack<NodeParseData> pendingNodes;
			pendingNodes.push({ m.GetCurrentNode(), out.tree });

			while (!pendingNodes.empty()) {
				auto currentParent = pendingNodes.top().dataNode;
				m.SetCurrentNode(&pendingNodes.top().xmlNode);
				pendingNodes.pop();

				m.GetArray([&](XMLUtil::Mapper& m) {
					std::shared_ptr<Node> currentNode = std::make_shared<Node>();
					m(&currentNode->id, ""s, false, false, "", "id");
					m(&currentNode->position, ""s, false, true, "PositionTree branch has no 'positionID' attribute!", "positionID");
					m(&currentNode->duration, 0.0f, false, false, "", "time");
					m(&currentNode->forceComplete, false, false, false, "", "forceComplete");

					if (m) {
						if (currentNode->id.size() < 1)
							currentNode->id = "Unknown";

						currentParent->children.push_back(currentNode);
						currentNode->parent = currentParent;
						pendingNodes.push({ m.GetCurrentNode(), currentNode });
					}

					return m;
				}, "branch", "", false);
			}

			return m;
		}

		std::vector<std::shared_ptr<Node>> ToFlat() const
		{
			std::vector<std::shared_ptr<Node>> result;
			std::stack<std::shared_ptr<Node>> pendingNodes;
			pendingNodes.push(tree);

			while (!pendingNodes.empty()) {
				auto cur = pendingNodes.top();
				pendingNodes.pop();
				result.push_back(cur);

				for (auto& c : cur->children) {
					pendingNodes.push(c);
				}
			}

			return result;
		}

		void FromFlat(std::vector<std::shared_ptr<Node>>& in)
		{
			if (in.size() > 0) {
				tree = in[0];
			}

			if (in.size() > 1) {
				for (size_t i = 1; i < in.size(); i++) {
					auto& c = in[i];
					auto p = c->parent.lock();
					if (p != nullptr) {
						p->children.push_back(c);
					}
				}
			}
		}

		std::shared_ptr<Node> tree;

		template <class Archive>
		void save(Archive& ar, const uint32_t) const
		{
			ar(ToFlat());
		}

		template <class Archive>
		void load(Archive& ar, const uint32_t)
		{
			std::vector<std::shared_ptr<Node>> flat;
			ar(flat);
			FromFlat(flat);
		}
	};
}
