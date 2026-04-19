#pragma once

#ifdef _WIN32
#include <windows.h>
#endif

#include <string>

namespace subprocess {

class Popen {
public:
    struct StdinPipe {
#ifdef _WIN32
        HANDLE hWrite = INVALID_HANDLE_VALUE;
#endif
        void write(const void* data, size_t size) {
#ifdef _WIN32
            if (hWrite == INVALID_HANDLE_VALUE) return;
            DWORD written = 0;
            WriteFile(hWrite, data, static_cast<DWORD>(size), &written, nullptr);
#endif
        }
    } m_stdin;

private:
#ifdef _WIN32
    HANDLE hProcess = INVALID_HANDLE_VALUE;
    HANDLE hThread  = INVALID_HANDLE_VALUE;
#endif

public:
    Popen() = default;

    explicit Popen(const std::string& command) {
#ifdef _WIN32
        SECURITY_ATTRIBUTES sa{};
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE;

        HANDLE hStdinRead  = INVALID_HANDLE_VALUE;
        HANDLE hStdinWrite = INVALID_HANDLE_VALUE;

        if (!CreatePipe(&hStdinRead, &hStdinWrite, &sa, 0)) return;
        SetHandleInformation(hStdinWrite, HANDLE_FLAG_INHERIT, 0);
        m_stdin.hWrite = hStdinWrite;

        STARTUPINFOA si{};
        si.cb        = sizeof(STARTUPINFOA);
        si.dwFlags   = STARTF_USESTDHANDLES;
        si.hStdInput = hStdinRead;
        si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        si.hStdError  = GetStdHandle(STD_ERROR_HANDLE);

        PROCESS_INFORMATION pi{};
        std::string cmdLine = command;

        if (CreateProcessA(
                nullptr, cmdLine.data(), nullptr, nullptr,
                TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
            hProcess = pi.hProcess;
            hThread  = pi.hThread;
        }
        CloseHandle(hStdinRead);
#endif
    }

    Popen(Popen&& o) noexcept {
#ifdef _WIN32
        m_stdin.hWrite = o.m_stdin.hWrite; o.m_stdin.hWrite = INVALID_HANDLE_VALUE;
        hProcess = o.hProcess;             o.hProcess = INVALID_HANDLE_VALUE;
        hThread  = o.hThread;              o.hThread  = INVALID_HANDLE_VALUE;
#endif
    }

    Popen& operator=(Popen&& o) noexcept {
        if (this != &o) {
            releaseHandles();
#ifdef _WIN32
            m_stdin.hWrite = o.m_stdin.hWrite; o.m_stdin.hWrite = INVALID_HANDLE_VALUE;
            hProcess = o.hProcess;             o.hProcess = INVALID_HANDLE_VALUE;
            hThread  = o.hThread;              o.hThread  = INVALID_HANDLE_VALUE;
#endif
        }
        return *this;
    }

    Popen(const Popen&)            = delete;
    Popen& operator=(const Popen&) = delete;

    ~Popen() { releaseHandles(); }

    bool close() {
#ifdef _WIN32
        if (m_stdin.hWrite != INVALID_HANDLE_VALUE) {
            CloseHandle(m_stdin.hWrite);
            m_stdin.hWrite = INVALID_HANDLE_VALUE;
        }
        if (hProcess == INVALID_HANDLE_VALUE) return true;
        WaitForSingleObject(hProcess, INFINITE);
        DWORD exitCode = 1;
        GetExitCodeProcess(hProcess, &exitCode);
        CloseHandle(hProcess); hProcess = INVALID_HANDLE_VALUE;
        CloseHandle(hThread);  hThread  = INVALID_HANDLE_VALUE;
        return exitCode != 0;
#else
        return false;
#endif
    }

private:
    void releaseHandles() {
#ifdef _WIN32
        if (m_stdin.hWrite != INVALID_HANDLE_VALUE) {
            CloseHandle(m_stdin.hWrite);
            m_stdin.hWrite = INVALID_HANDLE_VALUE;
        }
        if (hProcess != INVALID_HANDLE_VALUE) {
            WaitForSingleObject(hProcess, INFINITE);
            CloseHandle(hProcess); hProcess = INVALID_HANDLE_VALUE;
        }
        if (hThread != INVALID_HANDLE_VALUE) {
            CloseHandle(hThread); hThread = INVALID_HANDLE_VALUE;
        }
#endif
    }
};

} // namespace subprocess
