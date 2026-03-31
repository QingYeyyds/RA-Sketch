#ifndef _ELASTIC_SKETCH_H_
#define _ELASTIC_SKETCH_H_

#include "HeavyPart.h"
#include "LightPart.h"

class ElasticSketch
{

    int heavy_mem;
    int light_mem;

    HeavyPart *heavy_part;
    LightPart *light_part;

public:
    ElasticSketch(int bucket_num, int tot_memory_in_bytes){
        heavy_mem = bucket_num * COUNTER_PER_BUCKET * 8;
        light_mem = tot_memory_in_bytes - heavy_mem;
        heavy_part = new HeavyPart(bucket_num);
        light_part = new LightPart(light_mem);
    }
    ~ElasticSketch()= default;

    void insert(uint8_t *key, int f = 1)
    {
        uint8_t swap_key[KEY_LENGTH_4];
        uint32_t swap_val = 0;
        int result = heavy_part->insert(key, swap_key, swap_val, f);

        switch(result)
        {
            case 0: return;
            case 1:{
                if(HIGHEST_BIT_IS_1(swap_val))
                    light_part->insert(swap_key, GetCounterVal(swap_val));
                else
                    light_part->swap_insert(swap_key, swap_val);
                return;
            }
            case 2: light_part->insert(key, 1);  return;
            default:
                printf("error return value !\n");
                exit(1);
        }
    }

    int query(uint8_t *key)
    {
        uint32_t heavy_result = heavy_part->query(key);
        if(HIGHEST_BIT_IS_1(heavy_result))
        {
            int light_result = light_part->query(key);
            return (int)GetCounterVal(heavy_result) + light_result;
        }
        return heavy_result;
    }

};

#endif