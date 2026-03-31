#include <cmath>

#include "BOBHash32.h"

#define HK_b 1.08

class heavykeeper
{
    private:
        int HK_d, HK_w;
        struct node {int C;uint32_t FP;} **HK;
        BOBHash32 **hash;
    public:
        heavykeeper(int HK_d, int HK_w): HK_d(HK_d), HK_w(HK_w){
            HK = new node*[HK_d];
            for (int i = 0; i < HK_d; i++)
                HK[i] = new node[HK_w];
            for (int i = 0; i < HK_d; i++)
                for (int j = 0; j < HK_w; j++)
                {
                    HK[i][j].C = 0;
                    HK[i][j].FP = 0;
                }
            hash = new BOBHash32*[HK_d];
            for (int i = 0; i < HK_d; i++)
                hash[i] = new BOBHash32(i + 750);
        }

        
        void insert(uint32_t key)
        {
            uint32_t temp;
            for (int j = 0; j < HK_d; j++){
                temp = hash[j]->run((const char *)&key, 4) % HK_w;
                if (HK[j][temp].FP==key)
                {
                    HK[j][temp].C++;
                }
                else if(HK[j][temp].FP == 0){
                    HK[j][temp].FP=key;
                    HK[j][temp].C = 1;
                }
                else
                {
                    if (!(rand()%int(pow(HK_b,HK[j][temp].C))))
                    {
                        HK[j][temp].C --;
                        if (HK[j][temp].C <= 0)
                        {
                            HK[j][temp].FP=key;
                            HK[j][temp].C=1;
                        }
                    }
                }
            }
        }

        int query(uint32_t key)
        {
            int maxv = 0;
            for(int j = 0; j <HK_d; j++){
                uint32_t Hsh=hash[j]->run((const char *)&key, 4)% HK_w;
                if(HK[j][Hsh].FP==key){
                    maxv = max(maxv, HK[j][Hsh].C);
                }
            }
            return maxv;
        }

};
