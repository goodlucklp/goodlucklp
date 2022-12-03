#include <iostream>
#include <chrono>
 
int main()
{
 
    std::chrono::milliseconds ms = std::chrono::duration_cast< std::chrono::milliseconds >(
        std::chrono::system_clock::now().time_since_epoch()
    );
 
    std::cout << ms.count() << std::endl;
    return 0;
}

