#pragma once
#include "ParsedObject.h"
#include "Tag.h"
#include "Action.h"
#include "Animation.h"
#include "AnimationGroup.h"
#include "EquipmentSet.h"
#include "Furniture.h"
#include "MfgSet.h"
#include "MorphSet.h"
#include "objects_map.h"
#include "Overlay.h"
#include "Position.h"
#include "PositionTree.h"
#include "ProtectedEquipment.h"
#include "Race.h"
#include <IniParser/Ini.h>

extern ini::map inimap;

namespace NAFicator
{
	bool ParsedObject::is_valid()
	{
		return valid;
	}

	void ParsedObject::set_invalid(const std::string& to_log)
	{
		if (log.find(to_log) == std::string::npos)
			log += to_log + "\n";
		valid = false;
	}
	
	std::size_t CaseInsensitivePairObjTypeStringHash::operator()(const std::pair<obj_type, std::string>& t) const
	{
		std::size_t hash = std::_FNV_offset_basis;
		std::locale loc;  // Используем локаль по умолчанию

		// Хэш для obj_type
		hash ^= static_cast<std::size_t>(t.first) + 0x9e3779b9 + (hash << 6) + (hash >> 2);

		// Хэш для строки с учетом локали
		for (char c : t.second) {
			hash ^= static_cast<std::size_t>(std::tolower(static_cast<unsigned char>(c), loc));
			hash *= 0x1000193;  // FNV prime
		}

		return hash;
	}

	bool CaseInsensitivePairObjTypeStringEqual::operator()(const std::pair<obj_type, std::string>& lhs, const std::pair<obj_type, std::string>& rhs) const
	{
		if (lhs.first != rhs.first) {
			return false;
		}

		// Оптимизация: проверка длины строк
		if (lhs.second.size() != rhs.second.size()) {
			return false;
		}

		std::locale loc;  // Используем локаль по умолчанию

		return std::equal(lhs.second.begin(), lhs.second.end(), rhs.second.begin(),
			[&loc](char a, char b) { return std::tolower(static_cast<unsigned char>(a), loc) == std::tolower(static_cast<unsigned char>(b), loc); });
	}

	//void priority_insert(std::shared_ptr<ParsedObject> ele)
	//{
	//	auto pair = std::make_pair(ele->type, ele->id);

	//	auto it = parsed_objects.find(pair);
	//	if (it == parsed_objects.end()) {
	//		parsed_objects.insert(std::make_pair(pair, std::make_pair(RE::BSSpinLock(), ele)));
	//	} else if (auto& [l, ptr] = it->second; !ptr->is_valid() && ele->is_valid()) {
	//		RE::BSAutoLock lock(l);
	//		ptr = ele;
	//	}
	//	else {
	//		if (ptr && ele->type == kFurniture) {
	//			// Лямбда-функция для объединения уникальных векторов
	//			auto mergeUniqueFurn = [](const std::list<std::unordered_map<std::string, std::string>>& furn1,
	//									   const std::list<std::unordered_map<std::string, std::string>>& furn2,
	//									   std::list<std::unordered_map<std::string, std::string>>& mergedFurn) {
	//				// Множество для хранения уникальных значений
	//				std::unordered_set<std::string> uniqueValues;

	//				// Лямбда-функция для обработки каждого вектора
	//				auto processFurn = [&](const std::list<std::unordered_map<std::string, std::string>>& furn) {
	//					for (const auto& item : furn) {
	//						// Для каждого ключа в элементе
	//						for (const auto& [key, value] : item) {
	//							// Проверяем уникальность и добавляем в новый вектор
	//							if (uniqueValues.insert(key).second) {
	//								mergedFurn.push_back(item);
	//								break; // Прерываем, если добавили элемент
	//							}
	//						}
	//					}
	//				};

	//				// Обработка обоих векторов
	//				processFurn(furn1);
	//				processFurn(furn2);
	//			};

	//			auto& element = static_cast<Furniture*>(ele.get())->furn;
	//			auto& target = static_cast<Furniture*>(it->second.second.get())->furn;
	//			std::list<std::unordered_map<std::string, std::string>> tmp;

	//			// Вызов функции объединения
	//			RE::BSAutoLock lock(l);
	//			mergeUniqueFurn(target, element, tmp);

	//			// Обновление target с уникальными элементами
	//			target = tmp;
	//		} else {
	//			// Проверка и замена указателя, если новый объект имеет более высокий приоритет
	//			
	//			if (!ptr || ptr->loadPriority < ele->loadPriority) {
	//				if (ptr) {
	//					RE::BSAutoLock lock(l);
	//					ptr = ele;
	//				}
	//			}
	//		}
	//	}
	//}

	void priority_insert(std::shared_ptr<ParsedObject> ele)
	{
		auto pair = std::make_pair(ele->type, ele->id);
		auto it = parsed_objects.find(pair);

		// Если объект не существует, добавляем новый
		if (it == parsed_objects.end()) {
			parsed_objects.insert(std::make_pair(pair, std::make_pair(RE::BSSpinLock(), ele)));
		} else {
			if (!ele->is_valid())
				return;
			auto& [l, ptr] = it->second;
			// Проверяем валидность указателя
			if (!ptr->is_valid() && ele->is_valid()) {
				RE::BSAutoLock lock(l);
				ptr = ele;
			} else if (ptr && ele->type == kTag) {
				auto ptr_ele = static_cast<Tag*>(ele.get());
				auto ptr_tag = static_cast<Tag*>(ptr.get());
				RE::BSAutoLock lock(l);
				if (ptr_ele->replace) {
					if (ptr_tag->get_priority() < ptr_ele->get_priority())
						*ptr_tag = *ptr_ele;
				} else {
					ptr_tag->merge_tags(*ptr_ele);
				}
			} else if (ptr && ele->type == kFurniture) {
				// Обработка для типа kFurniture
				auto f_ptr = static_cast<Furniture*>(ptr.get());
				auto f_ele = static_cast<Furniture*>(ele.get());

				// Лямбда-функция для поиска в массиве
				auto contains = [](auto& furn, const std::string& form, const std::string& source) {
					for (auto it = furn.begin(); it != furn.end(); ++it) {
						auto aform = it->find("form");
						auto asource = it->find("source");
						if (aform != it->end() && asource != it->end()) {
							if (aform->second == form && asource->second == source) {
								return it;
							}
						}
					}
					return furn.end();  // Исправлено: добавлено возвращение end()
				};

				decltype(f_ptr->furn) tmp;

				// Проверяем наличие элементов в f_ptr->furn
				RE::BSAutoLock lock(l);
				if (!f_ptr->furn.empty()) {
					for (auto& p : f_ele->furn) {
						auto form = p.find("form");
						auto source = p.find("source");
						if (form != p.end() && source != p.end()) {
							auto iter = contains(f_ptr->furn, form->second, source->second);
							if (iter != f_ptr->furn.end()) {
								// Если новый элемент имеет более высокий приоритет
								if (f_ptr->loadPriority < f_ele->loadPriority && f_ele->furn.size() > 2) {
									*iter = p;  // Обновляем существующий элемент
								} else if (p.contains("id") && !iter->contains("id")) {
									iter->emplace("id", p.at("id"));  // Добавляем id, если его нет
								}
							} else {
								tmp.emplace_back(p);  // Добавляем новый элемент
							}
						}
					}

					// Добавляем новые элементы из tmp в f_ptr->furn
					if (!tmp.empty()) {
						f_ptr->furn.insert(f_ptr->furn.end(), tmp.begin(), tmp.end());
					}
				} else {
					// Если f_ptr->furn пустой, просто присваиваем
					f_ptr->furn = f_ele->furn;
				}
			} else {
				// Проверка и замена указателя, если новый объект имеет более высокий приоритет
				if (!ptr || ptr->loadPriority < ele->loadPriority) {
					RE::BSAutoLock lock(l);
					ptr = ele;
				}
			}
		}
	}


	//0 stream, 1 root, 2 filename
	bool parse_XML_files(std::vector<std::shared_ptr<std::stringstream>>& xfiles)
	{
		concurrency::parallel_for_each(xfiles.begin(), xfiles.end(), [](const auto& x) {
			if (x)
				parse_xml(x);
		});
		return true;
	}

	Data::XMLUtil::Mapper& ParsedObject::parse_id(Data::XMLUtil::Mapper& m)
	{
		m(&id, ""s, true, true, "Node has no 'id' attribute!", "id");
		m.GetMinMax(&loadPriority, 0, true, false, "", INT32_MIN, INT32_MAX, "loadPriority");
		filename = m.GetFileName();
		return m;
	}

	std::string_view ParsedObject::get_type_view()
	{
		return ::NAFicator::get_type_view(type);
	}

	std::string_view get_type_view(obj_type kType)
	{
		switch (kType) {
		case kAction:
			return "actionData"sv;
		case kAnimation:
			return "animationData"sv;
		case kAnimationGroup:
			return "animationGroupData"sv;
		case kEquipmentSet:
			return "equipmentSetData"sv;
		case kFurniture:
			return "furnitureData"sv;
		case kMfgSet:
			return "mfgSetData"sv;
		case kMorphSet:
			return "morphSetData"sv;
		case kOverlay:
			return "overlayData"sv;
		case kPosition:
			return "positionData"sv;
		case kPositionTree:
			return "positionTreeData"sv;
		/*case kProtectedEquipment:
			return "protectedEquipmentData"sv;*/
		case kRace:
			return "raceData"sv;
			/*case kTag:
			return "tagData"sv;*/
		default:
			return ""sv;
		}
	}

	obj_type get_type(std::string_view root)
	{
		if (root == "actionData"sv)
			return kAction;
		else if (root == "animationData"sv)
			return kAnimation;
		else if (root == "animationGroupData"sv)
			return kAnimationGroup;
		else if (root == "equipmentSetData"sv)
			return kEquipmentSet;
		else if (root == "furnitureData"sv)
			return kFurniture;
		else if (root == "mfgSetData"sv)
			return kMfgSet;
		else if (root == "morphSetData"sv)
			return kMorphSet;
		else if (root == "overlayData"sv)
			return kOverlay;
		else if (root == "positionData"sv)
			return kPosition;
		else if (root == "positionTreeData"sv)
			return kPositionTree;
		else if (root == "protectedEquipmentData"sv)
			return kProtectedEquipment;
		else if (root == "raceData"sv)
			return kRace;
		else if (root == "tagData"sv)
			return kTag;
		else
			return kNone;
	}

	static std::map<const std::string_view, const std::string_view> nodeMapping
	{
		{ "animationData", "animation" },
		{ "raceData", "race" },
		{ "positionData", "position" },
		{ "morphSetData", "morphSet" },
		{ "equipmentSetData", "equipmentSet" },
		{ "actionData", "action" },
		{ "animationGroupData", "animationGroup" },
		{ "furnitureData", "group" },
		{ "positionTreeData", "tree" },
		{ "tagData", "tag" },
		{ "mfgSetData", "mfgSet" },
		{ "overlayData", "overlaySet" },
		{ "protectedEquipmentData", "condition" }
	};

	bool parse_xml(std::shared_ptr<std::stringstream> x)
	{
		std::string xml_content = x->str();
		std::vector<char> buffer(xml_content.begin(), xml_content.end());

		pugi::xml_document doc;
		const pugi::xml_parse_result result = doc.load_buffer(buffer.data(), buffer.size());

		if (!result) {
			logger::error("❌ Failed to parse, message: {} at character {}", result.description(), result.offset);
			return false;
		}

		pugi::xml_node topNode = doc;
		std::string root_node = topNode.first_child().name();
		std::string filename = topNode.first_child().find_attribute(
			[](const auto& attrib) -> bool {
				return !std::strcmp(attrib.name(), "filename");
			})
		.as_string("");

		if (topNode.empty()) {
			return false;
		}

		if (root_node.empty()) {
			return false;
			}

		if (filename.empty()) {
			return false;
		}

		for (const auto& n : nodeMapping) {
			if (auto data = doc.child(n.first.data()); !data.empty()) {
				topNode = data;
				break;
			}
		}

		auto defaults = topNode.child("defaults");

		if (nodeMapping.contains(root_node)) {
			auto childNodes = topNode.children(nodeMapping[root_node].data());

			auto m = Data::XMLUtil::Mapper(defaults, topNode, filename);
			m.verbose = true;

			for (const auto& node : childNodes) {
				m.ResetSuccessFlag();
				m.SetCurrentNode(&node);

				std::shared_ptr<ParsedObject> obj;

				switch (get_type(root_node)) {
				case kAction:
					obj = std::make_shared<Action>(m);
					break;
				case kAnimation:
					obj = std::make_shared<Animation>(m);
					break;
				case kAnimationGroup:
					obj = std::make_shared<AnimationGroup>(m);
					break;
				case kEquipmentSet:
					obj = std::make_shared<EquipmentSet>(m);
					break;
				case kFurniture:
					obj = std::make_shared<Furniture>(m);
					break;
				case kMfgSet:
					obj = std::make_shared<MfgSet>(m);
					break;
				case kMorphSet:
					obj = std::make_shared<MorphSet>(m);
					break;
				case kOverlay:
					obj = std::make_shared<Overlay>(m);
					break;
				case kPosition:
					obj = std::make_shared<Position>(m);
					break;
				case kPositionTree:
					obj = std::make_shared<PositionTree>(m);
					break;
				case kRace:
					obj = std::make_shared<Race>(m);
					break;
				case kTag:
					obj = std::make_shared<Tag>(m);
					break;
				case kProtectedEquipment:
					ProtectedEquipment pe;
					pe(m);
					break;
				default:
					logger::warn("❌ Unknown object type. Filename {}", filename);
					continue;  // Пропустить итерацию, если тип не известен
				}

				if (obj && obj->is_valid()) {
					std::ostringstream s;
					logger::info("✅ Parsed success : {} - {}", filename, root_node);
					priority_insert(obj);
				} else {
					if (get_type(root_node) == kProtectedEquipment)
						logger::info("✅ Parsed success : {} - {}", filename, root_node);
					else
						logger::warn("❌ Invalid object. Filename {}", filename);
				}
			}
		} else {
			logger::error("❌ Not valid root node : {}, file : {}", root_node, filename);
			return false;
		}
		return true;
	}

	const std::string& ParsedObject::get_id() const
	{
		return id;
	}

	std::string& normalize_form_id(std::string& form_id)
	{
		if (form_id.empty()) {
			form_id = "0x0";
			return form_id;
		}

		if (form_id.length() > 6) {
			form_id = form_id.substr(form_id.length() - 6);
		}

		form_id.erase(0, form_id.find_first_not_of('0'));

		if (form_id.empty()) {
			form_id = "0x0";
		} else {
			form_id = "0x" + form_id;
		}

		normalize_source(form_id);

		return form_id;
	}

	std::string& normalize_source(std::string& source)
	{
		std::transform(source.begin(), source.end(), source.begin(),
			[](unsigned char c) { return std::tolower(c); });
		return source;
	}

	const int ParsedObject::get_priority() const 
	{
		return loadPriority;
	}
}

extern bool HkxFileExists(const std::string& filename);
extern RE::TESForm* get_form_from_string(const std::string& xFormID, const std::string& plugin);
extern std::filesystem::path Where;

namespace fixes
{
	using namespace NAFicator;

	void for_every_type(obj_type t, std::function<void(std::pair<RE::BSSpinLock, std::shared_ptr<ParsedObject>>& obj)> apply, bool only_valid)
	{
		auto it = parsed_objects.begin();
		while (it != parsed_objects.end()) {
			auto& obj = it->second.second;
			if (obj && (t == kNone || obj->get_type() == t)) {
				if (!only_valid || obj->is_valid())
					apply(it->second);
			}
			++it;
		}
	}

	void for_every_type_mt(obj_type t, std::function<void(std::pair<RE::BSSpinLock, std::shared_ptr<ParsedObject>>& obj)> apply, bool only_valid)
	{
		concurrency::parallel_for_each(parsed_objects.begin(), parsed_objects.end(), [t, apply, only_valid](auto& it) {
			auto& parsed_object = it.second.second;
			if (parsed_object && (t == kNone || parsed_object->get_type() == t)) {
				if (!only_valid || [&parsed_object, &it]() { RE::BSAutoLock lock(it.second.first); return parsed_object->is_valid(); }())
				{
					apply(it.second);
				}
			}
		});
	}

	void replace_formid_to_hkx(std::pair<RE::BSSpinLock, std::shared_ptr<ParsedObject>> obj)
	{	
		auto& [l, ptr] = obj;
		auto DataHandler = RE::TESDataHandler::GetSingleton();
		Animation* o = static_cast<Animation*>(ptr.get());

		if (o->actors.empty()) {
			o->set_invalid("Animation/actors is empty.");
			return;
		}

		for (auto& a : o->actors)
		{
			if (!a.hkx)
			{
				auto& [form, src] = a.formId.value();
				if (DataHandler->LookupModByName(src)) {
					auto f = extract(std::make_shared<Form>(form, src));
					if (f) {
						auto idle = dynamic_pointer_cast<IdleForm>(f);
						if (idle)
						{
							auto hkx_path = utils::string::trim(idle->hkx(), " \t\r\n");
							//can be empty.
							a.hkx = hkx_path;
							continue;
						}
					}
				 
					if (RE::TESIdleForm* tesform = static_cast<RE::TESIdleForm*>(get_form_from_string(form, src)); 
						tesform && tesform->formType == RE::ENUM_FORM_ID::kIDLE)
					{
						auto hkx_path = std::string(tesform->animFileName.c_str());
						a.hkx = utils::string::trim(hkx_path, " \t\r\n");
						put(std::make_shared<IdleForm>(form, src, a.hkx.value()));
						continue;
					}
				}
			}
			if (!a.hkx) {
				std::string report{ "Failed to parse animation .hkx file. Probably wrong FormId/Data or animation .hkx file doesn't exists." };
				if (a.formId) {
					report += a.formId->first.empty() ? "" : (" " + a.formId->first);
					report += a.formId->second.empty() ? "" : (" " + a.formId->second);
				}
				o->set_invalid(report);
				return;
			}
		}
		/*if (!o->is_valid())
			remove_objects_linked_to_this_animation(o->get_id());*/
	}

	const auto types_to_print = { 
		kAnimation,
		kAnimationGroup,
		kPosition,
		kPositionTree,
		kAction,
		kEquipmentSet,
		kFurniture,
		kMfgSet,
		kMorphSet,
		kProtectedEquipment,
		kRace, 
		kOverlay/*,
		kTag*/
	};

	std::ostringstream& get_stream(obj_type t)
	{
		static std::ostringstream* animations = nullptr;
		static std::ostringstream* animation_groups = nullptr;
		static std::ostringstream* positions = nullptr;
		static std::ostringstream* positiontrees = nullptr;
		static std::ostringstream* actions = nullptr;
		static std::ostringstream* equipmentsets = nullptr;
		static std::ostringstream* furnitures = nullptr;
		static std::ostringstream* mfgsets = nullptr;
		static std::ostringstream* morphsets = nullptr;
		static std::ostringstream* protectedequipments = nullptr;
		static std::ostringstream* races = nullptr;
		static std::ostringstream* overlays = nullptr;
		//
		static std::ostringstream* tags = nullptr;
		static std::ostringstream none;

		auto priority = inimap.get<int>("iNaficatorFilesPriority"s, "General"s);

		switch (t) {
		case kAnimation:
			if (!animations) {
				animations = new std::ostringstream();
				*animations << "<animationData info=\"Panda Naficator v 0.9\" dataSet=\"animation\">\n\t<defaults loadPriority=\"" << priority << "\"/>\n";
			}
			return *animations;

		case kAnimationGroup:
			if (!animation_groups) {
				animation_groups = new std::ostringstream();
				*animation_groups << "<animationGroupData info=\"Panda Naficator v 0.9\" dataSet=\"animationGroup\">\n\t<defaults loadPriority=\"" << priority << "\"/>\n";
			}
			return *animation_groups;
		case kPosition:
			if (!positions) {
				positions = new std::ostringstream();
				*positions << "<positionData info=\"Panda Naficator v 0.9\" dataSet=\"position\">\n\t<defaults loadPriority=\"" << priority << "\"/>\n";
			}
			return *positions;
		case kPositionTree:
			if (!positiontrees) {
				positiontrees = new std::ostringstream();
				*positiontrees << "<positionTreeData info=\"Panda Naficator v 0.9\" dataSet=\"positionTree\">\n\t<defaults loadPriority=\"" << priority << "\"/>\n";
			}
			return *positiontrees;
		case kAction:
			if (!actions) {
				actions = new std::ostringstream();
				*actions << "<actionData info=\"Panda Naficator v 0.9\" dataSet=\"action\">\n\t<defaults loadPriority=\"" << priority << "\"/>\n";
			}
			return *actions;
		case kEquipmentSet:
			if (!equipmentsets) {
				equipmentsets = new std::ostringstream();
				*equipmentsets << "<equipmentSetData info=\"Panda Naficator v 0.9\" dataSet=\"equipmentSet\">\n\t<defaults loadPriority=\"" << priority << "\"/>\n";
			}
			return *equipmentsets;
		case kFurniture:
			if (!furnitures) {
				furnitures = new std::ostringstream();
				*furnitures << "<furnitureData info=\"Panda Naficator v 0.9\" dataSet=\"furniture\">\n\t<defaults loadPriority=\"" << priority << "\"/>\n";
			}
			return *furnitures;
		case kMfgSet:
			if (!mfgsets) {
				mfgsets = new std::ostringstream();
				*mfgsets << "<mfgSetData info=\"Panda Naficator v 0.9\" dataSet=\"mfgSet\">\n\t<defaults loadPriority=\"" << priority << "\"/>\n";
			}
			return *mfgsets;
		case kMorphSet:
			if (!morphsets) {
				morphsets = new std::ostringstream();
				*morphsets << "<morphSetData info=\"Panda Naficator v 0.9\" dataSet=\"morphSet\">\n\t<defaults loadPriority=\"" << priority << "\"/>\n";
			}
			return *morphsets;
		case kProtectedEquipment:
			if (!protectedequipments) {
				protectedequipments = new std::ostringstream();
				*protectedequipments << "<protectedEquipmentData info=\"Panda Naficator v 0.9\" dataSet=\"protectedEquipment\">\n\t<defaults loadPriority=\"" << priority << "\"/>\n";
			}
			return *protectedequipments;
		case kRace:
			if (!races) {
				races = new std::ostringstream();
				*races << "<raceData info=\"Panda Naficator v 0.9\" dataSet=\"race\">\n\t<defaults loadPriority=\"" << priority << "\"/>\n";
			}
			return *races;
		case kOverlay:
			if (!overlays) {
				overlays = new std::ostringstream();
				*overlays << "<overlayData info=\"Panda Naficator v 0.9\" dataSet=\"overlay\">\n\t<defaults loadPriority=\"" << priority << "\"/>\n";
			}
			return *overlays;
		/*case kTag:
				if (!tags) {
					tags = new std::ostringstream();
					*tags << "<tagData info=\"Panda Naficator v 0.9\" dataSet=\"tag\">\n\t<defauls loadPriority=\"" << priority << "\"/>\n";
				}
				return *tags;*/
		default:
			return none;  //should never happen;
		}
	}

	auto skip(obj_type t) 
	{
		return std::find_if(types_to_print.begin(), types_to_print.end(), [t](obj_type type) {
			return t == type;
		}) == types_to_print.end();
	};

	void save_to_file(obj_type t)
	{
		auto save = [&t](const std::string& filename, const std::ostringstream& oss) {
			try {
				std::ofstream outFile(Where / filename);
				if (!outFile) {
					throw std::runtime_error("Failed to open file: " + (Where / filename).string());
				}
				outFile << oss.str();
				outFile.close();
			} catch (const std::exception& e) {
				logger::error("❌ save_to_file: {}", e.what());
			}
		};

		switch (t) {
		case kAnimation:
			get_stream(t) << "\n</animationData>";
			save("Panda's_Naficator_animationData.xml"s, get_stream(t));
			break;
		case kAnimationGroup:
			get_stream(t) << "\n</animationGroupData>";
			save("Panda's_Naficator_animationGroupData.xml"s, get_stream(t));
			break;
		case kPosition:
			get_stream(t) << "\n</positionData>";
			save("Panda's_Naficator_positionData.xml"s, get_stream(t));
			break;
		case kPositionTree:
			get_stream(t) << "\n</positionTreeData>";
			save("Panda's_Naficator_positionTreeData.xml"s, get_stream(t));
			break;
		case kAction:
			get_stream(t) << "\n</actionData>";
			save("Panda's_Naficator_actionData.xml"s, get_stream(t));
			break;
		case kEquipmentSet:
			get_stream(t) << "\n</equipmentSetData>";
			save("Panda's_Naficator_equipmentSetData.xml"s, get_stream(t));
			break;
		case kFurniture:
			get_stream(t) << "\n</furnitureData>";
			save("Panda's_Naficator_furnitureData.xml"s, get_stream(t));
			break;
		case kMfgSet:
			get_stream(t) << "\n</mfgSetData>";
			save("Panda's_Naficator_mfgSetData.xml"s, get_stream(t));
			break;
		case kMorphSet:
			get_stream(t) << "\n</morphSetData>";
			save("Panda's_Naficator_morphSetData.xml"s, get_stream(t));
			break;
		case kProtectedEquipment:
			get_stream(t) << "\n</protectedEquipmentData>";
			save("Panda's_Naficator_protectedEquipment.xml"s, get_stream(t));
			break;
		case kRace:
			get_stream(t) << "\n</raceData>";
			save("Panda's_Naficator_raceData.xml"s, get_stream(t));
			break;
		case kOverlay:
			get_stream(t) << "\n</overlayData>";
			save("Panda's_Naficator_overlayData.xml"s, get_stream(t));
			break;
		/*case kTag:
			get_stream(t) << "\n</tagData>";
			save("Panda's_Naficator_tagData.xml"s, get_stream(t));
			break;*/
		}
	}

	void print_all()
	{
		logger::info("⚙️ Formatting parsed objects to XML...");
		for_every_type(kNone, [&](std::pair<RE::BSSpinLock, std::shared_ptr<ParsedObject>>& o) {
				auto& [l, ptr] = o;
				if (skip(ptr->get_type())) {
					return;
				}
				ptr->ssprint(get_stream(ptr->get_type()));
			},
			false);

		ProtectedEquipment pe;
		pe(get_stream(kProtectedEquipment));

		for (const auto& t : types_to_print) {
			save_to_file(t);
		}
		logger::info("✅ Formatting parsed objects to XML... done");
	}

	void hkxficate()
	{
		logger::info("⚙️ HKXficating...");
		for_every_type(kAnimation, [](std::pair<RE::BSSpinLock, std::shared_ptr<ParsedObject>> obj) {
			replace_formid_to_hkx(obj);
		});
		logger::info("✅ HKXficating... done");
	}

	void remove_objects_linked_to_this_animation(const std::string& animation_id)
	{
		for_every_type(kAnimationGroup, [&animation_id](std::pair<RE::BSSpinLock, std::shared_ptr<ParsedObject>> pair) {
			auto& [l, ptr] = pair;
			AnimationGroup* obj = static_cast<AnimationGroup*>(ptr.get());
			if (obj->find_animation(animation_id) != obj->stages.end()) {
				ptr->set_invalid("Invalid animation : " + animation_id);
			}
		});

		for_every_type(kPosition, [&animation_id](std::pair<RE::BSSpinLock, std::shared_ptr<ParsedObject>> o) {
			auto& [l, ptr] = o;
			Position* obj = static_cast<Position*>(ptr.get());
			if (static_cast<NAFicator::obj_type>(obj->linked_animation_type) == kAnimation && obj->linked_animation == animation_id) {
				{
					obj->set_invalid("Invalid animation : " + animation_id);
				}
				auto position = obj->get_id();
				for_every_type(kPositionTree, [&position](std::pair<RE::BSSpinLock, std::shared_ptr<ParsedObject>> o) {
					auto& [l_tree, ptr_tree] = o;
					PositionTree* obj = static_cast<PositionTree*>(ptr_tree.get());
					if (obj->find_position(position)) {
						obj->set_invalid("Invalid position : " + position);
					}
				});
				for_every_type(kTag, [&position](std::pair<RE::BSSpinLock, std::shared_ptr<ParsedObject>> o) {
					auto& [l_tag, ptr_tag] = o;
					Tag* obj = static_cast<Tag*>(ptr_tag.get());
					if (obj->get_id() == position) {
						obj->set_invalid("Invalid position : " + position);
					}
				});
			}
		});
	}

	CheckFlags operator | (CheckFlags lhs, CheckFlags rhs)
	{
		return static_cast<CheckFlags>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
	}

	bool operator&(CheckFlags lhs, CheckFlags rhs)
	{
		return (static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs)) != 0;
	}

	void remove_positions_with_missed_links(CheckFlags flags)
	{
		for_every_type(kPosition, [&](std::pair<RE::BSSpinLock, std::shared_ptr<ParsedObject>> obj) {
			auto& [l, ptr] = obj;
			Position* o = static_cast<Position*>(ptr.get());

			bool checkAnimation = (flags & CheckFlags::Animation) != 0;
			bool checkGroup = (flags & CheckFlags::Group) != 0;
			bool checkTree = (flags & CheckFlags::Tree) != 0;

			if (o->linked_animation_type == Position::type::animation && checkAnimation) {
				auto f = parsed_objects.find(std::pair(kAnimation, o->linked_animation));
				if (f == parsed_objects.end() || !f->second.second->is_valid()) {
					o->set_invalid("Link to missed animation " + o->linked_animation);
				}
			} else if (o->linked_animation_type == Position::type::group && checkGroup) {
				auto f = parsed_objects.find(std::pair(kAnimationGroup, o->linked_animation));
				if (f == parsed_objects.end() || !f->second.second->is_valid()) {
					o->set_invalid("Link to missed animation group " + o->linked_animation);
				}
			} else if (o->linked_animation_type == Position::type::tree && checkTree) {
				auto f = parsed_objects.find(std::pair(kPositionTree, o->linked_animation));
				if (f == parsed_objects.end() || !f->second.second->is_valid()) {
					o->set_invalid("Link to missed animation tree " + o->linked_animation);
				}
			}
		});
	}

	void remove_animation_groups_with_missed_animation_links()
	{
		for_every_type(kAnimationGroup, [](std::pair<RE::BSSpinLock, std::shared_ptr<ParsedObject>> obj) {
			auto& [l, ptr] = obj;
			AnimationGroup* o = static_cast<AnimationGroup*>(ptr.get());
			for (const auto& s : o->stages) {
				auto f = parsed_objects.find(std::pair(kAnimation, s.animation));
				if (f == parsed_objects.end() || !f->second.second->is_valid()) {
					o->set_invalid("Link to missed animation " + s.animation);
					break;
				}
			}
		});
	}

	void remove_positions_with_missed_animation_groups_links()
	{
		for_every_type(kPosition, [](std::pair<RE::BSSpinLock, std::shared_ptr<ParsedObject>> obj) {
			auto& [l, ptr] = obj;
			Position* o = static_cast<Position*>(ptr.get());
			if (o->linked_animation_type == Position::type::group) {
				auto f = parsed_objects.find(std::pair(kAnimationGroup, o->linked_animation));
				if (f == parsed_objects.end() || !f->second.second->is_valid()) {
					o->set_invalid("Link to missed animation group " + o->linked_animation);
				}
			}
		});
	}

	void remove_positions_with_missed_position_trees_links()
	{
		for_every_type(kPosition, [](std::pair<RE::BSSpinLock, std::shared_ptr<ParsedObject>> obj) {
			auto& [l, ptr] = obj;
			Position* o = static_cast<Position*>(ptr.get());
			if (o->linked_animation_type == Position::type::tree) {
				auto f = parsed_objects.find(std::pair(kPositionTree, o->linked_animation));
				if (f == parsed_objects.end() || !f->second.second->is_valid()) {
					o->set_invalid("Link to missed position tree " + o->linked_animation);
				}
			}
		});
	}

	void remove_position_trees_with_missed_animation_links()
	{		
		for_every_type(kPositionTree, [](std::pair<RE::BSSpinLock, std::shared_ptr<ParsedObject>> obj) {
			auto& [l, ptr] = obj;
			PositionTree* o = static_cast<PositionTree*>(ptr.get());
			o->traverse_tree([&l, &o](const std::shared_ptr<PositionTree::Node>& n) {
				if ([&l, &o]() { return o->is_valid(); }()) {
					auto f = parsed_objects.find(std::pair(kPosition, n->position));
					if (f == parsed_objects.end() || !f->second.second->is_valid()) {
						o->set_invalid("Link to missed position " + n->position);
					}
				}
			},
				o->tree);
		});
	}

	void remove_missed_animations()
	{
		logger::info("⚙️ Removing missed animations...");
		remove_positions_with_missed_links(CheckFlags::Animation);
		remove_animation_groups_with_missed_animation_links();
		remove_position_trees_with_missed_animation_links();
		remove_positions_with_missed_animation_groups_links();
		remove_positions_with_missed_position_trees_links();
		remove_positions_with_missed_links(CheckFlags::Group | CheckFlags::Tree);
		remove_position_trees_with_missed_animation_links();
		remove_positions_with_missed_links(CheckFlags::Tree);
		logger::info("✅ Removing missed animations... done");
	}

	void check_and_clear_furniture_bad_links() 
	{
		logger::info("⚙️ Checking furniture links...");
		for_every_type(kFurniture, [](std::pair<RE::BSSpinLock, std::shared_ptr<ParsedObject>>& obj) {
			auto f = static_cast<Furniture*>(obj.second.get());
			f->check_links_to_forms_and_clear_badlinks();
		});
		logger::info("✅ Checking furniture links... done");
	}

	void update_tags()
	{
		auto update_tags_based_on_locations = [](Position* pos) {
			if (pos->locations.empty()) {
				pos->tags.insert("nofurn");  
				return;
			}

			if (pos->locations.count("Single_Bed")) {
				pos->tags.insert("singlebed");
			}
			if (pos->locations.count("Double_Bed")) {
				pos->tags.insert("doublebed");
			}
			if (pos->locations.count("Mattress")) {
				pos->tags.insert("mattress");
			}

			pos->tags.erase("nofurn");
		};

		logger::info("⚙️ Updating position tags...");
		for_every_type(kPosition, [update_tags_based_on_locations](std::pair<RE::BSSpinLock, std::shared_ptr<ParsedObject>>& obj) {
			auto& [l, ptr] = obj;
			auto pos = static_cast<Position*>(ptr.get());

			auto it_tag = parsed_objects.find(std::pair(kTag, ptr->get_id()));
			if (it_tag != parsed_objects.end()) {
				auto tag = static_cast<Tag*>(it_tag->second.second.get());
				if (tag->replace) {
					pos->tags = tag->tags; 
				} else {
					pos->tags.insert(tag->tags.begin(), tag->tags.end());  
				}
			}

			update_tags_based_on_locations(pos);
		});
		logger::info("✅ Updating position tags... done");
	}

	void merge_positions_optionals()
	{
		auto merge_actor_offset = [](Animation::Actor& actor, const std::array<float, 4>& new_offset) {
			if (!actor.offset) {
				actor.offset = new_offset;  // Устанавливаем новое смещение, если его нет
			} else {
				for (size_t c = 0; c < actor.offset->size(); ++c) {
					(*actor.offset)[c] += new_offset[c];  // Обновляем существующее смещение
				}
			}
		};
		
		for_every_type(kPosition, [merge_actor_offset](std::pair<RE::BSSpinLock, std::shared_ptr<ParsedObject>>& obj) {
			auto& [l, ptr] = obj;
			auto pos = static_cast<Position*>(ptr.get());

			auto animation_key = std::pair(kAnimation, pos->get_id());
			auto it = parsed_objects.find(animation_key);

			if (it != parsed_objects.end() && it->second.second->is_valid()) {
				auto anim = static_cast<Animation*>(it->second.second.get());
				auto& actors = anim->actors;
				
				if (pos->offset) {
					const auto& offset = pos->offset.value();

					for (size_t count = 0; count < actors.size(); ++count) {
						auto& actor = actors[count];

						if (count < offset.size()) {
							merge_actor_offset(actor, offset[count]);
						} else {
							merge_actor_offset(actor, offset.back());
						}
					}
				}
			}
		});
	}
}
