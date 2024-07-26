#include <iostream>
#include <memory>

class Foo{

    public:
        int var;

        Foo(int a){
            var = a;
            std::cout << "constructor " << var << "\n";
        }
        virtual ~Foo(){
            std::cout << "destructor " << var << "\n";
        }
};

class Foo1:public Foo{

    public:

        Foo1(int var):Foo(var){
            
            std::cout << "derived constructor " << var << "\n";
        }
        ~Foo1() override{
            std::cout << "derived destructor " << var << "\n";
        }
};


void function(){
    std::unique_ptr<Foo> foo;
    foo = std::make_unique<Foo>(1);
    foo = std::make_unique<Foo1>(2);
    foo = std::make_unique<Foo>(3);
}

int main(){
    function();

    return 0;
}