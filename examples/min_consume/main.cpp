#include <iostream>
#include <string>
#include "streamguard/version.hpp"

int main() {
    std::cout << "StreamGuard version: " << streamguard::version() << std::endl;
    return 0;
}
