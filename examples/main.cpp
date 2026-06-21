#include <iostream>
#include "easi/signal.hpp"

int main() {
    class MyClass {
    public:
        MyClass() = default;
        ~MyClass() =  default;
        
        void emit() {
            noargs_sig.emit();
            args_sig.emit(1);
        }

        easi::Signal<MyClass> noargs_sig;
        easi::Signal<MyClass, int> args_sig;
    };

    MyClass my_class;

    auto conn = my_class.noargs_sig.connect([](){
        std::cout << "Connected" << std::endl;
    });

    auto conn2 = my_class.args_sig.connect([](int num) {
        std::cout << "Connected with number: " << num << std::endl;
    });

    my_class.emit();

    conn.disconnect();
    conn2.disconnect();
}