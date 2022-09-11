#include <map>
#include <iostream>
using namespace std;
int main(int argc,char* argv[]){
//1.定义和初始化
    map<int,string> map1;                  //空map
    //2.常用操作方法
    map1[3] = "Saniya";                    //添加元素
    map1.insert(map<int,string>::value_type(2,"Diyabi"));//插入元素
    map1.insert(make_pair<int,string>(4,"V5"));
    string str = map1[3];                  //根据key取得value，key不能修改
    map<int,string>::iterator iter_map = map1.begin();//取得迭代器首地址
    int key = iter_map->first;             //取得eky
    string value = iter_map->second;       //取得value
    map1.size();                           //元素个数
    map1.empty();                          //判断空

    //3.遍历
    for(map<int,string>::iterator iter = map1.begin();iter!=map1.end();iter++)
    {
       int keyk = iter->first;
       string valuev = iter->second;
       cout<<valuev<<endl;
    }
}
