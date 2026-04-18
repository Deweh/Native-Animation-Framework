#pragma once
#include "NAF_Utils.h"
#include "F4SE/F4SE.h"
#include "RE/Fallout.h"

#include "Bridge/Consts.h"
#include "Bridge/IniParser/Ini.h"
#include "Bridge/Papyrus/Papyrus.h"
#include <random>

#include "Misc/Utility.h"

#include <ppl.h>

namespace ppl = concurrency;

namespace logger = F4SE::log;

#define PAPYRUS_BIND(funcName) a_VM->BindNativeMethod("NAF_Utils", #funcName, funcName, true)
#define PAPYRUS_BIND_LATENT(funcName, retType) a_VM->BindNativeMethod<retType>("NAF_Utils", #funcName, funcName, true, true)

extern int PRINT_LOG;

namespace Papyrus
{
	/*
	; Десериализует строку в массив форм
	; @param sPackedString - строка с FormID, разделенными запятыми
	; @return массив форм или nullptr
	*/
	static std::vector<RE::TESForm*> DeserializeFormsFromString(const std::string& sPackedString)
	{
		std::vector<RE::TESForm*> result;
		if (sPackedString.empty()) {
			return result;
		}

		size_t start = 0;
		size_t end = sPackedString.find(',');
		while (end != std::string::npos) {
			std::string token = sPackedString.substr(start, end - start);
			std::uint32_t formID = static_cast<std::uint32_t>(std::stoul(token));
			RE::TESForm* form = RE::TESForm::GetFormByID(formID);
			if (form != nullptr) {
				result.emplace_back(form);
			}
			start = end + 1;
			end = sPackedString.find(',', start);
		}

		std::string token = sPackedString.substr(start);
		if (!token.empty()) {
			std::uint32_t formID = static_cast<std::uint32_t>(std::stoul(token));
			RE::TESForm* form = RE::TESForm::GetFormByID(formID);
			if (form != nullptr) {
				result.emplace_back(form);
			}
		}
		return result;
	}

	static std::string trim(const std::string& s)
	{
		std::size_t a = 0;
		while (a < s.size() && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
		std::size_t b = s.size();
		while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) --b;
		return s.substr(a, b - a);
	}

	static std::string to_lower_copy(const std::string& s)
	{
		std::string r = s;
		for (std::size_t i = 0; i < r.size(); ++i) {
			r[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(r[i])));
		}
		return r;
	}

	// Сравнение подстроки s[start..start+len-1] с t без учёта регистра (ASCII), без выделения дополнительной строки
	static bool ci_equal_substr(const std::string& s, std::size_t start, std::size_t len, const std::string& t)
	{
		if (len != t.size())
			return false;
		for (std::size_t i = 0; i < len; ++i) {
			unsigned char a = static_cast<unsigned char>(s[start + i]);
			unsigned char b = static_cast<unsigned char>(t[i]);
			if (std::tolower(a) != std::tolower(b))
				return false;
		}
		return true;
	}

	bool RegisterNAFUtilsFunctions(RE::BSScript::IVirtualMachine* a_VM)
	{
		PAPYRUS_BIND(RemoveAllChar);
		PAPYRUS_BIND(CacheActorsToString);
		PAPYRUS_BIND(GetActorsFromString);
		PAPYRUS_BIND(CacheFormlistToString);
		PAPYRUS_BIND(FillFormlistFromString);
		PAPYRUS_BIND(GetEquipmentFromString);
		PAPYRUS_BIND(SetINISettingToFile);
		PAPYRUS_BIND(JoinStringArray);
		PAPYRUS_BIND(AddKeywordToActors);
		PAPYRUS_BIND(RemoveKeywordFromActors);
		PAPYRUS_BIND(NormalizeTag);
		PAPYRUS_BIND(ContainsPlayer);
		PAPYRUS_BIND(FilterActorsByBlockedKeywords);
		PAPYRUS_BIND(SwapActorsAtIndices);
		PAPYRUS_BIND(RotateFirstFemaleToNullIndexAtArray);
		PAPYRUS_BIND(IsFemale);
		PAPYRUS_BIND(AddTag);
		PAPYRUS_BIND(RemoveTag);
		PAPYRUS_BIND(ContainsTag);
		PAPYRUS_BIND(GetActorsInRange);

		logger::info("Registered NAF_Utils functions");

		return true;
	}

	// Удаляет все вхождения указанного символа из строки;
	// @param sStr - исходная строка;
	// @param sChar - символ для удаления;
	// @ return строка без указанных символов
	std::string RemoveAllChar(std::monostate, std::string sStr, std::string sChar)
	{
		if (sStr.empty() || sChar.empty())
			return sStr;
		sStr.erase(std::remove(sStr.begin(), sStr.end(), sChar[0]), sStr.end());
		return sStr;
	}

	// Сериализует массив акторов в строку с FormID, разделенными запятыми
	// @param akActors - массив акторов для сериализации
	// @return строка с FormID акторов, разделенными запятыми
	std::string CacheActorsToString(std::monostate, std::vector<RE::Actor*> akActors)
	{
		std::string result;
		const size_t actorCount = akActors.size();
		for (size_t i = 0; i < actorCount; ++i) {
			if (akActors[i] != nullptr) {
				result += std::to_string(akActors[i]->formID);
				if (i < actorCount - 1) {
					result += ",";
				}
			}
		}
		return result;
	}

	// Десериализует строку с FormID в массив акторов
	// @param sPackedActors - строка с FormID, разделенными запятыми
	// @return массив акторов
	std::vector<RE::Actor*> GetActorsFromString(std::monostate, std::string sPackedActors)
	{
		std::vector<RE::Actor*> result;
		if (sPackedActors.empty()) {
			return result;
		}
		auto arr = DeserializeFormsFromString(sPackedActors);
		for (RE::TESForm* form : arr) {
			RE::Actor* actor = form->As<RE::Actor>();
			if (actor != nullptr) {
				result.emplace_back(actor);
			}
		}
		return result;
	}

	// Сериализует массив форм в строку с FormID, разделенными запятыми
	// @param akForms - массив форм для сериализации
	// @return строка с FormID форм, разделенными запятыми
	std::string CacheFormlistToString(std::monostate, RE::BGSListForm* akFormList)
	{
		if (!akFormList) {
			return "";
		}
		if (akFormList->arrayOfForms.empty() && akFormList->scriptAddedFormCount == 0) {
			return "";
		}

		std::vector<int> vectorResult;
		vectorResult.reserve(akFormList->arrayOfForms.size() + akFormList->scriptAddedFormCount);
		for (const auto& form : akFormList->arrayOfForms) {
			if (form != nullptr) {
				vectorResult.emplace_back(form->formID);
			}
		}

		auto hasForm = [&vectorResult](RE::TESForm* form) {
			auto it = std::find(vectorResult.begin(),
				vectorResult.end(),
				form->formID);
			return it != vectorResult.end();
		};

		for (std::uint32_t i = 0; i < akFormList->scriptAddedFormCount; ++i) {
			RE::TESForm* form = RE::TESForm::GetFormByID(akFormList->scriptAddedTempForms->at(i));
			if (form != nullptr && !hasForm(form)) {
				vectorResult.emplace_back(form->formID);
			}
		}

		std::string result;
		size_t formCount = vectorResult.size();
		for (size_t i = 0; i < formCount; ++i) {
			result += std::to_string(vectorResult[i]);
			if (i < formCount - 1) {
				result += ",";
			}
		}
		return result;
	}

	// Заполняет FormList из строки с FormID. Очищает только скриптовые временные формы.
	// @param akFormList - FormList для заполнения;
	// @param sPackedObjects - строка с FormID, разделенными запятыми
	void FillFormlistFromString(std::monostate, RE::BGSListForm* akFormList, std::string sPackedObjects)
	{
		if (!akFormList || sPackedObjects.empty()) {
			return;
		}

		if (!akFormList->scriptAddedTempForms) {
			logger::warn("NAF_Utils::FillFormlistFromString: scriptAddedTempForms is null, creating new array");
			return;
		}

		auto& array = *akFormList->scriptAddedTempForms;
		auto& formCount = akFormList->scriptAddedFormCount;
		formCount = 0;
		array.clear();

		size_t start = 0;
		size_t end = sPackedObjects.find(',');
		while (end != std::string::npos) {
			std::string token = sPackedObjects.substr(start, end - start);
			std::uint32_t formID = static_cast<std::uint32_t>(std::stoul(token));
			RE::TESForm* form = RE::TESForm::GetFormByID(formID);
			if (form != nullptr) {
				array.emplace_back(formID);
				formCount += 1;
			}
			start = end + 1;
			end = sPackedObjects.find(',', start);
		}
		// Последний токен
		std::string token = sPackedObjects.substr(start);
		if (!token.empty()) {
			std::uint32_t formID = static_cast<std::uint32_t>(std::stoul(token));
			RE::TESForm* form = RE::TESForm::GetFormByID(formID);
			if (form != nullptr) {
				array.emplace_back(formID);
				formCount += 1;
			}
		}
	}

	// Десериализует строку в массив экипировки(только Armor, Weapon, Potion);
	// @param sPackedString - строка с FormID, разделенными запятыми;
	// @ return массив форм экипировки или None
	std::vector<RE::TESForm*> GetEquipmentFromString(std::monostate, std::string sPackedString)
	{
		std::vector<RE::TESForm*> result;
		std::vector<RE::TESForm*> allForms = DeserializeFormsFromString(sPackedString);
		for (RE::TESForm* form : allForms) {
			if (form->As<RE::TESObjectARMO>() ||
				form->As<RE::TESObjectWEAP>() ||
				form->As<RE::AlchemyItem>()) {
				result.emplace_back(form);
			}
		}
		return result;
	}

	// Записывает значение в INI файл;
	// @param file - путь к INI файлу;
	// @param section - название секции;
	// @param setting - название параметра;
	// @param value - значение параметра;
	// @ return true в случае успеха
	bool SetINISettingToFile(std::monostate, std::string file, std::string section, std::string setting, std::string value)
	{
		auto s = ini::map();
		if (!s.readFile(file)) {
			return false;
		}
		s.set(section + "/" + setting, value);
		return true;
	}

	// Объединить массив строк в одну строку с разделителем
	std::string JoinStringArray(std::monostate, std::vector<std::string> arr, std::string delim)
	{
		std::string result;
		const size_t count = arr.size();
		for (size_t i = 0; i < count; ++i) {
			result += arr[i];
			if (i < count - 1) {
				result += delim;
			}
		}
		return result;
	}

	// Добавить указанный keyword ко всем актёрам в массиве
	void AddKeywordToActors(std::monostate, std::vector<RE::Actor*> akActors, RE::BGSKeyword* keyword)
	{
		if (!keyword) {
			return;
		}
		for (RE::Actor* actor : akActors) {
			if (actor) {
				actor->ModifyKeyword(keyword, true);
			}
		}
	}

	// Удалить указанный keyword у всех актёров в массиве
	void RemoveKeywordFromActors(std::monostate, std::vector<RE::Actor*> akActors, RE::BGSKeyword* keyword)
	{
		if (!keyword) {
			return;
		}
		for (RE::Actor* actor : akActors) {
			if (actor) {
				actor->ModifyKeyword(keyword, false);
			}
		}
	}

	static std::string_view trim_view(std::string_view sv)
	{
		auto is_space = [](char c) {
			unsigned char uc = static_cast<unsigned char>(c);
			return uc == ' ' || uc == '\t' || uc == '\n' || uc == '\r' || uc == '\f' || uc == '\v';
		};
		size_t b = 0;
		while (b < sv.size() && is_space(sv[b])) ++b;
		size_t e = sv.size();
		while (e > b && is_space(sv[e - 1])) --e;
		return sv.substr(b, e - b);
	}

	static std::string to_lower_ascii(std::string_view sv)
	{
		std::string out;
		out.reserve(sv.size());
		for (unsigned char c : sv) out.push_back(static_cast<char>(std::tolower(c)));
		return out;
	}

	// Нормализует строковое значение тега, заменяя "None" на пустую строку, если тег содержит дубликаты - оставляет первый;
	// @param sTag - строка тега для нормализации;
	// @ return нормализованная строка(пустая, если была "None")
	std::string NormalizeTag(std::monostate, std::string sTag)
	{
		std::string_view sv(sTag);
		// Проверка "none" (тримаем и сравниваем в нижнем регистре)
		if (to_lower_ascii(trim_view(sv)) == "none") {
			return {};
		}

		// Используем неизменяемую функцию SplitString (возвращает vector<string_view>)
		auto parts = Utility::SplitString(sv, std::string_view{ "," });

		std::unordered_set<std::string> seen;
		seen.reserve(parts.size() * 2);

		std::string result;
		result.reserve(sTag.size());

		bool firstAdded = false;
		for (auto part : parts) {
			std::string_view t = trim_view(part);
			if (t.empty())
				continue;
			std::string key = to_lower_ascii(t);
			if (seen.find(key) != seen.end())
				continue;
			seen.insert(std::move(key));
			if (firstAdded)
				result.push_back(',');
			result.append(t.begin(), t.end());  // добавляем оригинальный (триммированный) текст
			firstAdded = true;
		}

		return result;
	}

	// Проверяет, содержит ли массив акторов игрока;
	// @param akActors - массив акторов для проверки;
	// @ return true, если игрок присутствует в массиве
	bool ContainsPlayer(std::monostate, std::vector<RE::Actor*> akActors)
	{
		for (const RE::Actor* actor : akActors) {
			if (actor && actor->IsPlayerRef()) {
				return true;
			}
		}
		return false;
	}
	// Меняет местами акторов в массиве по указанным индексам;
	// @param akActors - массив акторов;
	// @param index1 - первый индекс;
	// @param index2 - второй индекс;
	// @ return массив с поменянными актерами
	std::vector<RE::Actor*> SwapActorsAtIndices(std::monostate, std::vector<RE::Actor*> akActors, int index1, int index2) {
		if (index1 < 0 || index2 < 0 || static_cast<size_t>(index1) >= akActors.size() || static_cast<size_t>(index2) >= akActors.size()) {
			return akActors;  // Индексы вне диапазона, возвращаем исходный массив
		}
		std::swap(akActors[index1], akActors[index2]);
		return akActors;
	}

	std::vector<RE::Actor*> RotateFirstFemaleToNullIndexAtArray(std::monostate, std::vector<RE::Actor*> akActors)
	{
		if (akActors.size() <= 1) {
			return akActors;
		}

		if (akActors[0] && akActors[0]->GetSex() == RE::Actor::Sex::Female) {
			return akActors;
		}

		const size_t n = akActors.size();
		size_t femaleIndex = n;  // use n as 'not found' sentinel

		for (size_t i = 1; i < n; ++i) {
			RE::Actor* actor = akActors[i];
			if (actor && actor->GetSex() == RE::Actor::Sex::Female) {
				femaleIndex = i;
				break;
			}
		}

		// If no female found, return original
		if (femaleIndex == n) {
			return akActors;
		}

		// Move the found female to index 0 while preserving the relative order of other elements
		// This is slightly more expensive than a swap but avoids unexpected reordering
		std::rotate(akActors.begin(), akActors.begin() + femaleIndex, akActors.begin() + femaleIndex + 1);

		return akActors;
	}

	// Фильтрует массив акторов, удаляя None и дубликаты;
	// @param akActors - исходный массив акторов;
	// @param kBlockedKeyword - keyword для проверки блокировки(опционально);
	// @param kBusyKeyword - keyword для проверки занятости(опционально);
	// @ return отфильтрованный массив акторов
	std::vector<RE::Actor*> FilterActorsByBlockedKeywords(std::monostate, std::vector<RE::Actor*> akActors, std::vector<RE::BGSKeyword*> blockedKeywords)
	{
		std::vector<RE::Actor*> result;
		if (akActors.empty()) {
			return result;
		}

		result.reserve(akActors.size());
		auto contains = [&result](RE::Actor* actor) {
			return std::find(result.begin(), result.end(), actor) != result.end();
		};

		auto hasKeyword = [&blockedKeywords](RE::Actor* actor) {
			for (RE::BGSKeyword* keyword : blockedKeywords)
				if (keyword && actor->HasKeyword(keyword)) return true;
			return false;
		};

		for (RE::Actor* actor : akActors) {
			if (actor == nullptr) {
				continue;  // Пропустить None
			}
			
			if (hasKeyword(actor))
				continue;

			if (contains(actor)) {
				continue;  // Пропустить дубликаты
			}
			result.emplace_back(actor);
		}
		return result;
	}

	// Проверяет пол актора;
	// @param akActor - актор для проверки;
	// @ return true, если актор женского пола
	bool IsFemale(std::monostate, RE::Actor* akActor)
	{
		if (!akActor) {
			return false;
		}
		return akActor->GetSex() == RE::Actor::Sex::Female;
	}

	// Добавляет тег к строке тегов, если его там еще нет;
	// @param sTags - строка с тегами, разделенными запятыми;
	// @param sNewTag - новый тег для добавления;
	// @ return обновленная строка тегов
	std::string AddTag(std::monostate, std::string sTags, std::string sNewTag)
	{
		if (sNewTag.empty()) {
			return sTags;  // Пустой тег не добавляется
		}
		if (sTags.empty()) {
			return sNewTag;  // Если строка тегов пуста, просто вернуть новый тег
		}
		const std::string& tags = sTags;
		const std::size_t n = tags.size();
		std::size_t pos = 0;
		while (pos <= n) {
			std::size_t comma = tags.find(',', pos);
			std::size_t end = (comma == std::string::npos) ? n : comma;
			std::size_t token_len = end - pos;
			if (ci_equal_substr(tags, pos, token_len, sNewTag)) {
				return sTags;  // тег уже присутствует (токен совпал полностью, не частично)
			}
			if (comma == std::string::npos)
				break;
			pos = comma + 1;
		}
		return sTags + "," + sNewTag;
	}

	// Удаляет тег из строки тегов;
	// @param sTags - строка с тегами, разделенными запятыми;
	// @param sTagToRemove - тег для удаления;
	// @param removeAll - удалить все вхождения(1) или только первое(0), или с конца(2);
	// @ return обновленная строка тегов
	std::string RemoveTag(std::monostate, std::string sTags, std::string sTagToRemove, int removeAll)
	{
		if (sTagToRemove.empty()) {
			return sTags;  // ничего удалять
		}
		if (sTags.empty()) {
			return sTags;
		}
		const std::string& tags = sTags;
		const std::size_t n = tags.size();
		std::vector<std::string> tokens;
		tokens.reserve(8);
		std::size_t pos = 0;
		while (pos <= n) {
			std::size_t comma = tags.find(',', pos);
			std::size_t end = (comma == std::string::npos) ? n : comma;
			tokens.push_back(tags.substr(pos, end - pos));
			if (comma == std::string::npos)
				break;
			pos = comma + 1;
		}
		const std::string target = to_lower_copy(trim(sTagToRemove));
		std::vector<int> match_indices;
		for (std::size_t i = 0; i < tokens.size(); ++i) {
			std::string ttrim = trim(tokens[i]);
			if (to_lower_copy(ttrim) == target) {
				match_indices.push_back(static_cast<int>(i));
			}
		}
		if (match_indices.empty()) {
			return sTags;  // ничего не найдено
		}
		std::vector<char> remove(tokens.size(), 0);  // 0 - не удалять, 1 - удалять
		if (removeAll == 1) {
			for (int idx : match_indices) remove[idx] = 1;
		} else if (removeAll == 0) {
			remove[match_indices.front()] = 1;
		} else if (removeAll == 2) {
			remove[match_indices.back()] = 1;
		} else {
			for (int idx : match_indices) remove[idx] = 1;
		}
		std::string out;
		bool first_added = false;
		for (std::size_t i = 0; i < tokens.size(); ++i) {
			if (remove[i])
				continue;
			if (first_added)
				out.push_back(',');
			out += tokens[i];
			first_added = true;
		}
		return out;
	}

	// Проверяет содержит ли строка тегов указанный тег
	// @param sTags - строка с тегами, разделенными запятыми
	// @param sTag - тег для поиска
	// @return true, если тег найден
	bool ContainsTag(std::monostate, std::string sTags, std::string sTag)
	{
		if (sTag.empty())
			return false;
		if (sTags.empty())
			return false;
		const std::string target = to_lower_copy(trim(sTag));
		const std::string& tags = sTags;
		const std::size_t n = tags.size();
		std::size_t pos = 0;
		while (pos <= n) {
			std::size_t comma = tags.find(',', pos);
			std::size_t end = (comma == std::string::npos) ? n : comma;
			std::string token = trim(tags.substr(pos, end - pos));
			if (to_lower_copy(token) == target) {
				return true;
			}
			if (comma == std::string::npos)
				break;
			pos = comma + 1;
		}
		return false;
	}

	/**
	 * @brief Получает список актёров в радиусе от указанного объекта.
	 * @param a_ref Ссылка на объект-центр поиска (по умолчанию игрок).
	 * @param a_maxDistance Максимальная дистанция поиска.
	 * @param a_maxActorsCount Максимальное количество актёров.
	 * @param a_includeDead Включать ли мёртвых актёров.
	 * @return Вектор указателей на найденных актёров.
	 */
	inline std::vector<RE::Actor*> getActorsInRangeImpl(RE::TESObjectREFR* a_ref,
		std::uint32_t a_maxDistance, int a_maxActorsCount,
		bool a_includeDead, std::function<bool(const RE::Actor*)> filter)
	{
		std::vector<RE::Actor*> result;

		if (a_ref == nullptr)
			return result;

		auto originPos{ a_ref->GetPosition() };

		const auto processLists = RE::ProcessLists::GetSingleton();

		concurrency::parallel_for_each(processLists->highActorHandles.begin(), processLists->highActorHandles.end(),
			[&](const RE::ActorHandle& actorHandle) {
				const auto actorPtr = actorHandle.get();
				const auto currentActor = actorPtr.get();
				if (!currentActor) {
					return;
				}
				if (!a_includeDead && currentActor->IsDead(true)) {
					return;
				}
				if (currentActor == a_ref) {
					return;
				}
				if (filter && filter(currentActor) == false) {
					return;
				}
				if (originPos.GetDistance(currentActor->GetPosition()) <= a_maxDistance) {
					// Блокируем доступ к вектору при добавлении элемента
					static std::mutex mtx;
					std::lock_guard<std::mutex> lock(mtx);
					result.push_back(currentActor);
				}
			});

		if (result.size() > a_maxActorsCount)
			result.resize(a_maxActorsCount);
		return result;
	}

	std::vector<RE::Actor*> GetActorsInRange(std::monostate, RE::TESObjectREFR* from, float distance, bool includeDead)
	{
		return getActorsInRangeImpl(from, distance, 0xFFFFFFFF, includeDead, nullptr);
	}

	/** 
	* @brief Преобразует целочисленное значение в строку в шестнадцатеричном формате.
	* @param value Целочисленное значение для преобразования.
	* @return Шестнадцатеричная строка, представляющая значение.
	*/
	std::string ToHexString(std::monostate, std::uint32_t value) {
		constexpr char hexChars[] = "0123456789ABCDEF";
		std::string hexString;
		hexString.reserve(8);
		for (int i = 7; i >= 0; --i) {
			hexString.push_back(hexChars[(value >> (i * 4)) & 0x0F]);
		}
		return hexString;
	}

	/**
	* @brief Преобразует шестнадцатеричную строку в целочисленное значение.
	* @param hexString Шестнадцатеричная строка для преобразования.
	* @return Целочисленное значение, соответствующее шестнадцатеричной строке.
	*/
	std::uint32_t FromHexString(std::monostate, std::string hexString) {
		// Trim whitespace
		hexString = trim(hexString);
		// Optional 0x/0X prefix
		if (hexString.size() >= 2 && hexString[0] == '0' && (hexString[1] == 'x' || hexString[1] == 'X')) {
			hexString = hexString.substr(2);
		}
		// Must be 1..8 hex digits
		if (hexString.empty() || hexString.size() > 8) {
			return 0;
		}
		std::uint32_t value = 0;
		for (unsigned char c : hexString) {
			value <<= 4;
			if (c >= '0' && c <= '9') {
				value |= static_cast<std::uint32_t>(c - '0');
			} else if (c >= 'A' && c <= 'F') {
				value |= static_cast<std::uint32_t>(c - 'A' + 10);
			} else if (c >= 'a' && c <= 'f') {
				value |= static_cast<std::uint32_t>(c - 'a' + 10);
			} else {
				// invalid character
				return 0;
			}
		}
		return value;
	}
}

#undef PAPYRUS_BIND
#undef PAPYRUS_BIND_LATENT
