#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <unordered_map>
#include <string>

using namespace std;

#define MAX_INSERT_PACKAGE 25000000
#define PRO 0.05

vector<pair<uint32_t, int>> items;

int epoch_sum = 0;
int packet_num = 0;

vector<int> dist;
int max_counter_val = 0;

void load_data() {
    unordered_map<uint32_t, int> ground_truth;
    ground_truth.clear();

    const char *filename="dataset";
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

    for (auto itr : ground_truth)
        max_counter_val = max(max_counter_val, itr.second);
    dist.resize(max_counter_val + 1);
    fill(dist.begin(), dist.end(), 0);
    for (auto itr : ground_truth)
        dist[itr.second]++;
}

double mrac_wmre(int w){

    epoch_sum++;

    random_device rd;
    mt19937 generator(rd());
    poisson_distribution<int> poissonDist(1.0 * items.size()/w);
    uniform_int_distribution<int> uniformDist(0, items.size() - 1);

    double wmre = 0;
    int num = 0, epoch = 1000;

    vector<int> dist_est;
    int max_counter_val_est = 0;

    while(true){
        for(int epo=0; epo<epoch; epo++){
            int numSamples = poissonDist(generator);

            int sum = 0;
            for (int j = 0; j < numSamples; ++j) {
                int index = uniformDist(generator);
                sum += items[index].second;
            }

            if (sum >= dist_est.size()){
                max_counter_val_est = sum;
                dist_est.resize(max_counter_val_est + 1);
            }

            dist_est[sum]++;
        }
        num += epoch;

        double mul = 1.0 * w / num;
        double wmre_1 = 0, wmre_2 = 0;
        for (int i = 1; i <= max_counter_val_est; i++){
            if (i <= max_counter_val){
                wmre_1 += fabs(dist[i] - dist_est[i] * mul);
                wmre_2 += double(dist[i] + dist_est[i] * mul) / 2;
            }
            else{
                wmre_1 += dist_est[i] * mul;
                wmre_2 += double(dist_est[i] * mul) / 2;
            }
        }
        for (int i = max_counter_val_est+1; i <= max_counter_val; i++){
            wmre_1 += dist[i];
            wmre_2 += double(dist[i]) / 2;
        }
        double new_wmre = wmre_1 / wmre_2;
        if(fabs(new_wmre - wmre) < 0.0001 * wmre)
            break;
        else
            wmre = new_wmre;
    }
    return wmre;
}

// binary search
tuple<int, double>  search(double wmre){
    int meminkb = 10 * 1024;
    int bucket_num = meminkb * 1024 * 8 / 32;

    int res_w = 0;
    double res_wmre = 0, cur_wmre;

    cur_wmre = mrac_wmre( bucket_num);
    if(fabs(cur_wmre - wmre) <= PRO * wmre){
        res_w = bucket_num;
        res_wmre = cur_wmre;
    }
    else if(cur_wmre < wmre){
        int low = 1, high = bucket_num, mid;
        while(low < high){
            mid = (low + high )/2;
            cur_wmre = mrac_wmre(mid);
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

int main() {
    load_data();
    int w;
    double res;
    auto beforeTime = chrono::steady_clock::now();
    tie( w, res) = search(1);
    auto afterTime = chrono::steady_clock::now();
    double duration_millsecond = chrono::duration<double, milli>(afterTime - beforeTime).count();
    cout << duration_millsecond << "ms"<<endl;

    cout<<"Epoch: "<<epoch_sum<<endl;
    cout<<"W: "<<w<<" WMRE: "<<res<<endl;

    return 0;
}