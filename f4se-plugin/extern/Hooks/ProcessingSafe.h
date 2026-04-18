#pragma once
#include <chrono>
#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <unordered_set>

class ProcessingNPC
{
public:
	using map = std::unordered_set<uint32_t>;
	using value_type = typename map::value_type;

private:
	std::unique_ptr<map> ptr_map;
	mutable std::shared_mutex mutex;

	// Запрет копирования и перемещения
	ProcessingNPC(const ProcessingNPC&) = delete;
	ProcessingNPC& operator=(const ProcessingNPC&) = delete;
	ProcessingNPC(ProcessingNPC&&) = delete;
	ProcessingNPC& operator=(ProcessingNPC&&) = delete;

public:
	ProcessingNPC();

	~ProcessingNPC() = default;

	void insert(uint32_t key);
	bool contains(uint32_t key) const;
	void erase(uint32_t key);
	size_t size() const;
	bool empty() const;

	std::string print();

	std::optional<uint32_t> find(uint32_t key) const;

	class SafeIterator
	{
	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = uint32_t;
		using difference_type = std::ptrdiff_t;
		using pointer = uint32_t*;
		using reference = uint32_t&;

		SafeIterator(const ProcessingNPC& container, typename map::const_iterator iter);
		SafeIterator(const SafeIterator&) = delete;
		SafeIterator& operator=(const SafeIterator&) = delete;
		SafeIterator(SafeIterator&&) = default;
		SafeIterator& operator=(SafeIterator&&) = default;

		uint32_t operator*() const;
		SafeIterator& operator++();
		bool operator!=(const SafeIterator& other) const;

	private:
		const ProcessingNPC& container;
		typename map::const_iterator iter;
		mutable std::shared_lock<std::shared_mutex> lock;
	};

	SafeIterator begin() const;
	SafeIterator end() const;
};
