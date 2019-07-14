
#include <cassert>
#include "common.h"

constexpr u64 ALL = ~0;

int findZeroBit(const u64 section) {
	if (section == ALL) return -1;
	// at least one bit is still zero
	int index = 0;
	u64 partial = section;
	for (int half = 32; half > 0; half = half >> 1) {
		u64 mask = ALL >> (64 - half);
		u64 low = partial & mask;
		if (low == mask) {
			index += half;
			partial = partial >> half;
		}
		else {
			partial = low;
		}
	}
	assert(BITOF(section, index) == 0);
	return index;
}