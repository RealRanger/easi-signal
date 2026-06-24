#include <iostream>
#include "easi/signal.hpp"
#include <chrono>
#include <thread>
#include <string>

int main() {
    namespace easi_signal = easi::signal;

    class MyClass {
    public:
        MyClass() = default;
        ~MyClass() =  default;
        
        void emit() {
            noargs_sig.emit();
        }

        easi_signal::Signal<easi_signal::unique_own<MyClass>> noargs_sig;
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

    
    class Timer {
    public:
        Timer() : start_time(), end_time() {}

        void start() {
            start_time = std::chrono::high_resolution_clock::now();
        }

        void stop() {
            end_time = std::chrono::high_resolution_clock::now();
        }

        double elapsed_ms() {
            return std::chrono::duration<double, std::milli>(end_time - start_time).count();
        }

    private:
        std::chrono::high_resolution_clock::time_point start_time;
        std::chrono::high_resolution_clock::time_point end_time;
    };


    auto timer = Timer();
    timer.start();

    for (int i = 0; i < 1'000'000; i++) {
        auto conn = my_class.noargs_sig.connect([]() {
            volatile int result = 1;
            for (int j = 1; j < 100; j++) {
                result = (result * j) % 97;
            }
        });
    }
    timer.stop();

    std::cout << "Connection took " << std::to_string(timer.elapsed_ms() / 1000) << " seconds." << std::endl;

    auto timer_2 = Timer();
    timer_2.start();
    my_class.emit();
    timer_2.stop();
    std::cout << "Emission took " << std::to_string(timer_2.elapsed_ms() / 1000) << " seconds." << std::endl;
}