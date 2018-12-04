#include <algorithm>
#include <thread>
#include <vector>
#include <stdio.h>
#include "gtest/gtest.h"

//int main(int argc, char *argv[]) {
//    std::vector<int> nums{3,4,1,2,2,8,9,10};
//    std::sort(nums.begin(), nums.end(), [](auto a, auto b) {return a < b;});
//    
//    printf("%d\n", nums[0]);
//    return 0;
//}

//TEST(LambdaTest, Sanity) {
//    for (int i = 0; i < 10; i++) {
//        auto x = new int[128];
//
//        printf("%d\n", *x);
//        //delete x;
//        
//        auto y = static_cast<int *>(malloc(100));
//        printf("%d\n", *y);
//        
//        std::this_thread::sleep_for(std::chrono::seconds(1));
//    }
//}
