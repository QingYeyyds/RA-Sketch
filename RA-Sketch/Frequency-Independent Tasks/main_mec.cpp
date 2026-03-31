#include <iostream>
#include <random>
#include <chrono>
#include <unordered_map>
#include <unordered_set>

using namespace std;

#define PRO 0.05
#define MAX_INSERT_PACKAGE 25000000
#define THRE 250

int HEAVY_FLOWS = 0;

int epoch_sum = 0;

void load_data() {
    unordered_map<uint32_t, unordered_set<uint32_t>> ground_truth;
    ground_truth.clear();

    const char* filename = "dataset";
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

    for(const auto& i:ground_truth)
        if(i.second.size() >= THRE)
            HEAVY_FLOWS++;

    cout<<ret<<" "<<HEAVY_FLOWS<<endl;
}

double mec_p_rr(int k, int w){
    epoch_sum++;
    double lambda = 1.0 * HEAVY_FLOWS / w;
    double rr = exp(-lambda) * lambda;
    rr /= lambda;
    rr = 1 - pow(1 - rr, k);
    return rr;
}

double mec_m_rr(int w){
    epoch_sum++;
    double lambda = 1.0 * HEAVY_FLOWS / w;
    double rr = 0, pro = 1 - exp(-lambda);
    double temp;
    for (int i = 1; i <= 7; i++){
        temp = exp(-lambda) * pow(lambda, i) / tgamma(i + 1);
        rr += i * temp;
        pro -= temp;
    }
    rr += 6 * pro;
    rr /= lambda;
    return rr;
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

tuple<int, int, double> search_mec_p(double rr){
    vector<Parm> vec;

    int res_k = 0, res_w = 0;
    double res_rr = 0, cur_rr;

    for(int k=1; k<=3; k++){
        int bucket_num = HEAVY_FLOWS * (1- pow(1-rr,1.0/k));
        cur_rr = mec_p_rr(k, bucket_num);
        if(fabs(cur_rr - rr) <= PRO * rr){
            res_k = k;
            res_w = bucket_num;
            res_rr = cur_rr;
        }
        else if(cur_rr < rr){
            for(int w=bucket_num*2; ; w*=2){
                cur_rr = mec_p_rr(k, w);
                if(fabs(cur_rr - rr) <= PRO * rr){
                    res_k = k;
                    res_w = w;
                    res_rr = cur_rr;
                    break;
                }
                else if(cur_rr > rr){
                    int low = w/2, high = w, mid;
                    while(low < high){
                        mid = (low + high )/2;
                        cur_rr = mec_p_rr(k, mid);
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
                    break;
                }
            }
        }
        else{
            for(int w=bucket_num/2; w>0; w/=2){
                cur_rr = mec_p_rr(k, w);
                if(fabs(cur_rr - rr) <= PRO * rr){
                    res_k = k;
                    res_w = w;
                    res_rr = cur_rr;
                    break;
                }
                else if(cur_rr < rr) {
                    int low = w, high = w*2, mid;
                    while (low < high){
                        mid = (low + high) / 2;
                        cur_rr = mec_p_rr(k, mid);
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
                    break;
                }
            }
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

tuple<int, double> search_mec_m(double rr){
    int bucket_num = HEAVY_FLOWS * rr / 7;

    int res_w = 0;
    double res_rr = 0, cur_rr;

    cur_rr = mec_m_rr( bucket_num);
    if(fabs(cur_rr - rr) <= PRO * rr){
        res_w = bucket_num;
        res_rr = cur_rr;
    }
    else if(cur_rr < rr){
        for(int w=bucket_num*2; ; w*=2){
            cur_rr = mec_m_rr(w);
            if(fabs(cur_rr - rr) <= PRO * rr){
                res_w = w;
                res_rr = cur_rr;
                break;
            }
            else if(cur_rr > rr){
                int low = w/2, high = w, mid;
                while(low < high){
                    mid = (low + high )/2;
                    cur_rr = mec_m_rr(mid);
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
                break;
            }
        }
    }
    else{
        for(int w=bucket_num/2; w>0; w/=2){
            cur_rr = mec_m_rr(w);
            if(fabs(cur_rr - rr) <= PRO * rr){
                res_w = w;
                res_rr = cur_rr;
                break;
            }
            else if(cur_rr < rr) {
                int low = w, high = w*2, mid;
                while (low < high){
                    mid = (low + high) / 2;
                    cur_rr = mec_m_rr(mid);
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
                break;
            }
        }
    }

    return make_tuple(res_w,res_rr);
}

int main() {
    load_data();
    int k , w;
    double res;
    auto beforeTime = chrono::steady_clock::now();
    tie( k, w, res) = search_mec_p(0.9);
//    tie( w, res) = search_mec_m(0.9);
    auto afterTime = chrono::steady_clock::now();
    double duration_millsecond = chrono::duration<double, milli>(afterTime - beforeTime).count();
    cout << duration_millsecond << "ms"<<endl;

    cout<<"Epoch: "<<epoch_sum<<endl;

    cout<<"K: "<<k<<" W: "<<w<<" RR: "<<res<<endl;
//    cout<<"W: "<<w<<" RR: "<<res<<endl;
    return 0;
}
