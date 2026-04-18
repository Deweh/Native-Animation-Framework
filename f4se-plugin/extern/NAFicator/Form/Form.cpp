#pragma once
#include "Form.h"

#include "F4SE/F4SE.h"
#include "RE/Fallout.h"

#include <spdlog/sinks/basic_file_sink.h>

#define DLLEXPORT __declspec(dllexport)

namespace logger = F4SE::log;

using namespace std::literals;

uint32_t Form::get_uint() const
{	
	if (m_form.empty() || m_source.empty())
		return 0;

	uint32_t FormID_uint = 0;
	try {
		FormID_uint = stoul(m_form, nullptr, 16);
	} catch (std::invalid_argument e) {
		logger::critical("Wrong formId data {}", m_form);
		return 0;
	} catch (std::out_of_range e) {
		logger::critical("Wrong formId data {}", m_form);
		return 0;
	} catch (...) {
		logger::critical("Wrong formId data {}", m_form);
		return 0;
	}
	return FormID_uint;
}

void Form::normalize()
{
	if (m_form.empty() || m_source.empty())
		return;

	// Ограничиваем длину m_form до 6 символов
	if (m_form.length() > 6) {
		m_form = m_form.substr(m_form.length() - 6);
	}

	// Удаляем ведущие нули и 'x'
	while (!m_form.empty() && (m_form.front() == '0' || m_form.front() == 'x')) {
		m_form.erase(m_form.begin());
	}

	// Добавляем префикс '0x'
	m_form = "0x" + m_form;

	utils::remove_spaces_from_sides(m_source);
	utils::remove_spaces_from_sides(m_form);
}

std::string Form::make_archive_string() const
{
	if (m_form.empty() || m_source.empty())
		return ""s;

	return ("Form,"s + m_form + ","s + m_source);
}

RE::TESForm* Form::get() const
{
	if (m_form.empty() || m_source.empty())
		return nullptr;
	return RE::TESDataHandler::GetSingleton()->LookupForm<RE::TESForm>(get_uint(), m_source);
}

bool Form::operator==(const Form& f) const
{
	return (m_form == f.m_form && m_source == f.m_source);
}

void IdleForm::normalize()
{
	utils::remove_spaces_from_sides(m_hkx);
}

std::string IdleForm::make_archive_string() const
{
	if (m_form.empty() || m_source.empty())
		return ""s;
	return ("IdleForm,"s + m_form + ","s + m_source + ","s + m_hkx);
}

std::shared_ptr<Form> Archive::get_form(const std::string& str, const std::shared_ptr<Form> form) const
{
	std::vector<std::string> arr;
	utils::delim(str, ","s, arr);
	if (!arr.empty() && arr.size() > 1) {
		if (compare_formId_string(arr[1], form->m_form) && compare_source_string(arr[2], form->m_source)) {
			if (arr[0] == "Form") {
				return std::make_shared<Form>(Form(arr[1], arr[2]));
			} else if (arr[0] == "IdleForm") {
				return static_cast<std::shared_ptr<Form>>(std::make_shared<IdleForm>(arr[1], arr[2], arr[3]));
			}
		}
	}
	return std::shared_ptr<Form>(nullptr);
};

std::shared_ptr<Form> Archive::get_form(const std::string& str, const std::string& form, const std::string& source) const
{
	return get_form(str, std::make_shared<Form>(form, source));
};

std::shared_ptr<Form> Archive::find_and_apply(const std::string& form, const std::string& source, const std::function<std::shared_ptr<Form>(const std::string&)>& apply)
{
	return find_and_apply(std::make_shared<Form>(form, source), apply);
};

std::shared_ptr<Form> Archive::find_and_apply(std::shared_ptr<Form> form, const std::function<std::shared_ptr<Form>(const std::string&)>& apply)
{
	// Проверка входных данных
	if (!form || !form->has_value()) {
		return nullptr;
	}

	std::unique_lock l(lock);

	// Построение корректного пути: удаляем ведущий разделитель в конфигурном пути, если он есть
	std::filesystem::path pth = std::filesystem::current_path();
	std::string relPath = Archive::get_singleton()->path_to_archive;
	if (!relPath.empty() && (relPath.front() == '\\' || relPath.front() == '/')) {
		relPath.erase(0, 1);
	}
	pth /= relPath; // безопасное объединение
	pth.make_preferred();

	std::ifstream archiveFile(pth, std::ios::binary);
	if (!archiveFile.is_open()) {
		logger::error("'find_and_apply' couldn't open archive : {}", form->make_archive_string());
		return nullptr;
	}

	std::string line;
	while (std::getline(archiveFile, line)) { 
		if (line.empty())
			continue;
		std::vector<std::string> del;
		utils::delim(line, ","s, del);
		if (del.size() >= 3) {  
			if (compare_formId_string(del[1], form->m_form) && compare_source_string(del[2], form->m_source)) {
				archiveFile.close(); 
				return apply(line);
			}
		}
	}

	archiveFile.close();
	return nullptr;
}

std::shared_ptr<Form> extract(std::shared_ptr<Form> form)
{
	auto extract_form = [&](const std::string& str) {
		return Archive::get_singleton()->get_form(str, form);
	};

	std::shared_ptr<Form> res = std::shared_ptr<Form>(nullptr);
	res = Archive::get_singleton()->find_and_apply(form, extract_form);

	return res;
}

std::shared_ptr<Form> put(std::shared_ptr<Form> form)
{
	if (!form || !form->has_value())
		return form;

	// ✅ Блокировка ДО проверки существования
	std::unique_lock l(Archive::get_singleton()->lock);

	// Получаем путь к архиву. Удаляем ведущий разделитель, если указан в конфиге
	std::filesystem::path pth = std::filesystem::current_path();
	std::string relPath = Archive::get_singleton()->path_to_archive;
	if (!relPath.empty() && (relPath.front() == '\\' || relPath.front() == '/')) {
		relPath.erase(0, 1);
	}
	pth /= relPath;
	pth.make_preferred();

	// ✅ Проверяем существование записи ВНУТРИ блокировки
	bool exists = false;
	{
		std::ifstream checkFile(pth, std::ios::binary);
		if (checkFile.is_open()) {
			std::string line;
			while (std::getline(checkFile, line)) {
				if (line.empty())
					continue;
				std::vector<std::string> del;
				utils::delim(line, ","s, del);
				if (del.size() >= 3) {
					if (compare_formId_string(del[1], form->get_form_str()) && compare_source_string(del[2], form->get_source_str())) {
						exists = true;
						break;
					}
				}
			}
			checkFile.close();
		}
	}

	// Записываем только если записи нет
	if (!exists) {
		std::ofstream file(pth, std::ios::app | std::ios::binary);
		if (!file.is_open()) {
			logger::error("'put' couldn't open archive : {}", form->make_archive_string());
			return nullptr;
		}

		file << form->make_archive_string() << '\n';
		file.flush();

		bool writeSuccess = file.good();
		file.close();

		if (!writeSuccess) {
			logger::error("'put' write failed : {}", form->make_archive_string());
			return nullptr;
		}

		logger::info("'put' added to archive : {}", form->make_archive_string());
	} else {
		logger::info("'put' skipped (already exists) : {}", form->make_archive_string());
	}

	return form;
}

bool compare_formId_string(const std::string& a, const std::string& b)
{
	if (a.empty() || b.empty()) {
		return false;
	}

	// Удаляем префикс "0x" (если он есть)
	std::string_view a_view = a;
	std::string_view b_view = b;

	if (a_view.starts_with("0x")) {
		a_view.remove_prefix(2);
	}
	if (b_view.starts_with("0x")) {
		b_view.remove_prefix(2);
	}

	if (a_view.empty() || b_view.empty()) {
		return false;
	}

	// Удаляем префикс "01" (если он есть)
	if (a_view.size() > 6) {
		a_view = a_view.substr(a_view.size() - 6);
	}
	if (b_view.size() > 6) {
		b_view = b_view.substr(b_view.size() - 6);
	}

	size_t pos = 0;
	unsigned long value_a = std::stoul(std::string(a_view), &pos, 16);
	if (pos != a_view.size()) {
		return false;
	}

	pos = 0;
	unsigned long value_b = std::stoul(std::string(b_view), &pos, 16);
	if (pos != b_view.size()) {
		return false;
	}

	return value_a == value_b;
};

bool compare_source_string(const std::string& a, const std::string& b)
{
	if (a.length() != b.length()) {
		return false;
	}

	if (a == b) {
		return true;
	}

	return std::equal(a.begin(), a.end(), b.begin(),
		[](char lhs, char rhs) {
			return std::tolower(lhs) == std::tolower(rhs);
		});
}

std::string Form::get_form_str() const
{
	return m_form;
}

std::string Form::get_source_str() const
{
	return m_source;
}
