#include <algorithm>
#include "BOBHash32.h"

class CMSketch
{
private:
    int d, w;
	int **counters;
	BOBHash32 **hash{};
public:
    CMSketch(int d, int w): d(d), w(w){
        counters = new int*[d];
        for (int i = 0; i < d; i++)
            counters[i] = new int[w];
        for (int i = 0; i < d; i++)
            for (int j = 0; j < w; j++)
                counters[i][j] = 0;
        hash = new BOBHash32*[d];
        for (int i = 0; i < d; i++)
            hash[i] = new BOBHash32(i + 750);
    }

    void insert(uint32_t key)
    {
        uint32_t pos;
        for (int i = 0; i < d; i++) {
            pos = hash[i]->run((const char *)&key, 4) % w;
            counters[i][pos] += 1;
        }
    }

	int query(uint32_t key)
    {
        int ret = (1 << 30);
        uint32_t pos;
        for (int i = 0; i < d; i++) {
            pos = hash[i]->run((const char *)&key, 4) % w;
            ret = min(ret, counters[i][pos]);
        }
        return ret;
    }

};
