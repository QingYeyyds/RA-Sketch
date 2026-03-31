#include <iostream>
#include <unordered_map>
#include <chrono>
#include <vector>

#include "HK.h"

using namespace std;

#define MAX_INSERT_PACKAGE 25000000
#define PRO 0.05
#define thre 250

unordered_map<uint32_t, int> ground_truth;
uint32_t insert_data[MAX_INSERT_PACKAGE];

int packet_num = 0;
int epoch_sum = 0;

void load_data() {
    const char *filename="dataset";
    FILE *pf = fopen(filename, "rb");
    if (!pf) {
        cerr << filename << " not found." << endl;
        exit(-1);
    }

    ground_truth.clear();

    char ip[4];
    while (true) {
        size_t rsize;
        rsize = fread(ip, 1, 4, pf);
        if(rsize != 4) break;
        uint32_t key = *(uint32_t *) ip;
        insert_data[packet_num] = key;
        ground_truth[key]++;
        packet_num++;
        if (packet_num == MAX_INSERT_PACKAGE){
            cout << "MAX_INSERT_PACKAGE" << endl;
            break;
        }
    }
    fclose(pf);

    printf("Total packets = %d\n", packet_num);
    printf("Distinct flow number = %ld\n", ground_truth.size());
}

double demo_hk(int hash_num, int hk_w)
{
    epoch_sum++;

    auto hk = new heavykeeper(hash_num, hk_w);

    for (int i = 0; i < packet_num; ++i) {
        hk->insert(insert_data[i]);
    }

    double tot_recall = 0, tot_recall_ = 0;
    for (auto itr: ground_truth) {
        if(itr.second >= thre){
            tot_recall += 1;
            if(hk->query(itr.first) >= thre)
                tot_recall_ += 1;
        }
    }
    return tot_recall_/tot_recall;
}

class Parm
{
public:
    int k;
    int w;
    double met;

    Parm( int k_init, int w_init, double met_init){
        k = k_init;
        w = w_init;
        met = met_init;
    }
};

tuple<int, int, double> search_hk(double rr){
    int meminkb = 10 * 1024;
    int bucket_num_one = meminkb * 1024 * 8 / 64;

    vector<Parm> vec;

    int res_k = 0, res_w = 0;
    double res_rr = 0, cur_rr;

    for(int k=1; k<=3; k++){
        int bucket_num = bucket_num_one/k;
        cur_rr = demo_hk(k, bucket_num);
        if(fabs(cur_rr - rr) <= PRO * rr){
            res_k = k;
            res_w = bucket_num;
            res_rr = cur_rr;
        }
        else if(cur_rr > rr){
            int low = 1, high = bucket_num, mid;
            while(low < high){
                mid = (low + high )/2;
                cur_rr = demo_hk(k, mid);
                if(fabs(cur_rr - rr) <= PRO * rr) {
                    res_k = k;
                    res_w = mid;
                    res_rr = cur_rr;
                    break;
                }
                else if(cur_rr < rr)
                    low = mid + 1;
                else
                    high = mid - 1;
            }
        }
        else{
            continue;
        }
        vec.emplace_back(res_k,res_w,res_rr);
    }

    int min_mem = (1<<30), cur_mem;
    for(auto i:vec){
        cur_mem = i.k * i.w;
        if(cur_mem < min_mem){
            min_mem = cur_mem;
            res_k = i.k;
            res_w = i.w;
            res_rr = i.met;
        }
    }

    return make_tuple(res_k,res_w,res_rr);
}

int main(){
    load_data();
    int k , w;
    double res;
    auto beforeTime = chrono::steady_clock::now();
    tie( k , w, res) = search_hk(0.8);
    auto afterTime = std::chrono::steady_clock::now();
    double duration_millsecond = std::chrono::duration<double, std::milli>(afterTime - beforeTime).count();
    std::cout << duration_millsecond << "ms" << std::endl;

    cout<<"Epoch: "<<epoch_sum<<endl;
    cout<<"K: "<<k<<" W: "<<w<<" RR: "<<res<<endl;

    return 0;
}
