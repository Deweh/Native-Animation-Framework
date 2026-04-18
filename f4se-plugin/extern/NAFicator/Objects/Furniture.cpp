#pragma once
#include "Furniture.h"

namespace NAFicator
{
	bool Furniture::is_valid()
	{
		bool v = (valid && !id.empty() && !furn.empty());
		if (valid && !v)
			set_invalid("Failed object validation!");
		return valid;
	}
	
	Data::XMLUtil::Mapper& Furniture::parse_id(Data::XMLUtil::Mapper& m)
	{
		return ParsedObject::parse_id(m);
	}

	bool Furniture::parse(Data::XMLUtil::Mapper& m)
	{
		parse_id(m);
		std::string tmp("");

		m.GetArray([&](Data::XMLUtil::Mapper& m) {
			std::unordered_map<std::string, std::string> furnitureItem;  // Создаем временный объект

			// Получаем значения и добавляем их в временный объект
			m(&tmp, Data::XMLUtil::Mapper::emptyStr, true, false, "", "source");
			if (!tmp.empty()) {
				furnitureItem.emplace("source", normalize_source(tmp));
				tmp.clear();
			}

			m(&tmp, Data::XMLUtil::Mapper::emptyStr, false, false, "", "form");
			if (!tmp.empty()) {

				furnitureItem.emplace("form",normalize_form_id(tmp));
				tmp.clear();
			}

			m(&tmp, Data::XMLUtil::Mapper::emptyStr, false, false, "", "id");
			if (!tmp.empty()) {
				furnitureItem.emplace("id",tmp);
				tmp.clear();
			}

			m(&tmp, Data::XMLUtil::Mapper::emptyStr, false, false, "", "keyword");
			if (!tmp.empty()) {
				furnitureItem.emplace("keyword",tmp);
				tmp.clear();
			}

			furn.push_back(std::move(furnitureItem));
			return m;
		},
			"furniture", "Furniture group has no 'furniture' nodes!");

		m(&startAnimation, std::optional<std::string>(std::nullopt), true, false, "", "startAnimation");
		m(&stopAnimation, std::optional<std::string>(std::nullopt), true, false, "", "stopAnimation");

		return m;
	}

	std::ostringstream& Furniture::ssprint(std::ostringstream& s) const
	{
		auto print_furn_nodes = [](const std::unordered_map<std::string, std::string>& f, std::ostringstream& s) {
			auto get_value = [&](const std::string& key) {
				auto it = f.find(key);
				return (it != f.end()) ? it->second : Data::XMLUtil::Mapper::emptyStr;
			};

			s << "\t\t<furniture form=\"" << get_value("form") << "\" source=\"" << get_value("source") << "\"";

			const std::string& id = get_value("id");
			if (!id.empty()) {
				s << " id=\"" << id << "\"";
			}

			const std::string& keyword = get_value("keyword");
			if (!keyword.empty()) {
				s << " keyword=\"" << keyword << "\"";
			}

			s << "/>\n";
		};

		s << "\n<!-- FILE : " << filename << (valid ? " -->\n" : "\n");
		s << ((!valid && !log.empty()) ? (log + "\n\n"s) : "");

		if (id.empty()) {
			s << "\t<group id=\"" << get_unknown_id() << "\"";
		} else {
			s << "\t<group id=\"" << id << "\"";
		}
		if (startAnimation)
			s << " startAnimation=\"" << startAnimation.value() << "\"";
		if (stopAnimation)
			s << " stopAnimation=\"" << stopAnimation.value() << "\"";
		s << ">\n";

		for (const auto& f : furn) {
			print_furn_nodes(f, s);
		}
		s << "\t</group>" << (valid ? "" : " -->");
		
		return s;
	}

	void Furniture::check_links_to_forms_and_clear_badlinks()
	{
		for (auto it = furn.begin(); it != furn.end();) {
			auto to_string_hex = [](size_t value) {
				std::stringstream stream;
				stream << "0x" << std::hex << value;
				return stream.str();
			};

			// Проверка на наличие "source" и его валидность
			if (!it->contains("source") ||
				!RE::TESDataHandler::GetSingleton()->LookupModByName((*it)["source"])) {
				it = furn.erase(it);
				continue;
			}

			// Обработка "form"
			if (it->contains("form")) {
				auto kwd = get_form_from_string((*it)["form"], (*it)["source"]);
				if (kwd) 
				{
					++it;
					continue;
				}
			}

			// Обработка "id"
			if (it->contains("id")) {
				if (auto kwd = get_form_by_editor_id((*it)["id"]); kwd && kwd->formType == RE::ENUM_FORM_ID::kKYWD) {
					auto hex = to_string_hex(kwd->formID);
					(*it)["form"] = normalize_form_id(hex);
					++it;
					continue;
				}
			}

			// Удаление элемента, если ни одно из условий не выполнено
			it = furn.erase(it);
		}
		is_valid();
	}
}
