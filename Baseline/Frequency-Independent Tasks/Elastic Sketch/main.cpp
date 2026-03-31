#include <cstdio>
#include <unordered_map>
#include <vector>
#include <cmath>
#include <chrono>

#include "ElasticSketch.h"

using namespace std;

#define THRE 250
#define PRO 0.05

int epoch_sum = 0;

struct FIVE_TUPLE{char key[4];};
typedef vector<FIVE_TUPLE> TRACE;
TRACE traces;

void ReadInTraces()
{
    char datafileName[100];
    sprintf(datafileName, "dataset");
    FILE *fin = fopen(datafileName, "rb");

    FIVE_TUPLE tmp_five_tuple{};
    traces.clear();
    while(fread(&tmp_five_tuple, 1, 4, fin) == 4)
    {
        traces.push_back(tmp_five_tuple);
    }
    fclose(fin);

    printf("Successfully read in %s, %ld packets\n", datafileName, traces.size());
}

double demo_elastic(int BUCKET_NUM, int TOT_MEM_IN_BYTES)
{
    epoch_sum++;

	ElasticSketch *elastic;

    unordered_map<string, int> Real_Freq;
    elastic = new ElasticSketch(BUCKET_NUM, TOT_MEM_IN_BYTES);

    for(int i = 0; i < 25000000; ++i)
    {
        elastic->insert((uint8_t*)(traces[i].key));
        string str((const char*)(traces[i].key), 4);
        Real_Freq[str]++;
    }

    double recall_true = 0, recall = 0;
    for(auto & it : Real_Freq){
        if(it.second >= THRE){
            recall_true += 1;
            uint8_t res[4];
            memcpy(res, it.first.data(), 4);
            if(elastic->query(res) >= THRE)
                recall += 1;
        }
    }

    return recall/recall_true;
}

tuple<int, double> search_elastic(double rr){
    int meminkb = 10 * 1024;
    int bucket_num = meminkb * 1024 * 8 / 4 / 64 / COUNTER_PER_BUCKET;

    int res_w = 0;
    double res_rr = 0, cur_rr;

    cur_rr = demo_elastic(bucket_num, bucket_num * COUNTER_PER_BUCKET * 64 * 4 / 8);
    if(fabs(cur_rr - rr) <= PRO * rr){
        res_w = bucket_num;
        res_rr = cur_rr;
    }
    else if(cur_rr > rr){
        int low = 1, high = bucket_num, mid;
        while(low < high){
            mid = (low + high )/2;
            cur_rr = demo_elastic(mid, mid * COUNTER_PER_BUCKET * 64 * 4 / 8);
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
    ReadInTraces();
    int w;
    double res;
    auto beforeTime = std::chrono::steady_clock::now();
    tie( w, res) = search_elastic(0.8);
    auto afterTime = std::chrono::steady_clock::now();
    double duration_millsecond = std::chrono::duration<double, std::milli>(afterTime - beforeTime).count();
    std::cout << duration_millsecond << "ms" << std::endl;

    cout<<"Epoch: "<<epoch_sum<<endl;
    cout<<"W: "<<w<<" RR: "<<res<<endl;

    return 0;
}