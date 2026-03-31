#include <cstring>
#include <valarray>
#include "BOBHash32.h"

class rSkt1
{
private:
    int d{}, w{};
    int m{};
    double alpha{};
    double setmax{};

    uint8_t*** ca_v{};
    uint8_t*** ca_nv{};

    BOBHash32 **hash1{};
    BOBHash32 **hash2{};

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
    rSkt1(int d, int w, int hll_width): d(d), w(w)
    {
        m = hll_width;
        alpha = 0.7213*m*m/(1+1.079/m);
        setmax = m*0.07;

        ca_v = new uint8_t **[d];
        ca_nv = new uint8_t **[d];
        for(int i=0; i<d; i++){
            ca_v[i] = new uint8_t *[w];
            ca_nv[i] = new uint8_t *[w];
            for(int j=0; j<w; j++){
                ca_v[i][j] = new uint8_t[m]();
                ca_nv[i][j] = new uint8_t[m]();
            }
        }

        hash1 = new BOBHash32*[d+1];
        for (int i = 0; i < d+1; i++)
            hash1[i] = new BOBHash32(i + 750);

        hash2 = new BOBHash32*[d];
        for (int i = 0; i < d; i++)
            hash2[i] = new BOBHash32(i + 850);
    }

    void insert(uint32_t src, uint32_t dst)
    {
        uint64_t edge = src;
        edge = (edge << 32) | dst;
        uint32_t hashval = hash1[d]->run((const char *)&edge, 8);
        uint8_t tmplevel = loghash(hashval);
        uint32_t pos = ((hashval * ((uint64_t)m)) >> 32);

        for (int i = 0; i < d; i++) {
            uint32_t buc = ((hash1[i]->run((const char *)&src, 4) * ((uint64_t)w)) >> 32);
            if (hash2[i]->run((const char *)&src, 4) & 1)
                ca_v[i][buc][pos] = max(ca_v[i][buc][pos], tmplevel);
            else
                ca_nv[i][buc][pos] = max(ca_nv[i][buc][pos], tmplevel);
        }
    }

    int query(uint32_t src) {
        int temp1{}, temp2{}, x{};
        int val = (1<<30);
        uint32_t index{};
        for (int i = 0; i < d; i++) {
            index = ((hash1[i]->run((const char *)&src, 4) * ((uint64_t)w)) >> 32);
            int ca_v_val = estimate(ca_v[i][index]);
            int ca_nv_val = estimate(ca_nv[i][index]);
            if(val > ca_v_val + ca_nv_val){
                val = ca_v_val + ca_nv_val;
                x = i;
                temp1 = ca_v_val;
                temp2 = ca_nv_val;
            }
        }

        if(hash2[x]->run((const char *)&src, 4) & 1)
            val = temp1 - temp2;
        else
            val = temp2 - temp1;
        return val;
    }

};
