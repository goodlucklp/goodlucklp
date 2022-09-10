#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <thread>
using namespace std;
void thread_task(){
    cout<< "hello thread"<<endl;
}


int main(int argc, char* argv[]){
    thread t(thread_task);
    t.join();

    return EXIT_SUCCESS;
}



