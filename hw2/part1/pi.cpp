#include <iostream>
#include <random>
#include <atomic>
#include <stdlib.h>
#include <pthread.h>

using namespace std;

pthread_mutex_t MUTEX = PTHREAD_MUTEX_INITIALIZER;
atomic<long long> IN_CIRCLE;

typedef struct
{
    long long toss;
    uint32_t seed;
} Arg;

void *child(void *arg)
{
    long long toss_num = ((Arg *)arg)->toss;
    long long in_circle = 0;

    mt19937 generator(((Arg *)arg)->seed);
    uniform_real_distribution<float> unif(0.0, 1.0);

    for (long long i = 0; i < toss_num; i++)
    {
        float x = unif(generator);
        float y = unif(generator);
        if (x * x + y * y <= 1)
            in_circle++;
    }

    IN_CIRCLE += in_circle;
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    // string to long and long long
    int core_num = atoi(argv[1]);
    long long toss_num = atoll(argv[2]);

    pthread_t *cores = new pthread_t[core_num];
    Arg *args = new Arg[core_num];
    random_device rd;

    for (int i = 0; i < core_num; i++)
    {
        args[i].seed = rd();
        args[i].toss = toss_num / core_num + (toss_num % core_num > i ? 1 : 0);
    }

    for (int i = 0; i < core_num; i++)
    {
        pthread_create(&cores[i], NULL, child, (void *)&args[i]);
    }
    for (int i = 0; i < core_num; i++)
    {
        pthread_join(cores[i], NULL);
    }
    cout << 4 * IN_CIRCLE / (double)toss_num << endl;
    return 0;
}