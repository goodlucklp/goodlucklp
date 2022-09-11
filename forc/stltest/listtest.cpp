#include <list>
#include <iostream>
using namespace std;

int main(int argc,char* argv[]){
//1.定义和初始化
    list<int> lst1;          //创建空list
    list<int> lst2(3);       //创建含有三个元素的list
    list<int> lst3(3,2); //创建含有三个元素的list
    list<int> lst4(lst2);    //使用lst2初始化lst4
    list<int> lst5(lst2.begin(),lst2.end());  //同lst4

    //2.常用操作方法
    lst1.assign(lst2.begin(),lst2.end());  //分配值
    lst1.push_back(10);                    //添加值
    lst1.push_back(11);                    //添加值
    lst1.push_back(12);                    //添加值
    lst1.push_back(13);                    //添加值
    lst1.push_back(14);                    //添加值
    lst1.pop_back();                       //删除末尾值
    lst1.begin();                          //返回首值的迭代器
    lst1.end();                            //返回尾值的迭代器
    lst1.clear();                          //清空值
    auto isEmpty1 = lst1.empty();          //判断为空
    cout<< isEmpty1 << endl;
    lst1.erase(lst1.begin(),lst1.end());   //删除元素
    lst1.front();                          //返回第一个元素的引用
    lst1.back();                           //返回最后一个元素的引用
    lst1.insert(lst1.begin(),3,2);         //从指定位置插入个
    lst1.rbegin();                         //返回第一个元素的前向指针
    lst1.remove(2);                        //相同的元素全部删除
    lst1.reverse();                        //反转
    lst1.size();                           //含有元素个数
    lst1.sort();                           //排序
    lst1.unique();                         //删除相邻重复元素

    //3.遍历
    //迭代器法
    for(list<int>::const_iterator iter = lst1.begin();iter != lst1.end();iter++)
    {
       cout<<*iter<<endl;
    }
    cout<<endl;
}
