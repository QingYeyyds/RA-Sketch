#include <iostream>
#include <unordered_map>
#include <chrono>
#include <vector>
#include <cmath>

#include "MRAC.h"

using namespace std;

#define MAX_INSERT_PACKAGE 25000000
#define PRO 0.05

unordered_map<uint32_t, int> ground_truth;
uint32_t insert_data[MAX_INSERT_PACKAGE];

int packet_num = 0;
int epoch_sum = 0;

vector<int> dist;
int max_counter_val = 0;

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

    for (auto itr : ground_truth)
        max_counter_val = max(max_counter_val, itr.second);
    dist.resize(max_counter_val + 1);
    fill(dist.begin(), dist.end(), 0);
    for (auto itr : ground_truth)
        dist[itr.second]++;

    printf("Total packets = %d\n", packet_num);
    printf("Distinct flow number = %ld\n", ground_truth.size());
}

double demo_mrac(int mrac_w)
{
    epoch_sum++;

    auto mrac = new MRAC(mrac_w);
    
    for (int i = 0; i < packet_num; ++i) {
        mrac->insert(insert_data[i]);
    }

    vector<int> dist_est;
    int max_counter_val_est = mrac->get_distribution(dist_est);

    double WMRE_1 = 0, WMRE_2 = 0;
    for (int i = 1; i <= max_counter_val_est; i++){
        if (i <= max_counter_val){
            WMRE_1 += fabs(dist[i] - dist_est[i]);
            WMRE_2 += double(dist[i] + dist_est[i]) / 2;
        }
        else{
            WMRE_1 += dist_est[i];
            WMRE_2 += double(dist_est[i]) / 2;
        }
    }
    return WMRE_1 / WMRE_2;
}

tuple<int, double> search_mrac(double wmre){
    int meminkb = 10 * 1024;
    int bucket_num = meminkb * 1024 * 8 / 32;

    int res_w = 0;
    double res_wmre = 0, cur_wmre;

    cur_wmre = demo_mrac( bucket_num);
    if(fabs(cur_wmre - wmre) <= PRO * wmre){
        res_w = bucket_num;
        res_wmre = cur_wmre;
    }
    else if(cur_wmre < wmre){
        int low = 1, high = bucket_num, mid;
        while(low < high){
            mid = (low + high )/2;
            cur_wmre = demo_mrac(mid);
            if(fabs(cur_wmre - wmre) <= PRO * wmre) {
                res_w = mid;
                res_wmre = cur_wmre;
                break;
            }
            else if(cur_wmre > wmre)
                low = mid + 1;
            else
                high = mid - 1;
        }
    }
    else{
        cout<<"Not enough memory!"<<endl;
    }

    return make_tuple(res_w,res_wmre);
}

int main(){
    load_data();
    int w;
    double res;
    auto beforeTime = std::chrono::steady_clock::now();
    tie( w, res) = search_mrac(1);
    auto afterTime = std::chrono::steady_clock::now();
	double duration_millsecond = std::chrono::duration<double, std::milli>(afterTime - beforeTime).count();
	std::cout << duration_millsecond << "ms" << std::endl;

    cout<<"Epoch: "<<epoch_sum<<endl;
    cout<<"W: "<<w<<" WMRE: "<<res<<endl;

    return 0;
}
