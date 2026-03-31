#include <iostream>
#include <random>
#include <chrono>
#include <unordered_map>

using namespace std;

#define PRO 0.05
#define MAX_INSERT_PACKAGE 25000000

int TOT_FLOWS = 0;

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
    int ret = 0;
    while(true){
        size_t rsize;
        rsize = fread(ip, 1, 4, pf);
        if(rsize != 4) break;
        uint32_t key = *(uint32_t *) ip;
        ground_truth[key]++;
        ret++;
        if (ret == MAX_INSERT_PACKAGE){
            cout <<"MAX_INSERT_PACKAGE"<< endl;
            break;
        }
    }
    fclose(pf);

    TOT_FLOWS = ground_truth.size();
}

double bloom_fp(int k, int w){
    epoch_sum++;
    return pow(1 - exp(-1.0 * k * TOT_FLOWS/w), k);
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

tuple<int, int, double> search_bf(double fp){
    int meminkb = 10 * 1024;
    int bucket_num_one = meminkb * 1024 * 8;

    vector<Parm> vec;

    int res_k = 0, res_w = 0;
    double res_fp = 0, cur_fp;

    for(int k=1; k<=3; k++){
        int bucket_num = bucket_num_one/k;
        cur_fp = bloom_fp(k, bucket_num);
        if(fabs(cur_fp - fp) <= PRO * fp){
            res_k = k;
            res_w = bucket_num;
            res_fp = cur_fp;
        }
        else if(cur_fp < fp){
            int low = 1, high = bucket_num, mid;
            while(low < high){
                mid = (low + high )/2;
                cur_fp = bloom_fp(k, mid);
                if(fabs(cur_fp - fp) <= PRO * fp) {
                    res_k = k;
                    res_w = mid;
                    res_fp = cur_fp;
                    break;
                }
                else if(cur_fp < fp)
                    high = mid - 1;
                else
                    low = mid + 1;
            }
        }
        else{
            continue;
        }
        vec.emplace_back(res_k,res_w,res_fp);
    }

    int min_mem = (1<<30), cur_mem;
    for(auto i:vec){
        cur_mem = i.k * i.w;
        if(cur_mem < min_mem){
            min_mem = cur_mem;
            res_k = i.k;
            res_w = i.w;
            res_fp = i.met;
        }
    }

    return make_tuple(res_k,res_w,res_fp);
}

int main() {
    load_data();
    int k , w;
    double res;
    auto beforeTime = chrono::steady_clock::now();
    tie( k , w, res) = search_bf(0.1);
    auto afterTime = chrono::steady_clock::now();
    double duration_millsecond = chrono::duration<double, milli>(afterTime - beforeTime).count();
    cout << duration_millsecond << "ms"<<endl;

    cout<<"Epoch: "<<epoch_sum<<endl;
    cout<<"K: "<<k<<" W: "<<w<<" FP: "<<res<<endl;
    return 0;
}
