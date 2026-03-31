#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <chrono>

#include "MEC_M.h"

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

double demo_mec_m(int mec_m_w)
{
    epoch_sum++;

    auto sketch = new MEC_M(mec_m_w);

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

tuple<int, double> search_mec_m(double rr){
    int meminkb = 10 * 1024;
    int bucket_num =  meminkb * 1024 * 8 / (8 * (32 + hll_bits * hll_width + 32));

    int res_w = 0;
    double res_rr = 0, cur_rr;

    cur_rr = demo_mec_m( bucket_num);
    if(fabs(cur_rr - rr) <= PRO * rr){
        res_w = bucket_num;
        res_rr = cur_rr;
    }
    else if(cur_rr > rr){
        int low = 1, high = bucket_num, mid;
        while(low < high){
            mid = (low + high )/2;
            cur_rr = demo_mec_m(mid);
            if(fabs(cur_rr - rr) <= PRO * rr) {
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
        cout<<"Not enough memory!"<<endl;
    }

    return make_tuple(res_w,res_rr);
}

int main(){
    load_data();
    int w;
    double res;
    auto beforeTime = std::chrono::steady_clock::now();
    tie( w, res) = search_mec_m(0.9);
    auto afterTime = std::chrono::steady_clock::now();
    double duration_millsecond = std::chrono::duration<double, std::milli>(afterTime - beforeTime).count();
    std::cout << duration_millsecond << "ms" << std::endl;

    cout<<"Epoch: "<<epoch_sum<<endl;
    cout<<"W: "<<w<<" RR: "<<res<<endl;

    return 0;
}
