#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <unordered_map>
#include <string>
#include <algorithm>

using namespace std;

#define MAX_INSERT_PACKAGE 25000000
#define PRO 0.05

vector<pair<uint32_t, int>> items;

int packet_num = 0;
int epoch_sum = 0;

void load_data() {
    unordered_map<uint32_t, int> ground_truth;
    ground_truth.clear();

    const char *filename= "dataset";
    FILE *pf = fopen(filename, "rb");
    if (!pf) {
        cerr << filename << " not found." << endl;
        exit(-1);
    }

    char ip[4];
    while(true){
        size_t rsize;
        rsize = fread(ip, 1, 4, pf);
        if(rsize != 4) break;
        uint32_t key = *(uint32_t *) ip;
        ground_truth[key]++;
        packet_num++;
        if (packet_num == MAX_INSERT_PACKAGE){
            cout <<packet_num<< endl;
            break;
        }
    }
    fclose(pf);

    copy(ground_truth.begin(), ground_truth.end(), back_inserter(items));
}

double co_aae(int k, int w){

    epoch_sum++;

    random_device rd;
    mt19937 generator(rd());
    poisson_distribution<int> poissonDist(1.0 * items.size()/w);
    uniform_int_distribution<int> uniformDist(0, items.size() - 1);

    double aae = 0, aae_tot = 0;
    int num = 0, epoch = 1000;

    vector<int> sums(k);

    int mid = k / 2;
    bool is_jo = false;
    if(k % 2 == 0)
        is_jo = true;

    while(true) {
        for (int epo = 0; epo < epoch; epo++) {
            for (int i = 0; i < k; i++) {
                int numSamples = poissonDist(generator);
                int sum = 0;
                for (int j = 0; j < numSamples; ++j) {
                    int index = uniformDist(generator);
                    sum += (generator() % 2) ? items[index].second : -items[index].second;
                }
                sums[i] = sum;
            }
            sort(sums.begin(), sums.end());
            int medianSum;
            if (is_jo)
                medianSum = (sums[mid] + sums[mid - 1]) / 2;
            else
                medianSum = sums[mid];
            aae_tot += fabs(medianSum);
        }
        num += epoch;
        double new_aae = aae_tot / num;
        if(fabs(new_aae - aae) < 0.0001 * aae)
            break;
        else
            aae = new_aae;
    }

    return aae;
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

tuple<int, int, double> search(double aae){
    double ave_flow_size = packet_num * 1.0 / items.size();
    double share_bucket = aae / ave_flow_size + 1;
    int bucket_num = items.size() / share_bucket;

    vector<Parm> vec;

    int res_k, res_w;
    double res_aae, cur_aae;

    for(int k=1; k<=3; k++){
        res_k = 0, res_w = 0;
        cur_aae = co_aae(k, bucket_num);
        if(fabs(cur_aae - aae) <= PRO * aae){
            res_k = k;
            res_w = bucket_num;
            res_aae = cur_aae;
        }
        else if(cur_aae < aae){
            for(int w=bucket_num/2; w>0; w/=2){
                cur_aae = co_aae(k, w);
                if(fabs(cur_aae - aae) <= PRO * aae){
                    res_k = k;
                    res_w = w;
                    res_aae = cur_aae;
                    break;
                }
                else if(cur_aae > aae){
                    int low = w, high = w * 2, mid;
                    while(low < high){
                        mid = (low + high )/2;
                        cur_aae = co_aae(k, mid);
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
                    break;
                }
            }
        }
        else{
            for(int w=bucket_num*2; ; w*=2){
                cur_aae = co_aae(k, w);
                if(fabs(cur_aae - aae) <= PRO * aae){
                    res_k = k;
                    res_w = w;
                    res_aae = cur_aae;
                    break;
                }
                else if(cur_aae < aae) {
                    int low = w / 2, high = w, mid;
                    while (low < high){
                        mid = (low + high) / 2;
                        cur_aae = co_aae(k, mid);
                        if (fabs(cur_aae - aae) <= PRO * aae) {
                            res_k = k;
                            res_w = mid;
                            res_aae = cur_aae;
                            break;
                        }
                        else if (cur_aae < aae)
                            high = mid - 1;
                        else if (cur_aae > aae)
                            low = mid + 1;
                    }
                    break;
                }
            }
        }
        if(res_k == 0 || res_w == 0)
            continue;
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

int main() {
    load_data();
    int k , w;
    double res;
    auto beforeTime = chrono::steady_clock::now();
    tie( k , w, res) = search(10);
    auto afterTime = chrono::steady_clock::now();
    double duration_millsecond = chrono::duration<double, milli>(afterTime - beforeTime).count();
    cout << duration_millsecond << "ms"<<endl;

    cout<<"Epoch: "<<epoch_sum<<endl;
    cout<<"K: "<<k<<" W: "<<w<<" AAE: "<<res<<endl;

    return 0;
}