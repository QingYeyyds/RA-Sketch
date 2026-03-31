#include <iostream>
#include <unordered_map>
#include <chrono>
#include <vector>
#include <cmath>

#include "CMSketch.h"

using namespace std;

#define MAX_INSERT_PACKAGE 25000000
#define PRO 0.05

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

double demo_cm(int hash_num, int cm_w)
{
    epoch_sum++;

    auto cm = new CMSketch(hash_num, cm_w);
    
    for (int i = 0; i < packet_num; ++i) {
        cm->insert(insert_data[i]);
    }

    double tot_ae = 0;
    for (auto itr: ground_truth) {
        int report_val = cm->query(itr.first);
        tot_ae += report_val - itr.second;
    }
    return tot_ae / ground_truth.size();
}

class Parm
{
public:
    int k;
    int w;
    double aae;

    Parm( int k_init, int w_init, double aae_init){
        k = k_init;
        w = w_init;
        aae = aae_init;
    }
};

tuple<int, int, double> search_cm(double aae){
    int meminkb = 10 * 1024;
    int bucket_num_one = meminkb * 1024 * 8 / 32;

    vector<Parm> vec;

    int res_k, res_w;
    double res_aae, cur_aae;

    for(int k=1; k<=3; k++){
        int bucket_num = bucket_num_one/k;
        cur_aae = demo_cm(k,bucket_num);
        if(fabs(cur_aae - aae) <= PRO * aae) {
            res_k = k;
            res_w = bucket_num;
            res_aae = cur_aae;
        }
        else if(cur_aae < aae){
            int low = 1, high = bucket_num, mid;
            while(low < high){
                mid = (low + high )/2;
                cur_aae = demo_cm(k,mid);
                if(fabs(cur_aae - aae) <= PRO * aae) {
                    res_k = k;
                    res_w = mid;
                    res_aae = cur_aae;
                    break;
                }
                else if(cur_aae < aae)
                    high = mid - 1;
                else
                    low = mid + 1;
            }
        }
        else{
            continue;
        }
        vec.emplace_back(res_k,res_w,res_aae);
    }

    int min_mem = (1<<30), cur_mem;
    for(auto i:vec){
        cur_mem = i.k * i.w;
        if(cur_mem < min_mem){
            min_mem = cur_mem;
            res_k = i.k;
            res_w = i.w;
            res_aae = i.aae;
        }
    }

    return make_tuple(res_k,res_w,res_aae);
}

int main(){
    load_data();
    int k , w;
    double res;
    auto beforeTime = std::chrono::steady_clock::now();
    tie( k , w, res) = search_cm(10);
    auto afterTime = std::chrono::steady_clock::now();
	double duration_millsecond = std::chrono::duration<double, std::milli>(afterTime - beforeTime).count();
	std::cout << duration_millsecond << "ms" << std::endl;

    cout<<"Epoch: "<<epoch_sum<<endl;
    cout<<"K: "<<k<<" W: "<<w<<" AAE: "<<res<<endl;

    return 0;
}
