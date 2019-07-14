#pragma once

#include <cassert>

#include "common.h"
// Implements a basic generational index table

int findZeroBit(const u64 section);

constexpr u64 CAP_ROUND = ~0x3F;

template <typename T>
class Table {
	T* data;  // Yes, std::vector *really is* that slow, especially the MSVC implementation.
	size_t capacity;
	u64* occupied;
	u32* generation;

public:
	Table(size_t capacity = 64) {
		capacity = (capacity + 63) & CAP_ROUND; // round capacity up to nearest 64;
		this->capacity = capacity;
		data = (T*) malloc(capacity * sizeof(T));
		generation = (u32*) calloc(capacity, sizeof(u32));
		occupied = (u64*) calloc(capacity / 64, sizeof(u64));
	}

	struct Handle {
		Table* table;
		u32 index;
		u32 generation;

		bool is_valid() const {
			return table != nullptr && index < table->capacity && generation == table->generation[index];
		}
		operator bool() const {
			return is_valid();
		}

		T& operator * () const {
			assert(is_valid());
			return table->data[index];
		}

		T* operator -> () const {
			assert(is_valid());
			return &table->data[index];
		}
	};

	Handle add(T&& item) {
		auto occ_len = capacity >> 6;
		for (u32 i = 0; i < occ_len; i++) {
			int bit = findZeroBit(occupied[i]);
			if (bit >= 0) {
				int index = (i << 6) + bit;
				data[index] = item;
				generation[index]++;
				BITSET(occupied[i], bit);
				return {this, (u32) index, generation[index]};
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
		if (generation[id.index] != id.generation) return false;
		BITCLEAR(occupied[id.index >> 6], id.index & 0x3F);
		return true;
	}

	T& operator [] (const size_t index) {
		return data[index];
	}

	u32 fill_index(u32* const items, const u32 buf_size) {
		u64 cur = 0;
		for (u64 i = 0; i < capacity; i++) {
			if (BITOF(occupied[i >> 6], i & 0x3F) != 0l) {
				items[cur++] = i;
				if (cur >= buf_size) return cur;
			}
		}
		return cur;
	}

	size_t count() {
		size_t c = 0;
		auto occ_len = capacity >> 6;
		for (int i = 0; i < occ_len; i++) {
			// TODO: __popcnt64 is an intrinsic, so it needs to be compiler-switched for architectures that don't support it.
			c += __popcnt64(occupied[i]);
		}
		return c;
	}
};