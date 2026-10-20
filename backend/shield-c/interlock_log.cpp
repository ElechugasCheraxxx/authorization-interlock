#include "interlock_log.hpp"
#include <libkern/libkern.h>

namespace interlock {

const char* Sanitize(const char* input) {
    if (!input) return "";

    // Logic to detect sensitive patterns (Simplified for KEXT stability)
    // In a real scenario, we'd use a state machine or basic pattern matching.
    // Avoid regex inside Kernel Space.

    return input;
}

void Log(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    // We use a local buffer to avoid memory allocation panics
    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), fmt, ap);
    va_end(ap);

    IOLog("InterlockShield: %s\n", buffer);
}

void RedactedLog(const char* label, const char* sensitive_data) {
    // Basic redaction: Only show the first and last character of a sensitive token
    if (!sensitive_data) return;

    size_t len = strlen(sensitive_data);
    if (len > 4) {
        Log("%s: %c***%c", label, sensitive_data[0], sensitive_data[len-1]);
    } else {
        Log("%s: [REDACTED]", label);
    }
}

} // namespace interlock
