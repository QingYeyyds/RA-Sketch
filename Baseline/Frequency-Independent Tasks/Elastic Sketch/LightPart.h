#ifndef _LIGHT_PART_H_
#define _LIGHT_PART_H_

#include "param.h"

class LightPart
{
	int counter_num;
	BOBHash32 *bobhash = nullptr;

public:
	uint8_t *counters;

	explicit LightPart(int init_mem_in_bytes)
	{
        counter_num = init_mem_in_bytes;
        counters = new uint8_t[counter_num];
		clear();
		std::random_device rd;
       	bobhash = new BOBHash32(rd() % MAX_PRIME32);
	}

    ~LightPart()
    {
        delete bobhash;
    }

	void clear() const
	{
		memset(counters, 0, counter_num);
	}

	void insert(uint8_t *key, int f = 1)
	{
		uint32_t hash_val = bobhash->run((const char*)key, KEY_LENGTH_4);
		uint32_t pos = hash_val % (uint32_t)counter_num;

        int new_val = (int)counters[pos] + f;

        new_val = new_val < 255 ? new_val : 255;
        counters[pos] = (uint8_t)new_val;
	}

	void swap_insert(uint8_t *key, int f)
	{
		uint32_t hash_val = bobhash->run((const char*)key, KEY_LENGTH_4);
        uint32_t pos = hash_val % (uint32_t)counter_num;

        f = f < 255 ? f : 255;
        if (counters[pos] < f) 
        {
            counters[pos] = (uint8_t)f;
		}
	}

	int query(uint8_t *key) 
	{
        uint32_t hash_val = bobhash->run((const char*)key, KEY_LENGTH_4);
        uint32_t pos = hash_val % (uint32_t)counter_num;

        return (int)counters[pos];
    }

};

#endif