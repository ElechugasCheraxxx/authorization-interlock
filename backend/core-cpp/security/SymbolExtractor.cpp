#include "security/SymbolExtractor.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <regex>

namespace interlock::security {

std::vector<Symbol> SymbolExtractor::parseBsdOutput(const std::string& output) {
    std::vector<Symbol> symbols;
    std::istringstream stream(output);
    std::string line;

    while (std::getline(stream, line)) {
        Symbol s;
        if (parseBsdLine(line, s)) {
            symbols.push_back(s);
        }
    }
    return symbols;
}

std::vector<Symbol> SymbolExtractor::parseSysvOutput(const std::string& output) {
    std::vector<Symbol> symbols;
    std::istringstream stream(output);
    std::string line;

    while (std::getline(stream, line)) {
        Symbol s;
        if (parseSysvLine(line, s)) {
            symbols.push_back(s);
        }
    }
    return symbols;
}

bool SymbolExtractor::parseBsdLine(const std::string& line, Symbol& out_symbol) {
    if (line.empty()) return false;

    std::istringstream iss(line);
    std::string rva_str, size_str, section, name;

    if (!(iss >> rva_str)) return false;

    try {
        out_symbol.rva = std::stoul(rva_str, nullptr, 16);
    } catch (...) { return false; }

    if (!(iss >> size_str)) return false;

    // Check if second token is a type (single char) or a size (hex)
    if (size_str.length() == 1 && std::isalpha(size_str[0])) {
        // Standard NM: RVA TYPE NAME
        out_symbol.section = size_str;
        if (!(iss >> out_symbol.name)) return false;
        out_symbol.size = 0;
    } else {
        // referencia previa/Enhanced BSD: RVA SIZE SECTION NAME
        try {
            out_symbol.size = std::stoul(size_str, nullptr, 16);
        } catch (...) { return false; }

        if (!(iss >> out_symbol.section >> out_symbol.name)) return false;
    }

    size_t tab_pos = line.find('\t');
    if (tab_pos != std::string::npos) {
        out_symbol.source_file = line.substr(tab_pos + 1);
    }

    return true;
}

bool SymbolExtractor::parseSysvLine(const std::string& line, Symbol& out_symbol) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(line);
    while (std::getline(tokenStream, token, '|')) {
        // Trim whitespace
        token.erase(0, token.find_first_not_of(" \t"));
        size_t last = token.find_last_not_of(" \t");
        if (last != std::string::npos) {
            token.erase(last + 1);
        }
        tokens.push_back(token);
    }

    if (tokens.size() < 7) return false;

    out_symbol.name = tokens[0];
    try {
        if (!tokens[1].empty()) out_symbol.rva = std::stoul(tokens[1], nullptr, 16);
        if (!tokens[4].empty()) out_symbol.size = std::stoul(tokens[4], nullptr, 16);
    } catch (...) { return false; }

    out_symbol.section = tokens[6];

    return true;
}

} // namespace interlock::security
