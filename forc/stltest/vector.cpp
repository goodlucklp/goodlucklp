#include <vector>
#include <iostream>
using namespace std;
int main(int argc, char* argv[]){
    //1.定义和初始化
    vector<int> vec1;                           //默认初始化，vec1为空
    vector<int> vec2(vec1);                     //使用vec1初始化vec2
    vector<int> vec3(vec1.begin(),vec1.end());  //使用vec1初始化vec2
    vector<int> vec4(10);                       //10个值为的元素
    vector<int> vec5(10,4);                     //10个值为的元素

    //2.常用操作方法
    vec1.push_back(100);            //添加元素
    vec1.push_back(101);            //添加元素
    vec1.push_back(102);            //添加元素
    vec1.push_back(103);            //添加元素
    vec1.push_back(104);            //添加元素
    vec1.push_back(105);            //添加元素
    int size = vec1.size();         //元素个数
    bool isEmpty = vec1.empty();    //判断是否为空
    cout<<vec1[0]<<endl;        //取得第一个元素
    cout<<vec1[1]<<endl;        //取得第一个元素
    cout<<vec1[2]<<endl;        //取得第一个元素
    cout<<vec1[3]<<endl;        //取得第一个元素
    cout<<vec1[4]<<endl;        //取得第一个元素
    cout<<vec1[5]<<endl;        //取得第一个元素
    vec1.insert(vec1.end(),5,3);    //从vec1.back位置插入个值为的元素
    //vec1.pop_back();              //删除末尾元素
    //vec1.erase(vec1.begin(),vec1.end());//删除之间的元素，其他元素前移
    cout<<(vec1==vec2)?true:false;  //判断是否相等==、！=、>=、<=...i
    cout<<endl;
    vector<int>::iterator iter = vec1.begin();    //获取迭代器首地址
    vector<int>::const_iterator c_iter = vec1.begin();   //获取const类型迭代器
    //vec1.clear();                 //清空元素

    //3.遍历
    //下标法
    int length = vec1.size();
    for(int i=0;i<length;i++)
    {
       cout<<vec1[i]<<endl;
    }
    cout<<endl<<endl;
    //迭代器法
    vector<int>::const_iterator iterator = vec1.begin();
    for(;iterator != vec1.end();iterator++)
    {
       cout<<*iterator<<endl;
    }
}
