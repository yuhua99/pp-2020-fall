#include <iostream>
#include <random>
int main(){
    std::random_device rd;

    /* 梅森旋轉演算法 */
    std::mt19937 generator( rd() );

    std::uniform_real_distribution<float> unif(0.0, 1.0);
    float x = unif(generator);
    std::cout << "x = " << x << std::endl;
    return 0;
}