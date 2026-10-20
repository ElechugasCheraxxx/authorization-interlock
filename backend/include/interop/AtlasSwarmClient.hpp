#pragma once
#include <grpcpp/grpcpp.h>
#include "../../core-cpp/pb/atlas.grpc.pb.h"
#include "../security/Harden.hpp"
#include <memory>
#include <string>
#include <iostream>
#include <ctime>

namespace interlock::network {

namespace pb = interlock::atlas;

class AtlasSwarmClient {
public:
    AtlasSwarmClient(const std::string& socket_path) {
        std::string target = "unix:" + socket_path;
        auto channel = grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
        stub_ = pb::SwarmOrchestrator::NewStub(channel);
    }

    bool register_agent(const std::string& agent_id, const std::string& os, const std::string& arch) {
        pb::AgentInfo request;
        request.set_agent_id(agent_id);
        request.set_target_os(os);
        request.set_arch(arch);
        request.set_connection_type("GRPC_UDS");

        pb::RegistrationResponse response;
        grpc::ClientContext context;

        grpc::Status status = stub_->RegisterAgent(&context, request, &response);

        if (status.ok()) {
            if (response.success()) {
                std::cout << "[AtlasClient] Agent " << agent_id << " registered. Token: " << response.session_token() << std::endl;
                return true;
            } else {
                std::cerr << "[AtlasClient] Registration failed: " << response.error_message() << std::endl;
            }
        } else {
            std::cerr << "[AtlasClient] gRPC Error: " << status.error_message() << std::endl;
        }
        return false;
    }

    /**
     * @brief Routes a message through the Atlas Swarm.
     * Used to send REASONING_OUTPUT from Cipher/Vantage to the Go-native parser.
     */
    bool route_message(const std::string& from, const std::string& to, const std::string& subject, const std::string& artifact) {
        pb::Message msg;
        msg.set_from(from);
        msg.set_to(to);
        msg.set_subject(subject);
        msg.set_payload(artifact);

        // Security Hardening: SHA-256 Integrity Verification (Ported from referencia previa TunablesVerifier)
        // We include a hash of the artifact to ensure it hasn't been tampered with in transit/IPC.
        std::string payload_hash = security::IntegrityVerifier::calculateHash(artifact.c_str(), artifact.size());
        // For now, we print it. In a real scenario, we'd add this to the gRPC metadata or a specific field.
        std::cout << "[AtlasClient] Artifact Integrity Hash: " << payload_hash << std::endl;

        // Use current timestamp if needed (uint64)
        msg.set_timestamp(static_cast<uint64_t>(std::time(nullptr)));

        pb::RouteResponse response;
        grpc::ClientContext context;

        grpc::Status status = stub_->RouteMessage(&context, msg, &response);

        if (status.ok()) {
            if (response.delivered()) {
                return true;
            } else {
                std::cerr << "[AtlasClient] Message delivery failed: " << response.error() << std::endl;
            }
        } else {
            std::cerr << "[AtlasClient] gRPC Error: " << status.error_message() << std::endl;
        }
        return false;
    }

private:
    std::unique_ptr<pb::SwarmOrchestrator::Stub> stub_;
};

} // namespace interlock::network
