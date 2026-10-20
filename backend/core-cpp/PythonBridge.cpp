#include "PythonBridge.hpp"
#include "embedded/CoreScripts.hpp"
#include <iostream>
#include <nanobind/nanobind.h>
#include <nanobind/eval.h>
#include <sys/mman.h>
#include <unistd.h>

namespace nb = nanobind;

namespace interlock {

void PythonBridge::initialize(const std::string& python_path) {
    std::lock_guard<std::mutex> lock(interpreter_mtx);
    if (initialized) return;

    try {
        if (!Py_IsInitialized()) {
            Py_Initialize();
        }

        {
            // GIL is held by Py_Initialize().
            // 1. Bloqueo de Memoria (Fix #3: mlock + anti-dump)
            auto modules = embedded::get_all_embedded_modules();
            size_t page_size = sysconf(_SC_PAGESIZE);

            for (auto& [name, source] : modules) {
                if (source.empty()) continue;

                void* addr = const_cast<void*>(reinterpret_cast<const void*>(source.data()));
                size_t len = source.size();

                // Align to page boundary for mlock if necessary
                uintptr_t start = reinterpret_cast<uintptr_t>(addr);
                uintptr_t aligned_start = start & ~(page_size - 1);
                size_t aligned_len = len + (start - aligned_start);

                if (mlock(reinterpret_cast<void*>(aligned_start), aligned_len) == 0) {
#ifdef MADV_DONTDUMP
                    madvise(reinterpret_cast<void*>(aligned_start), aligned_len, MADV_DONTDUMP);
#endif
                }
            }

            // 2. Inyección de VFS MetaPathFinder
            nb::module_ main_mod = nb::module_::import_("__main__");
            nb::dict main_dict = main_mod.attr("__dict__");

            nb::exec(nb::str(embedded::IMPORT_HOOK.data(), embedded::IMPORT_HOOK.size()), main_dict);

            nb::dict py_modules;
            for (const auto& [name, source] : modules) {
                py_modules[nb::str(name.data(), name.size())] = nb::str(source.data(), source.size());
            }
            main_dict["install_hook"](py_modules);

            std::cout << "[PythonBridge] M4 Pro Hardened: Memory Locked & VFS Active." << std::endl;
        }

        // Release the GIL so other threads can use it.
        // We use PyEval_SaveThread() because Py_Initialize() held it.
        PyEval_SaveThread();

        initialized = true;
    } catch (const std::exception& e) {
        std::cerr << "[PythonBridge] CRITICAL: Init failed: " << e.what() << std::endl;
    }
}

void PythonBridge::finalize() {
    std::lock_guard<std::mutex> lock(interpreter_mtx);
    if (!initialized) return;
    // Note: We don't call Py_FinalizeEx() here because it's dangerous in multi-threaded apps
    // with detached threads. We just mark as not initialized.
    initialized = false;
}

nlohmann::json PythonBridge::call(const std::string& module_name, const std::string& function_name, const nlohmann::json& args) {
    {
        std::lock_guard<std::mutex> lock(interpreter_mtx);
        if (!initialized) return {{"error", "Not initialized"}};
    }

    try {
        nb::gil_scoped_acquire acquire;
        nb::module_ m = nb::module_::import_(module_name.c_str());
        nb::object func = m.attr(function_name.c_str());
        nb::object res = func(args.dump().c_str());
        return nlohmann::json::parse(nb::cast<std::string>(res));
    } catch (const std::exception& e) {
        return {{"status", "error"}, {"message", e.what()}};
    }
}

void PythonBridge::executeString(const std::string& code) {
    {
        std::lock_guard<std::mutex> lock(interpreter_mtx);
        if (!initialized) return;
    }
    try {
        nb::gil_scoped_acquire acquire;
        nb::exec(nb::str(code.c_str()));
    } catch (const std::exception& e) {
        std::cerr << "[PythonBridge] exec failed: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "[PythonBridge] exec failed with unknown error." << std::endl;
    }
}

} // namespace interlock
