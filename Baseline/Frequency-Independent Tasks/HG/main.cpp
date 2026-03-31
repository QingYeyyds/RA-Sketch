#include <iostream>
#include <unordered_map>
#include <chrono>
#include <cmath>

#include "HeavyGuarding.h"

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

double demo_hg(int hk_w)
{
    epoch_sum++;

    auto hk = new HeavyGuarding(hk_w);

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

tuple<int, double> search_hg(double rr){
    int meminkb = 10 * 1024;
    int bucket_num = meminkb * 1024 * 8 / 64 / G;

    int res_w = 0;
    double res_rr = 0, cur_rr;

    cur_rr = demo_hg( bucket_num);
    if(fabs(cur_rr - rr) <= PRO * rr){
        res_w = bucket_num;
        res_rr = cur_rr;
    }
    else if(cur_rr > rr){
        int low = 1, high = bucket_num, mid;
        while(low < high){
            mid = (low + high )/2;
            cur_rr = demo_hg(mid);
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
    tie( w, res) = search_hg(0.8);
    auto afterTime = std::chrono::steady_clock::now();
    double duration_millsecond = std::chrono::duration<double, std::milli>(afterTime - beforeTime).count();
    std::cout << duration_millsecond << "ms" << std::endl;

    cout<<"Epoch: "<<epoch_sum<<endl;
    cout<<"W: "<<w<<" RR: "<<res<<endl;

    return 0;
}
