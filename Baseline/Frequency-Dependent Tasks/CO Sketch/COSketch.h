#include <algorithm>
#include "BOBHash32.h"

class COSketch
{
private:
    int d, w;
    int **counters;
    BOBHash32 **hash{};
public:
    COSketch(int d, int w): d(d), w(w){
        counters = new int*[d];
        for (int i = 0; i < d; i++)
            counters[i] = new int[w];
        for (int i = 0; i < d; i++)
            for (int j = 0; j < w; j++)
                counters[i][j] = 0;
        hash = new BOBHash32*[2*d];
        for (int i = 0; i < 2*d; i++)
            hash[i] = new BOBHash32(i + 750);
    }

    void insert(uint32_t key)
    {
        int j = 0;
        uint32_t index, sign;
        for (int i = 0; i < 2*d; i+=2)
    	{
			index = hash[i]->run((const char*)&key, 4) % w;
    		sign = hash[i+1]->run((const char*)&key, 4) % 2;

            counters[j][index] += sign ? 1 : -1;
            j++;
        }
    }

	int query(uint32_t key)
    {
        int result[d], j=0;
        uint32_t index, sign;
    	for (int i = 0; i < 2*d; i+=2)
    	{
			index = hash[i]->run((const char*)&key, 4) % w;
    		sign = hash[i+1]->run((const char*)&key, 4) % 2;

    		result[j] = sign ? counters[j][index] : -counters[j][index];
            j++;
    	}

    	sort(result, result + d);

    	int mid = d / 2;
    	int ret;
    	if(d % 2 == 0)
    		ret = (result[mid] + result[mid - 1]) / 2;
    	else
    		ret = result[mid];

    	return ret;
    }

};
