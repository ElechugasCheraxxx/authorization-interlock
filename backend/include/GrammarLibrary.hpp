#pragma once
#include <string>

namespace interlock {

/**
 * GBNF Grammars for forcing LLM output structure.
 * Optimized for Assessment Agents (Royal Family) on M4 Pro.
 */
struct GrammarLibrary {
    // Default fallback grammar
    static inline const std::string ASSESSMENT_ACTION_XML = R"gbnf(
root ::= thought action
thought ::= "<thought>" [^<]* "</thought>"
action ::= "<action type=\"" action_type "\">" action_body "</action>"
action_type ::= "command" | "assess" | "pivot" | "report" | "wait"
action_body ::= [^<]*
)gbnf";

    // Coordinator: Especialista en Exploitation
    static inline const std::string REX_GBNF = R"gbnf(
root ::= thought artifact execution
thought ::= "<thought>" [^<]* "</thought>"
artifact ::= "<artifact type=\"" p_type "\">" [^<]* "</artifact>"
execution ::= "<execution method=\"" e_method "\">" [^<]* "</execution>"
p_type ::= "shellcode" | "binary" | "script" | "memory_injection"
e_method ::= "pipe" | "process_hollowing" | "direct_exec"
)gbnf";

    // Vantage: Inteligencia de Red y Recon
    static inline const std::string VANTAGE_GBNF = R"gbnf(
root ::= thought recon results
thought ::= "<thought>" [^<]* "</thought>"
recon ::= "<recon target=\"" [^"]* "\">" [^<]* "</recon>"
results ::= "<results format=\"" r_format "\">" [^<]* "</results>"
r_format ::= "xml" | "json" | "table" | "raw"
)gbnf";

    // Atlas: Generación de Código Nativo
    static inline const std::string ATLAS_GBNF = R"gbnf(
root ::= thought code filepath
thought ::= "<thought>" [^<]* "</thought>"
code ::= "<code language=\"" lang "\">" [^<]* "</code>"
filepath ::= "<filepath>" [^<]* "</filepath>"
lang ::= "cpp" | "swift" | "python" | "ruby" | "asm"
)gbnf";

    // Chronos: Persistencia y Sigilo
    static inline const std::string CHRONOS_GBNF = R"gbnf(
root ::= thought persistence timing
thought ::= "<thought>" [^<]* "</thought>"
persistence ::= "<persistence method=\"" m_method "\">" [^<]* "</persistence>"
timing ::= "<timing interval=\"" [^"]* "\">" [^<]* "</timing>"
m_method ::= "launchd" | "cron" | "kernel_callback" | "dylib_hijack"
)gbnf";

    // Cipher: Orquestación y Respuesta al Usuario
    static inline const std::string CIPHER_GBNF = R"gbnf(
root ::= thought response delegation
thought ::= "<thought>" [^<]* "</thought>"
response ::= "<response>" [^<]* "</response>"
delegation ::= ("<delegate to=\"" agent "\">" [^<]* "</delegate>")?
agent ::= "rex" | "vantage" | "atlas" | "chronos" | "apex" | "wraith" | "archive"
)gbnf";

    // Apex: Coordinador de Enjambre (JSON Estratégico)
    static inline const std::string APEX_GBNF = R"gbnf(
root ::= "{" space "\"strategy\":" space string "," space "\"tasks\":" space array "}"
value  ::= object | array | string | number | ("true" | "false" | "null")
object ::= "{" space ( string ":" space value ("," space string ":" space value)* )? "}"
array  ::= "[" space ( value ("," space value)* )? "]"
string ::= "\"" ( [^"\\\x00-\x1F] | "\\" (["\\/bfnrt] | "u" [0-9a-fA-F]{4}) )* "\""
number ::= ("-"? ([0-9] | [1-9] [0-9]*)) ("." [0-9]+)? ([eE] [+-]? [0-9]+)?
space  ::= " "?
)gbnf";

    // Overseer: OPSEC y Cumplimiento
    static inline const std::string OVERSEER_GBNF = R"gbnf(
root ::= thought audit risk
thought ::= "<thought>" [^<]* "</thought>"
audit ::= "<audit result=\"" verdict "\">" [^<]* "</audit>"
risk ::= "<risk_level>" [0-9] "</risk_level>"
verdict ::= "PASS" | "FAIL" | "WARNING"
)gbnf";

    // Grader: Evaluación y Calidad
    static inline const std::string GRADER_GBNF = R"gbnf(
root ::= thought score feedback
thought ::= "<thought>" [^<]* "</thought>"
score ::= "<score>" [0-9]+ "</score>"
feedback ::= "<feedback>" [^<]* "</feedback>"
)gbnf";

    // Minimizer: Evasión y Anti-Forensics
    static inline const std::string WRAITH_GBNF = R"gbnf(
root ::= thought evasion cleanup
thought ::= "<thought>" [^<]* "</thought>"
evasion ::= "<evasion type=\"" e_type "\">" [^<]* "</evasion>"
cleanup ::= "<cleanup target=\"" c_target "\">" [^<]* "</cleanup>"
e_type ::= "log_spoof" | "memory_wipe" | "shadow_exec"
c_target ::= "syslog" | "sh_history" | "audit_pipe" | "all"
)gbnf";

    // Archive: Memoria Vectorial y Recuperación
    static inline const std::string ARCHIVE_GBNF = R"gbnf(
root ::= thought query metadata
thought ::= "<thought>" [^<]* "</thought>"
query ::= "<query>" [^<]* "</query>"
metadata ::= "<metadata>" [^<]* "</metadata>"
)gbnf";

    // Beacon: Telemetría y Latido
    static inline const std::string BEACON_GBNF = R"gbnf(
root ::= status telemetry
status ::= "<status>" [^<]* "</status>"
telemetry ::= "<telemetry>" [^<]* "</telemetry>"
)gbnf";
};

} // namespace interlock
