#include <iostream>
#include "easi/signal.hpp"
#include <chrono>
#include <thread>

int main() {
    class MyClass {
    public:
        MyClass() = default;
        ~MyClass() =  default;
        
        void emit() {
            noargs_sig.emit();
            args_sig.emit(1);
        }

        easi::signal::Signal<MyClass> noargs_sig;
        easi::signal::Signal<MyClass, int> args_sig;
    };

    MyClass my_class;
    /*
    auto conn = my_class.noargs_sig.connect([](){
        std::cout << "Connected" << std::endl;
    });

    auto conn2 = my_class.args_sig.connect([](int num) {
        std::cout << "Connected with number: " << num << std::endl;
    });

    my_class.emit();

    conn.disconnect();
    conn2.disconnect();
    */

    for (int i = 0; i < 10; i++) {
        auto conn = my_class.noargs_sig.connect([]() {
            std::cout << "Connected" << std::endl;
        });

        my_class.emit();

        conn.disconnect();

        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
}