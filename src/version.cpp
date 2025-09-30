#include "streamguard/version.hpp"
#include <string>

namespace streamguard {

std::string version() {
    return std::to_string(kVersionMajor) + "." +
           std::to_string(kVersionMinor) + "." +
           std::to_string(kVersionPatch);
}

}  // namespace streamguard
