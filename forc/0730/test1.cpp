#include <iostream>
using namespace std;

class HasPtrMem {
public:
    HasPtrMem() : d(new int(1))
    {
        cout<< "构造" << ++n_cstr << endl;
    }
    HasPtrMem(const HasPtrMem& h) : d(new int(*h.d))
    {
        cout<< "拷贝构造" << ++n_cptr << endl;
    }
    HasPtrMem(HasPtrMem&& h) : d(h.d)
    {
        cout<< "移动构造" << ++n_mvtr << *h.d << endl;
	h.d = nullptr;

    }
    ~HasPtrMem()
    {   
	delete d;
        cout<< "析构" << ++n_dstr << endl;
    }


    int* d {nullptr};
    static int n_cstr;
    static int n_dstr;
    static int n_cptr;
    static int n_mvtr;

};

int HasPtrMem::n_cstr = 0;
int HasPtrMem::n_mvtr = 0;
int HasPtrMem::n_dstr = 0;
int HasPtrMem::n_cptr = 0;

HasPtrMem GetTmp() 
{
    return HasPtrMem();
}

void IrunCodeActually(int && a ) {
    cout<< "IrunCodeActually called 1" << endl;
}

void IrunCodeActually(const int && a ) {
    cout<< "IrunCodeActually called 2" << endl;
}

void IrunCodeActually(int & a ) {
    cout<< "IrunCodeActually called 3" << endl;
}

void IrunCodeActually(const int & a ) {
    cout<< "IrunCodeActually called 4" << endl;
}
// wan mei zhuan fa 
template <typename T> 
    void IamForwording(T &&t) {
        IrunCodeActually(forward<T>(t));
    }

struct Copyable {
    Copyable() 
    { 
        cout <<"gouzao"<<endl;
    }
    Copyable(const Copyable &o)
    {
	cout << "Copied" << endl;
    }
};

Copyable Returnvalue()
{
    return Copyable();
}

void AcceptVal(Copyable)
{
    cout << "called AcceptVal" <<endl;
}

void AcceptRef(const Copyable& ) 
{
    cout << "called AcceptRef" <<endl;
}


int main ()
{
    int a, b;
    const int c = 1;
    const int d = 0;
    IamForwording(a);
    IamForwording(move(b));
    IamForwording(c);
    IamForwording(move(d));

    AcceptVal(Returnvalue());
    AcceptRef(Returnvalue());
    // HasPtrMem a = GetTmp();
    // HasPtrMem b = HasPtrMem(a);
    // HasPtrMem c = HasPtrMem(move(b));
}
