#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <chrono>

#include "MEC_P.h"

using namespace std;

#define MAX_INSERT_PACKAGE 25000000
#define PRO 0.05
#define threshold 250

unordered_map<uint32_t, unordered_set<uint32_t>> ground_truth;
vector<pair<uint32_t, uint32_t>> insert_data;

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

    char src[4], dst[4];
    pair<uint32_t,uint32_t> temp{};
    while (true) {
        size_t rsize;
        rsize = fread(src, 1, 4, pf);
        if(rsize != 4) break;
        rsize = fread(dst, 1, 4, pf);
        if(rsize != 4) break;
        temp.first = *(uint32_t *) src;
        temp.second = *(uint32_t *) dst;
        insert_data.push_back(temp);
        ground_truth[temp.first].insert(temp.second);
        packet_num++;
        if (packet_num == MAX_INSERT_PACKAGE){
            cout << "MAX_INSERT_PACKAGE" << endl;
            break;
        }
    }
    fclose(pf);

    printf("Total packets = %d\n", packet_num);
    printf("Distinct flow number = %zu\n", ground_truth.size());
}

double demo_mec_p(int hash_num, int mec_p_w)
{
    epoch_sum++;

    auto sketch = new MEC_P(hash_num, mec_p_w);

    for (int i = 0; i < packet_num; ++i) {
        sketch->insert(insert_data[i].first, insert_data[i].second);
    }

    vector<pair<uint32_t, uint32_t>> results{};
    sketch->query_superspreader(threshold, results);

    double recall = 0;
    int tp = 0, cnt = 0;
    for (const auto& it : ground_truth) {
        int truth = (int)it.second.size();
        if(truth >= threshold) {
            cnt++;
            for(auto &result : results) {
                if(result.first == it.first) {
                    tp++;
                    break;
                }
            }
        }
    }
    recall = tp*1.0/cnt;
    return recall;
}

class Parm
{
public:
    int k;
    int w;
    double rr;

    Parm( int k_init, int w_init, double rr_init){
        k = k_init;
        w = w_init;
        rr = rr_init;
    }
};

tuple<int, int, double> search_mec_p(double rr){
    int meminkb = 10 * 1024;
    int bucket_num_one = meminkb * 1024 * 8 / (32 + hll_bits * hll_width * 2 + 32 * 2);

    vector<Parm> vec;

    int res_k = 0, res_w = 0;
    double res_rr = 0, cur_rr = 0;

    for(int k=1; k<=3; k++){
        int bucket_num = bucket_num_one/k;
        cur_rr = demo_mec_p(k,bucket_num);
        if(fabs(cur_rr - rr) <= PRO * rr) {
            res_k = k;
            res_w = bucket_num;
            res_rr = cur_rr;
        }
        else if(cur_rr > rr){
            int low = 1, high = bucket_num, mid;
            while(low < high){
                mid = (low + high )/2;
                cur_rr = demo_mec_p(k,mid);
                if(fabs(cur_rr - rr) <= PRO * rr) {
                    res_k = k;
                    res_w = mid;
                    res_rr = cur_rr;
                    break;
                }
                else if(cur_rr > rr)
                    high = mid - 1;
                else
                    low = mid + 1;
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
            res_rr = i.rr;
        }
    }

    return make_tuple(res_k,res_w,res_rr);
}

int main(){
    load_data();
    int k, w;
    double res;
    auto beforeTime = std::chrono::steady_clock::now();
    tie( k , w, res) = search_mec_p(0.9);
    auto afterTime = std::chrono::steady_clock::now();
    double duration_millsecond = std::chrono::duration<double, std::milli>(afterTime - beforeTime).count();
    std::cout << duration_millsecond << "ms" << std::endl;

    cout<<"Epoch: "<<epoch_sum<<endl;
    cout<<"K: "<<k<<" W: "<<w<<" RR: "<<res<<endl;

    return 0;
}