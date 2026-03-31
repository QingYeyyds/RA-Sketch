#include <cstring>
#include <valarray>
#include "BOBHash32.h"

class gSkt
{
private:
    int d{}, w{};
    int m{};
    double alpha{};
    double setmax{};

    uint8_t*** ca_v{};

    BOBHash32 **hash{};

    static uint8_t loghash(uint32_t p) {
        uint8_t ret = 0;
        while ((p&0x00000001) == 1) {
            p >>= 1;
            ret++;
        }
        return min(ret+1, 31);
    }

    int estimate(const uint8_t* ca_v_temp) const{
        double res = 0;
        int zeros = 0;
        for (int i = 0; i < m; i++) {
            res += 1.0/int((uint64_t)1 << ca_v_temp[i]);
            zeros = ca_v_temp[i] == 0 ? zeros+1 : zeros;
        }
        if (zeros > setmax) return int(m*log(m*1.0/zeros));
        return (int)(alpha/res);
    }

public:
    gSkt(int d, int w, int hll_width): d(d), w(w)
    {
        m = hll_width;
        alpha = 0.7213*m*m/(1+1.079/m);
        setmax = m*0.07;

        ca_v = new uint8_t **[d];
        for(int i=0; i<d; i++){
            ca_v[i] = new uint8_t *[w];
            for(int j=0; j<w; j++){
                ca_v[i][j] = new uint8_t[m]();
            }
        }

        hash = new BOBHash32*[d+1];
        for (int i = 0; i < d+1; i++)
            hash[i] = new BOBHash32(i + 750);
    }

    void insert(uint32_t src, uint32_t dst)
    {
        uint64_t edge = src;
        edge = (edge << 32) | dst;
        uint32_t hashval = hash[d]->run((const char *)&edge, 8);
        uint8_t tmplevel = loghash(hashval);
        uint32_t pos = ((hashval * ((uint64_t)m)) >> 32);

        for (int i = 0; i < d; i++) {
            uint32_t buc = ((hash[i]->run((const char *)&src, 4) * ((uint64_t)w)) >> 32);
            uint8_t* temp = ca_v[i][buc];
            if(temp[pos] < tmplevel)
            {
                temp[pos] = tmplevel;
            }
        }
    }

    int query(uint32_t src) {
        int val = (1<<30);
        for (int i = 0; i < d; i++) {
            uint32_t buc = ((hash[i]->run((const char *)&src, 4) * ((uint64_t)w)) >> 32);
            val = min(val, estimate(ca_v[i][buc]));
        }
        return val;
    }

};
