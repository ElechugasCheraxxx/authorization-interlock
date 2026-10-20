#pragma once
#include <string_view>
#include <map>

namespace interlock::embedded {

inline constexpr std::string_view ARSENAL_PAYLOADS = R"python(
import interlock_core_bind as core
import json

class PayloadAgent:
    def __init__(self):
        self.mgr = core.ArtifactManager.get_instance()

    def generate_polymorphic(self, original_code, count=3):
        # La respuesta viene como lista de strings desde C++
        return self.mgr.generate_variants(original_code, count)

    def package_payload(self, code, os_target):
        return self.mgr.wrap_for_os(code, os_target)

    def simple_obfuscate(self, code):
        return self.mgr.obfuscate_strings(code)
)python";

inline const std::map<std::string_view, std::string_view> PAYLOAD_MODULES = {
    {"interlock.arsenal.payloads", ARSENAL_PAYLOADS}
};

} // namespace interlock::embedded
