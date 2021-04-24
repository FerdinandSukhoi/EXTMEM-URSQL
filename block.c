#include "block.h"
#include <stdio.h>

block_t* create_block(buffer_t buffer)
{
	block_t* r = (block_t*)getNewBlockInBuffer(buffer);
	memset(r, 0, sizeof(block_t));
	return r;
}
inline bool save_data(void* addr, data_t data)
{
	char buf[4];
	if (data > 9999) return false;
	char* const exp = (char*)addr;
	if (data == 0)
	{
		exp[0] = '0';
		exp[1] = 0;
		exp[2] = 0;
		exp[3] = 0;
		return true;
	}
	char* const start = buf;
	char* d = start + 3;
	for (; d >= start; d--)
	{
		*d = (char)('0' + data % 10);
		data /= 10;
	}

	for (size_t i = 0, j = 0; i < 4; i++)
	{
		while (buf[j] == '0' && j < 4)
		{
			j++;
		}
		exp[j] = (char)(j >= 4 ? 0 : buf[j]);

	}
	return true;
}
bool save_block(block_t* block, address_t address, buffer_t buffer)
{
	for (size_t i = 0; i < 7; i++)
	{
		if (!save_data(&block->items[i].first, block->items[i].first))return false;
		if (!save_data(&block->items[i].second, block->items[i].second))return false;
	}
	if (!save_data(&block->next, (data_t)block->next)) return false;
	return !writeBlockToDisk((unsigned char*)block, address, buffer);
}
inline data_t load_data(void* addr)
{
	data_t r = 0;
	char* start = (char*)addr;
	for (size_t i = 0; i < 4; i++)
	{
		if (start[i] == 0) continue;
		r *= 10;
		r += start[i] - '0';

	}

	return r;
}
block_t* load_block(const address_t address, const buffer_t buffer)
{
	block_t* block = (block_t*)readBlockFromDisk(address, buffer);
	for (size_t i = 0; i < 7; i++)
	{
		block->items[i].first = load_data(&block->items[i].first);
		block->items[i].second = load_data(&block->items[i].second);
	}
	block->next = load_data(&block->next);
	printf("读入数据块%lld\n", address);
	return block;
}

static inline data_t key_of(item_t item, name_t key)
{
	// A(65),C(67)则使用第一项
	return key & 1 ? item.first : item.second;
}

address_t sort_block(address_t addr, name_t key, buffer_t buffer)
{
	block_t* const block = load_block(addr, buffer);
	item_t* const start = block->items;  // NOLINT(clang-diagnostic-incompatible-pointer-types-discards-qualifiers)
	item_t* end = (item_t*)block->items + 6;  // NOLINT(clang-diagnostic-incompatible-pointer-types-discards-qualifiers, clang-diagnostic-cast-qual)
	/* Note: in assertions below, i and j are always inside original bound of array to sort. */
	while (end > start)
	{
		item_t* max = start;
		for (item_t* p = start + 1; p <= end; p++)
			if (key_of(*p, key) > key_of(*max, key))
				max = p;
		item_t const swap_buf = *end;
		*end = *max;
		*max = swap_buf;
		end--;
	}
	const address_t next_addr = block->next;
	save_block(block, addr + URSQL_BLOCK_SORTED_BASE, buffer);
	return next_addr;
}
