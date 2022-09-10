#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <iostream>
using namespace std;

void sig_catch(int sig);
void interrupt(int sig);
int main(int argc, char** argv){
	signal(SIGINT,interrupt);
	signal(SIGQUIT,interrupt);
	printf("Interrupt set for SIGINT\n");
	sleep(10);
	printf("end\n");
	return 0;
}
void interrupt(int sig){
	cout << "SIG" << sig <<endl;
	puts("Interreupt called\n");
	sleep(3);
	puts("Interreupt Func Ended\n");
}
void sig_catch(int){
	printf("Catch succeed!\n");
	return;
}
