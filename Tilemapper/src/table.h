#pragma once

#include <vector>
#include <assert.h>

#include "common.h"
// Implements a basic generational index table

template <typename T>
class Table {
	struct Entry {
		T item;
		u32 generation = 0;
		bool occupied = false;

		Entry(const T& item, u32 generation, bool occupied): item(item), generation(generation), occupied(occupied) {}
		Entry(T&& item, u32 generation, bool occupied): item(item), generation(generation), occupied(occupied) {}

		friend struct Handle;
	};

	std::vector<Entry> data;

public:
	struct Handle {
		Table* table;
		u32 index;
		u32 generation;

		bool is_valid() {
			return table != nullptr && index < table->data.size() && generation == table->data[index].generation;
		}

		T& operator * () {
			assert(is_valid());
			return data[index].item;
		}

		T* operator -> () {
			if(is_valid()) return &data[index].item;
			else return nullptr;
		}
	};

	Table(size_t initial_capacity=64) { data.reserve(initial_capacity); }

	Handle add(T&& item) {
		for (u32 i = 0; i < data.size(); i++) {
			if (!data[i].occupied) {
				data[i].item = item;
				data[i].generation++;
				data[i].occupied = true;
				return {this, i, data[i].generation};
			}
		}
		// Table is full. Add an entry.
		data.emplace_back(item, 1, true);
		return {this, (u32) data.size() - 1, 1};
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

	template <typename S=size_t>
	std::vector<S>&& create_index() {
		std::vector<S> items;
		items.reserve(data.size());
		for (int i = 0; i < data.size(); i++) {
			if (data[i].occupied) {
				items.push_back(i);
			}
		}
		items.shrink_to_fit();
		return std::move(items);
	}

	template <typename S>
	S fill_index(std::vector<S>& items) {
		for (int i = 0; i < data.size(); i++) {
			if (data[i].occupied) {
				items.push_back(i);
			}
		}
		return static_cast<S>(items.size());
	}

	size_t count() {
		size_t c = 0;
		for (int i = 0; i < data.size(); i++) {
			c += data[i].occupied? 1 : 0;
		}
		return c;
	}
};