#ifndef INTERLOCK_LOG_HPP
#define INTERLOCK_LOG_HPP

#include <IOKit/IOLib.h>

namespace interlock {
    // Standard sanitized log
    void Log(const char* fmt, ...);

    // Redacts sensitive info like IPs or Keys
    void RedactedLog(const char* label, const char* sensitive_data);

    // Internal helper to check for sensitive patterns
    const char* Sanitize(const char* input);
}

#endif
