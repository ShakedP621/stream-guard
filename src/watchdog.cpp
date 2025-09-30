#include "streamguard/watchdog.hpp"

namespace streamguard {
IClock::~IClock() = default; // <-- single, out-of-line definition
} // namespace streamguard
