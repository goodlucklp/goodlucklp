#include <iostream>
#include <cstdlib>
using namespace std;
int main(){
    int boys = 4; 
    int girls = 3;
    auto ret =  [&girls, &boys](int i,int j)->int {
        cout << (i + j) <<endl;
        return girls + boys;};

    auto c = ret(5,6);
    return 0;
}
