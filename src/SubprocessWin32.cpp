#include "../include/slib/Subprocess.hpp"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>

namespace slib {
    HANDLE jobObject;

    struct SubprocessNativeWin32 {
        PROCESS_INFORMATION processInfo;
    };

    Subprocess::Subprocess(const String& commandLine) {
        STARTUPINFOA si{};
        si.cb = sizeof(si);

        if (jobObject != nullptr) {
            jobObject = CreateJobObjectA(nullptr, "Subprocess Job Object");
            JOBOBJECT_BASIC_LIMIT_INFORMATION info{};
            info.LimitFlags = 0x2000;

            JOBOBJECT_EXTENDED_LIMIT_INFORMATION extendedInfo{};
            extendedInfo.BasicLimitInformation = info;

            SetInformationJobObject(jobObject, JobObjectExtendedLimitInformation, &extendedInfo, sizeof(extendedInfo));
        }

        SubprocessNativeWin32* sn = new SubprocessNativeWin32;
        sn->processInfo = PROCESS_INFORMATION{};
        nativeHandle = sn;

        char* cmdline = _strdup(commandLine.cStr());
        CreateProcessA(nullptr, cmdline, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &sn->processInfo);
        AssignProcessToJobObject(jobObject, sn->processInfo.hProcess);
        free((void*)cmdline);
        attached = true;
    }

    Subprocess::Subprocess(const String& commandLine, const String& workingDirectory) {
        STARTUPINFOA si{};
        si.cb = sizeof(si);

        SubprocessNativeWin32* sn = new SubprocessNativeWin32;
        sn->processInfo = PROCESS_INFORMATION{};
        nativeHandle = sn;

        char* cmdline = _strdup(commandLine.cStr());
        CreateProcessA(nullptr, cmdline, nullptr, nullptr, FALSE, 0, nullptr, workingDirectory.cStr(), &si, &sn->processInfo);
        free((void*)cmdline);
        attached = true;
    }

    void Subprocess::waitForFinish() {
        SubprocessNativeWin32* sn = reinterpret_cast<SubprocessNativeWin32*>(nativeHandle);
        WaitForSingleObject(sn->processInfo.hProcess, INFINITE);
    }

    void Subprocess::detach() {
        attached = false;
    }

    void Subprocess::kill() {
        SubprocessNativeWin32* sn = reinterpret_cast<SubprocessNativeWin32*>(nativeHandle);
        TerminateProcess(sn->processInfo.hProcess, 0);
    }

    bool Subprocess::alive() {
        if (!attached) return false;
        SubprocessNativeWin32* sn = reinterpret_cast<SubprocessNativeWin32*>(nativeHandle);

        DWORD exit;
        GetExitCodeProcess(sn->processInfo.hProcess, &exit);

        return exit == STILL_ACTIVE;
    }

    Subprocess::~Subprocess() {
        if (attached && alive()) abort();

        SubprocessNativeWin32* sn = reinterpret_cast<SubprocessNativeWin32*>(nativeHandle);
        CloseHandle(sn->processInfo.hProcess);
        CloseHandle(sn->processInfo.hThread);
        delete sn;
    }
}