#include "BOBHash32.h"

class MRAC
{
private:
    int w;
	int *counters{};
	BOBHash32 *hash{};
public:
    MRAC(int w): w(w){
        counters = new int[w];
        for (int i = 0; i < w; i++)
            counters[i] = 0;
        hash = new BOBHash32(750);
    }

    void insert(uint32_t key)
    {
        uint32_t pos = hash->run((const char *)&key, 4) % w;
        counters[pos] += 1;
    }

    int get_distribution(vector<int> &dist_est) {
        int max_counter_val = 0;
        for (int i = 0; i < w; i++) {
            max_counter_val = max(max_counter_val, counters[i]);
        }
        dist_est.resize(max_counter_val + 1);
        fill(dist_est.begin(), dist_est.end(), 0);
        for (int i = 0; i < w; i++) {
            dist_est[counters[i]]++;
        }
        return max_counter_val;
    }

};
