#include <cstdio>
#include <iostream>
using namespace std;

int main(int argc,char* argv[]){
    unsigned int a = 16;
    signed int b = -2;
    static  int test,test1;
    auto c = a+b;
    cout << "c is" << c<< endl;
    char u8string[] =u8R"(你好)""=hello";
    auto f = test++;
    auto h = ++test1;
    cout<<u8string<<endl; //输出"你好=hello"

    cout<<sizeof(u8string)<<endl; //15
    
    cout << "f is "<< f << endl;
    cout << "test is "<< test << endl;

    cout << "h is "<< h << endl;
    cout << "test1 is " << test1 <<endl;
    auto f1 = [](int i)-> void {cout << i<<endl;};  

    f1(3);


    return 0;
}
