#pragma once
#include "PositionTree.h"

namespace NAFicator
{
	bool PositionTree::is_valid()
	{
		bool v = valid && !id.empty() && tree;
		if (valid && !v)
			set_invalid("Failed object validation!");
		return valid;
	}
	
	Data::XMLUtil::Mapper& PositionTree::parse_id(Data::XMLUtil::Mapper& m) 
	{
		return ParsedObject::parse_id(m);
	}
	bool PositionTree::parse(Data::XMLUtil::Mapper& m) 
	{
		parse_id(m);

		m.DownNode("branch", "PositionTree has no root branch!");
		tree = std::make_shared<Node>();
		m(&tree->id, ""s, false, false, "", "id");
		m(&tree->position, ""s, false, true, "PositionTree branch has no 'positionID' attribute!", "positionID");
		m(&tree->duration, 0.0f, false, false, "", "time");
		m(&tree->forceComplete, false, false, false, "", "forceComplete");

		if (m && tree->id.size() < 1) {
			tree->id = "Unknown";
		}

		pendingNodes.push({ m.GetCurrentNode(), tree });

		while (!pendingNodes.empty()) {
			auto currentParent = pendingNodes.top().dataNode;
			m.SetCurrentNode(&pendingNodes.top().xmlNode);
			pendingNodes.pop();

			m.GetArray([&](Data::XMLUtil::Mapper& m) {
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
			},
				"branch", "", false);
		}

		return m;
	}

	std::vector<std::shared_ptr<PositionTree::Node>> PositionTree::ToFlat() const
	{
		std::vector<std::shared_ptr<Node>> result;
		std::stack<std::shared_ptr<Node>> _pendingNodes;
		_pendingNodes.push(tree);

		while (!_pendingNodes.empty()) {
			auto cur = _pendingNodes.top();
			_pendingNodes.pop();
			result.push_back(cur);

			for (auto& c : cur->children) {
				_pendingNodes.push(c);
			}
		}

		return result;
	}

	void PositionTree::FromFlat(std::vector<std::shared_ptr<Node>>& in)
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

	std::ostringstream& PositionTree::ssprint(std::ostringstream& s) const
	{
		
		s << "\n<!-- FILE : " << filename << (valid ? " -->\n" : "\n");
		s << ((!valid && !log.empty()) ? (log + "\n\n"s) : "");

		if (id.empty()) {
			s << "\t<tree id=\"" << get_unknown_id() << "\" strictExiting=\"true\">\n";
		} else {
			s << "\t<tree id=\"" << id << "\" strictExiting=\"true\">\n";
		}
		ssprint_branch(s, tree, 2);  // Начинаем с глубины 2 для первого уровня вложенности
		s << "\t</tree>" << (valid ? "" : " -->");
		return s;
	}

	std::ostringstream& PositionTree::ssprint_branch(std::ostringstream& s, const std::shared_ptr<Node>& node, int depth) const
	{
		s << std::string(depth, '\t');  // Увеличиваем количество табуляций на основе глубины
		if (node->id.empty()) {
			s << "<branch id=\"" << get_unknown_id() << "\" positionID=\"" << node->position << "\"";
		} else {
			s << "<branch id=\"" << node->id << "\" positionID=\"" << node->position << "\"";
		}
		if (node->forceComplete) {
			s << " forceComplete=\"true\"";
		}
		s << " time=\"" << node->duration << "\">\n";

		for (const auto& child : node->children) {
			ssprint_branch(s, child, depth + 1);  // Увеличиваем глубину для дочерних узлов
		}

		s << std::string(depth, '\t');  // Возвращаемся к текущему уровню
		s << "</branch>\n";
		return s;
	}

	void PositionTree::traverse_tree(const std::function<void(const std::shared_ptr<PositionTree::Node>&)>& visit, const std::shared_ptr<Node>& node) const
	{
		if (!node)
			return;

		visit(node);

		for (const auto& child : node->children) {
			traverse_tree(visit, child);
		}
	}

	std::shared_ptr<PositionTree::Node> PositionTree::find_position(const std::string& positionId)
	{
		std::shared_ptr<PositionTree::Node> result = nullptr;
		traverse_tree([&](const std::shared_ptr<Node>& node) {
			if (!result && (node->position == positionId)) {
				result = node;
			}
		}, tree);
		return result;
	}
}
