#include "ProcessingSafe.h"

ProcessingNPC::ProcessingNPC() :
	ptr_map(std::make_unique<map>()) {}

void ProcessingNPC::insert(uint32_t key)
{
	std::scoped_lock lock(mutex);  
	ptr_map->insert(key);
}

bool ProcessingNPC::contains(uint32_t key) const
{
	std::shared_lock lock(mutex);  
	return ptr_map->find(key) != ptr_map->end();
}

void ProcessingNPC::erase(uint32_t key)
{
	std::scoped_lock lock(mutex);  
	ptr_map->erase(key);
}

size_t ProcessingNPC::size() const
{
	std::shared_lock lock(mutex);  
	return ptr_map->size();
}

bool ProcessingNPC::empty() const
{
	std::shared_lock lock(mutex); 
	return ptr_map->empty();
}

std::optional<uint32_t> ProcessingNPC::find(uint32_t key) const
{
	std::shared_lock lock(mutex);  
	auto it = ptr_map->find(key);
	if (it != ptr_map->end()) {
		return *it;
	}
	return std::nullopt;
}

std::string ProcessingNPC::print()
{
	std::string result;
	if (ptr_map->empty()) {
		result = "Current map is empty!";
	} else {
		result = "Current map :\n";
		bool first = true;
		for (auto el : *ptr_map) {
			if (!first) {
				result += ", ";
			}
			result += std::format("{:x}", el);
			first = false;
		}
	}
	return result;
}

// Реализация SafeIterator
ProcessingNPC::SafeIterator::SafeIterator(const ProcessingNPC& container, typename map::const_iterator iter) :
	container(container), iter(iter), lock(container.mutex) {}

uint32_t ProcessingNPC::SafeIterator::operator*() const
{
	return *iter;
}

ProcessingNPC::SafeIterator& ProcessingNPC::SafeIterator::operator++()
{
	++iter;
	return *this;
}

bool ProcessingNPC::SafeIterator::operator!=(const SafeIterator& other) const
{
	return iter != other.iter;
}

ProcessingNPC::SafeIterator ProcessingNPC::begin() const
{
	return SafeIterator(*this, ptr_map->begin());
}

ProcessingNPC::SafeIterator ProcessingNPC::end() const
{
	return SafeIterator(*this, ptr_map->end());
}
