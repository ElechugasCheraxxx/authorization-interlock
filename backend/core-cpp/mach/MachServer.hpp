#pragma once
#include <mach/mach.h>

namespace interlock::mach {
    bool StartMachServer();
    mach_port_t GetServerPort();
}
