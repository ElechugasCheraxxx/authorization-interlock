#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t encryption_key[32];
    uint32_t process_id;
    void* base_address;
    uint8_t stealth_level;
} ShieldContext;

void shield_init(ShieldContext* ctx);
void shield_mem_wipe(void* ptr, size_t len);
int execute_payload_stealth(const uint8_t* artifact, size_t size);

#ifdef __cplusplus
}
#endif
