#include <iostream>
#include <random>

using namespace std;
int main()
{
    random_device rd;
    /* 梅森旋轉演算法 */
    mt19937 generator(rd());
    uniform_real_distribution<float> unif(0.0, 1.0);

    long long in_circle_num = 0, toss_num;
    cin >> toss_num;

    for (long long toss = 0; toss < toss_num; toss++)
    {
        float x = unif(generator);
        float y = unif(generator);
        if (x * x + y * y <= 1)
            in_circle_num++;
    }
    cout << 4 * in_circle_num / (double)toss_num << endl;
    return 0;
}

// number_in_circle = 0;
// for ( toss = 0; toss < number_of_tosses; toss ++) {
//     x = random double between -1 and 1;
//     y = random double between -1 and 1;
//     distance_squared = x * x + y * y;
//     if ( distance_squared <= 1)
//         number_in_circle++;
// }
// pi_estimate = 4 * number_in_circle /(( double ) number_of_tosses);