#import "NetworkBridge.hpp"
#import <Network/Network.h>
#import <Foundation/Foundation.h>
#include <iostream>

namespace interlock {

NetworkBridge& NetworkBridge::getInstance() {
    static NetworkBridge instance;
    return instance;
}

void NetworkBridge::initialize() {
    NSLog(@"[NetworkBridge] Initializing Integrity Network Stack via Network.framework");
    // Initialization of Network.framework components if needed
}

void NetworkBridge::shutdown() {
    NSLog(@"[NetworkBridge] Shutting down network stack");
}

bool NetworkBridge::transmitStealthy(const std::string& destination, const std::vector<uint8_t>& data) {
    @autoreleasepool {
        __block bool success = false;
        dispatch_semaphore_t sema = dispatch_semaphore_create(0);

        NSString* nsDest = [NSString stringWithUTF8String:destination.c_str()];
        NSArray* parts = [nsDest componentsSeparatedByString:@":"];
        if ([parts count] < 2) return false;

        const char* host = [[parts objectAtIndex:0] UTF8String];
        const char* port = [[parts objectAtIndex:1] UTF8String];

        nw_endpoint_t endpoint = nw_endpoint_create_host(host, port);
        nw_parameters_t parameters = nw_parameters_create_secure_tcp(
            NW_PARAMETERS_DEFAULT_CONFIGURATION,
            NW_PARAMETERS_DEFAULT_CONFIGURATION
        );

        // Integrity configuration: Avoid common fingerprints
        nw_connection_t connection = nw_connection_create(endpoint, parameters);

        nw_connection_set_queue(connection, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0));

        nw_connection_set_state_changed_handler(connection, ^(nw_connection_state_t state, nw_error_t error) {
            if (state == nw_connection_state_ready) {
                dispatch_data_t dispatchData = dispatch_data_create(data.data(), data.size(), nil, DISPATCH_DATA_DESTRUCTOR_DEFAULT);
                nw_connection_send(connection, dispatchData, NW_CONNECTION_DEFAULT_MESSAGE_CONTEXT, true, ^(nw_error_t sendError) {
                    if (sendError == NULL) {
                        success = true;
                    }
                    nw_connection_cancel(connection);
                    dispatch_semaphore_signal(sema);
                });
            } else if (state == nw_connection_state_failed || state == nw_connection_state_cancelled) {
                dispatch_semaphore_signal(sema);
            }
        });

        nw_connection_start(connection);
        dispatch_semaphore_wait(sema, dispatch_time(DISPATCH_TIME_NOW, 5 * NSEC_PER_SEC));

        if (success) {
            stats.bytesSent += data.size();
        }

        return success;
    }
}

NetworkStats NetworkBridge::getStats() const {
    return stats;
}

} // namespace interlock
