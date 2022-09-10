//#include <cstdio>
//#include <cstddef>
//#include <type_traits>
//#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstdlib>
using namespace std;

class TwoCstor{
public:
	TwoCstor() = default;
	TwoCstor(int i): data(i){}
private:
	int data;
};
class AirportPrice{
private:
	float _dutyfreerate;
public:
	AirportPrice(float rate): _dutyfreerate(rate){}
	float operator() (float price){
		return price * (1 - _dutyfreerate/100);
	}
};
#define MAXFD (64)
void daemon_init(const char* pname,int facility){
	int i;
	pid_t pid;

	if (pid=fork()){
		return;
	}
	setsid();
	signal(SIGHUP,SIG_IGN);
	if(pid=fork()){
		return;
	}
	chdir("/");
	umask(0);
	for (i=0;i<MAXFD;i++){
		close(i);
	}
	openlog(pname,LOG_PID,facility);
}
int main(){
#if 0
	nullptr_t my_null;
	printf("%x\n",&my_null);

	printf("%d\n",my_null == nullptr);

	const nullptr_t && default_nullptr = nullptr;
	printf("%x\n", &default_nullptr);
	cout << is_pod<TwoCstor>::value << endl;
	int a = 3,b = 4;
	auto sum = [](int x,int y)->int { return x + y;} ;
	cout << sum (a,b);
	float tax_rate = 5.5f;
	AirportPrice Changi(tax_rate);

	auto Chang2 = [tax_rate](float price)->float{ return price * (1 - tax_rate/100);};
	float purchased = Changi(3699);
	float purchased2 = Chang2(2899);

	cout << purchased << purchased2 << endl;
	char utf8[] = u8"\u4F60\u597D\u554A";
	char16_t utf16[] = u"hello";
	char32_t utf32[] = U"hello equals \u4F60\u597D\u554A";

	cout << utf8 << " " << utf16 << " " << utf32 << endl; 
	pid_t pid;
	printf("Now only one process\n");
	printf("Calling fork... \n");
	pid = fork();
	if (!pid){
		printf("I am the child\n");
	}
	else if (pid >0){
		printf("I am the parent  has pid %d\n", pid);
	}
	else printf("Fork failed!\n");

	daemon_init("lptest",3);
#endif
    FILE* fp = std::fopen("test.txt", "r");
    if(!fp) {
        std::perror("File opening failed");
        return EXIT_FAILURE;
    }
 
    int c; // 注意：是 int 而非 char ，要求处理 EOF
    while ((c = std::fgetc(fp)) != EOF) { // 标准 C I/O 文件读取循环
       std::putchar(c);
    }
 
    if (std::ferror(fp))
        std::puts("I/O error when reading");
    else if (std::feof(fp))
        std::puts("End of file reached successfully");
 
    std::fclose(fp);
	return 0;
}
