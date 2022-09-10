#include <type_traits>
#include <iostream>

using namespace std;
class TwoCstor{
public:
    TwoCstor() = default;
    TwoCstor(int i): data(i){}
private:
    int data;
};


int main(){
    cout << is_pod<TwoCstor>::value << endl;
    return 0;
}
