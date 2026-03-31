#include "BOBHash32.h"

#define G 8
#define HK_b 1.08

class HeavyGuarding
{
    private:
        int HK_w;
        struct node {int C; uint32_t FP;} **HK;
        BOBHash32 *bobhash;
    public:
        explicit HeavyGuarding(int HK_w):HK_w(HK_w){
            HK = new node*[HK_w];
            for (int i = 0; i < HK_w; i++)
                HK[i] = new node[G];
            for(int i=0; i<HK_w; i++)
                for(int j=0; j<G; j++){
                    HK[i][j].C = 0;
                    HK[i][j].FP = 0;
                }
            bobhash=new BOBHash32(750);
        }

        void insert(uint32_t key)
        {
            uint32_t index = bobhash->run((const char *)&key, 4) % HK_w;
            bool FLAG=false;
            for (int k=0; k<G; k++)
            {
                if (HK[index][k].FP==key)
                {
                    HK[index][k].C++;
                    FLAG=true;
                    break;
                }
            }
            if (!FLAG)
            {
                int X,MIN=1000000000;
                for (int k=0; k<G; k++)
                {
                    int c=HK[index][k].C;
                    if (c<MIN) {MIN=c; X=k;}
                }
                if (!(rand()%int(pow(HK_b,HK[index][X].C))))
                {
                    HK[index][X].C--;
                    if (HK[index][X].C<=0)
                    {
                        HK[index][X].FP=key;
                        HK[index][X].C=1;
                    }
                }
            }
        }

        int query(uint32_t key)
        {
            uint32_t index=bobhash->run((const char *)&key, 4) % HK_w;
            for (int k=0; k<G; k++)
                if (HK[index][k].FP==key)
                    return HK[index][k].C;
            return 0;
        }
};
