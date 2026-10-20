#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Dispatches a ticket to the C# Operations Engine via XML.
 * @param xml_ptr Pointer to the UTF-8 XML string.
 */
void tactical_dispatch_ticket(const char* xml_ptr);

/**
 * @brief Registers a finding in the C# Operations Engine via XML.
 * @param xml_ptr Pointer to the UTF-8 XML string.
 */
void tactical_register_finding(const char* xml_ptr);

/**
 * @brief Generates a full mission report in XML format from the C# engine.
 * @return A pointer to a C-string containing the XML report. Must be freed by the caller if applicable (depends on HGlobal logic).
 */
const char* report_forge_generate();

#ifdef __cplusplus
}
#endif
