#include "core/gc_user.hpp"
#include <thread>
int main() {
    std::thread t1([]() {
                     plugin::gc::Alice alice("/Users/fionser/Documents/codes/coda/build//alice.bin");
                     alice.run("jj");
                   });
    std::thread t2([]() {
                     plugin::gc::Bob bob("127.0.0.1", 123, "/Users/fionser/Documents/codes/coda/build/bob.bin");
                     bob.run("jj");
                   });
    t1.join();
    t2.join();
    return 0;
}
