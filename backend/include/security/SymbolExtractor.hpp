#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace interlock::security {

struct Symbol {
    uint32_t rva = 0;
    uint32_t size = 0;
    std::string name;
    std::string section;
    std::string source_file;
};

/**
 * @brief Minimal native symbol extractor ported from referencia previa SymbolExtract.
 * Handles BSD and SYSV symbol formats commonly found in 'nm' or 'objdump' outputs.
 */
class SymbolExtractor {
public:
    static std::vector<Symbol> parseBsdOutput(const std::string& output);
    static std::vector<Symbol> parseSysvOutput(const std::string& output);

private:
    static bool parseBsdLine(const std::string& line, Symbol& out_symbol);
    static bool parseSysvLine(const std::string& line, Symbol& out_symbol);
};

} // namespace interlock::security
