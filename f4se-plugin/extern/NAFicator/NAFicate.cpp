#pragma once
#include <NAFicator/NAFicate.h>
extern ini::map inimap;
extern std::atomic<bool> voodoo_ready;
extern void MessageHandler(F4SE::MessagingInterface::Message* a_msg);
extern void outputCachedMessage();

void AfterGameDataReady()
{
	static std::atomic<bool> already_run{ false };
	// Ensure this runs only once
	if (already_run.exchange(true)) {
		logger::info("AfterGameDataReady already executed, skipping.");
		return;
	}
	
	logger::info("🧰 Start processing data...");
	fixes::hkxficate();
	fixes::remove_missed_animations();
	fixes::check_and_clear_furniture_bad_links();
	fixes::update_tags();
	fixes::print_all();
	outputCachedMessage();
}

bool HkxFileExists(const std::string& filename)
{
	auto LogAndReturn = [](const std::string& filename, RE::BSResource::ErrorCode code) {
		switch (code) {
		case RE::BSResource::ErrorCode::kNone:
			return true;
		case RE::BSResource::ErrorCode::kNotExist:
			logger::info("'FileExists' false : {} - kNotExist", filename);
			return false;
		case RE::BSResource::ErrorCode::kInvalidPath:
			logger::info("'FileExists' false : {} - kInvalidPath", filename);
			return false;
		default:
			logger::info("'FileExists' true (archived) : {} - Error code: {}", filename, static_cast<int>(code));
			return true;
		}
	};
	
	std::filesystem::path filePath = std::filesystem::current_path() / "Data" / "meshes" / filename;

	// Проверка существования файла в файловой системе
	if (std::filesystem::exists(filePath)) {
		//logger::info("'FileExists' true (loose) : {}", filename);
		return true;
	}

	// Проверка наличия файла в архиве
	RE::BSTSmartPointer<RE::BSResource::Stream, RE::BSTSmartPointerIntrusiveRefCount> a_result = nullptr;
	auto files = RE::BSResource::GetOrCreateStream(("meshes/" + filename).c_str(), a_result);

	if (a_result) {
		a_result->DoClose();
	}

	return LogAndReturn(filename, files);
}

bool process_file(const std::string& p_src, std::vector<XMLfile>& xmls, std::vector<std::string>& skipped)
{
	logger::info("📄 Processing file: {}", p_src);

	try {
		XMLfile xml_file(static_cast<const std::filesystem::path>(p_src));
		if (xml_file.has_value()) {
			logger::info("✅ File processed successfully: {}", p_src);
			xmls.push_back(xml_file);
			return true;
		} else {
			logger::warn("❌ File has no value: {}", p_src);
			skipped.push_back(p_src + " : has no value");
		}
	} catch (const std::exception& e) {
		logger::error("💥 Exception processing {}: {}", p_src, e.what());
	} catch (...) {
		logger::error("💥 Unknown error processing {}", p_src);
	}
	return false;
}

// Функция для обработки файлов
void parse_files(const std::filesystem::path& from, std::vector<XMLfile>& xmls, std::vector<std::string>& skipped)
{
	xmls.clear();
	skipped.clear();

	// Обработка race data ДО основного цикла
	auto RaceDataFile = std::filesystem::current_path() / "Data" / "NAFicator" / "NAF_raceData_Fallout4.xml";
	if (std::filesystem::exists(RaceDataFile) && std::filesystem::file_size(RaceDataFile) >= 10) {
		process_file(RaceDataFile.string(), xmls, skipped);
	} else {
		logger::error("ERROR: missed or empty {}", RaceDataFile.string());
	}

	// Основной цикл
	for (const auto& entry : std::filesystem::directory_iterator(from)) {
		try {
			if (entry.file_size() < 10) {
				skipped.emplace_back(entry.path().string() + " : < 10 bytes");
				continue;
			}
			if (entry.path().extension() != ".xml") {
				skipped.emplace_back(entry.path().string() + " : not .xml");
				continue;
			}
			process_file(entry.path().string(), xmls, skipped);
		} catch (const std::exception& e) {
			logger::error("Error processing {}: {}", entry.path().string(), e.what());
		}
	}
}

RE::TESForm* get_form_from_string(const std::string& xFormID, const std::string& plugin)
{
	if (xFormID.empty() || plugin.empty()) {
		return nullptr; 
	}
	std::string trimmedID = xFormID.length() > 6 ? xFormID.substr(xFormID.length() - 6) : xFormID;
	trimmedID.erase(trimmedID.begin(), std::find_if(trimmedID.begin(), trimmedID.end(), [](char c) { return c != '0'; }));

	if (!trimmedID.empty() && trimmedID.front() == 'x') {
		trimmedID.erase(trimmedID.begin());
	}

	if (trimmedID.empty()) {
		return nullptr;
	}

	trimmedID = "0x" + trimmedID;

	try {
		uint32_t FormID_uint = std::stoul(trimmedID, nullptr, 16);
		return RE::TESDataHandler::GetSingleton()->LookupForm<RE::TESForm>(FormID_uint, plugin);
	} catch (const std::invalid_argument&) {
		return nullptr;
	} catch (const std::out_of_range&) {
		return nullptr;
	} catch (...) {
		return nullptr;
	}
}

RE::TESForm* get_form_by_editor_id(std::string editor_id)
{
	RE::TESForm* obj;
	obj = obj->GetFormByEditorID(editor_id);
	return obj;
}


void START(const std::filesystem::path& from)
{
	std::vector<XMLfile> store;
	std::vector<std::string> skipped;	

	parse_files(from, store, skipped);

	/*for_each_file(store, [&](XMLfile& x) {
		logger::info("{} - {}", x.filename(), x.get_root());	
	},"");*/

	logger::info("Start parsing data...");

	std::vector<std::shared_ptr<std::stringstream>> xfiles;
	xfiles.reserve(store.size());

	for_each_file(
		store, [&](XMLfile& x) {
			xfiles.emplace_back(x.make_stringstream());		
	},
	"");

	fixes::merge_positions_optionals();

	NAFicator::parse_XML_files(xfiles);

	if (voodoo_ready.exchange(true) == false) {
		return;
	}

	AfterGameDataReady();
	logger::info("🧐 NAFicator finished!");
}

void for_each_file(std::vector<XMLfile>& xmls, std::function<void(XMLfile&)> apply, const std::string& rootNode)
{
	std::vector<std::future<void>> futures;  // Вектор для хранения будущих задач
	for (auto& x : xmls) {
		if (x.has_value() && (rootNode.empty() || (rootNode == x.get_root()))) {
				apply(x);
		}
	}
}
