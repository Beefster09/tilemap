#pragma once

#include <assert.h>

#include "common.h"
// Implements a basic generational index table

template <typename T>
class Table {
	struct Entry {
		u32 generation = 0;
		bool occupied = false;
		T item;

		Entry(const T& item, u32 generation, bool occupied): item(item), generation(generation), occupied(occupied) {}
		Entry(T&& item, u32 generation, bool occupied): item(item), generation(generation), occupied(occupied) {}

		friend struct Handle;
	};

	Entry* data;  // Yes, std::vector *really is* that slow, especially the MSVC implementation.
	size_t capacity;

public:
	struct Handle {
		Table* table;
		u32 index;
		u32 generation;

		bool is_valid() const {
			return table != nullptr && index < table->capacity && generation == table->data[index].generation;
		}
		operator bool() const {
			return is_valid();
		}

		T& operator * () const {
			assert(is_valid());
			return table->data[index].item;
		}

		T* operator -> () const {
			assert(is_valid());
			return &table->data[index].item;
		}
	};

	Table(size_t initial_capacity = 64) {
		data = (Entry*) calloc(initial_capacity, sizeof(Entry));
		capacity = initial_capacity;
	}

	Handle add(T&& item) {
		for (u32 i = 0; i < capacity; i++) {
			if (!data[i].occupied) {
				data[i].item = item;
				data[i].generation++;
				data[i].occupied = true;
				return {this, i, data[i].generation};
			}
		}
		// No more room.
		return { nullptr, 0, 0 };
		// Table is full. Add an entry.
		//data.emplace_back(item, 1, true);
		//return {this, (u32) data.size() - 1, 1};
	}

	Handle add(const T& item) {
		return add(T(item));
	}

	bool remove(const Handle id) {
		if (id.table != this) return false;
		auto candidate = data[id.index];
		if (candidate.generation != id.generation) return false;
		candidate.occupied = false;
		return true;
	}

	T& operator [] (const size_t index) {
		return data[index].item;
	}

	u32 fill_index(u32* const items, const u32 buf_size) {
		u32 cur = 0;
		for (u32 i = 0; i < capacity; i++) {
			if (data[i].occupied) {
				items[cur++] = i;
			}
		}
		return cur;
	}

	size_t count() {
		size_t c = 0;
		for (int i = 0; i < data.size(); i++) {
			c += data[i].occupied? 1 : 0;
		}
		return c;
	}
};