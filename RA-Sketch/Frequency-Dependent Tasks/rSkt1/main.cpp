#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <string>

using namespace std;

#define MAX_INSERT_PACKAGE 25000000
#define PRO 0.05

vector<pair<uint32_t, int>> items;

int epoch_sum = 0;
int card_num = 0;

void load_data() {
    unordered_map<uint32_t, unordered_set<uint32_t>> ground_truth;
    ground_truth.clear();

    const char *filename= "dataset";
    FILE *pf = fopen(filename, "rb");
    if (!pf) {
        cerr << filename << " not found." << endl;
        exit(-1);
    }

    char src[4], dst[4];
    pair<uint32_t,uint32_t> temp{};
    int ret = 0;
    while (true) {
        size_t rsize;
        rsize = fread(src, 1, 4, pf);
        if(rsize != 4) break;
        rsize = fread(dst, 1, 4, pf);
        if(rsize != 4) break;
        temp.first = *(uint32_t *) src;
        temp.second = *(uint32_t *) dst;
        ground_truth[temp.first].insert(temp.second);
        ret++;
        if (ret == MAX_INSERT_PACKAGE){
            cout << "MAX_INSERT_PACKAGE" << endl;
            break;
        }
    }
    fclose(pf);

    int num = 0;
    for(const auto& itr:ground_truth){
       num = (int)itr.second.size();
       items.emplace_back(itr.first,num);
       card_num += num;
    }

}

double card_aae(int k, int w){

    epoch_sum++;

    random_device rd;
    mt19937 generator(rd());
    poisson_distribution<int> poissonDist(1.0 * items.size()/w);
    uniform_int_distribution<int> uniformDist(0, items.size() - 1);

    double aae = 0, aae_tot = 0;
    int num = 0, epoch = 1000;

    while(true){
        for(int epo=0; epo<epoch; epo++){
            int minSum = (1<<30);
            int temp1, temp2;
            for (int i = 0; i < k; i++) {
                int numSamples = poissonDist(generator);
                int sum1 = 0, sum2 = 0;
                for (int j = 0; j < numSamples; ++j) {
                    int index = uniformDist(generator);
                    if(generator() % 2)
                        sum1 += items[index].second;
                    else
                        sum2 += items[index].second;
                }
                if(minSum > sum1 + sum2){
                    minSum = sum1 + sum2;
                    temp1 = sum1;
                    temp2 = sum2;
                }
            }
            aae_tot += fabs(temp1 - temp2);
        }
        num += epoch;
        double new_aae = aae_tot/num;
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

    Parm(int k_init, int w_init, double aae_init){
        k = k_init;
        w = w_init;
        aae = aae_init;
    }
};

tuple<int, int, double>  search(double aae){
    double ave_flow_size = card_num * 1.0 / items.size();
    double share_bucket = aae / ave_flow_size + 1;
    int bucket_num = items.size() / share_bucket;

    vector<Parm> vec;

    int res_k, res_w;
    double res_aae, cur_aae;

    for(int k=1; k<=3; k++){
        res_k = 0, res_w = 0;
        cur_aae = card_aae(k, bucket_num);
        if(fabs(cur_aae - aae) <= PRO * aae){
            res_k = k;
            res_w = bucket_num;
            res_aae = cur_aae;
        }
        else if(cur_aae < aae){
            for(int w=bucket_num/2; w>0; w/=2){
                cur_aae = card_aae(k, w);
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
                        cur_aae = card_aae(k, mid);
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
                cur_aae = card_aae(k, w);
                if(fabs(cur_aae - aae) <= PRO * aae){
                    res_k = k;
                    res_w = w;
                    res_aae = cur_aae;
                    break;
                }
                else if(cur_aae < aae){
                    int low = w / 2, high = w, mid;
                    while (low < high){
                        mid = (low + high) / 2;
                        cur_aae = card_aae(k, mid);
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