// Engineering diagnostic only. Run once per DLL in a separate process.
// Static CRT keeps this probe independent of the application's runtime DLLs.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstdio>
#include <cwchar>

int wmain(int argc, wchar_t** argv)
{
    if (argc != 2) {
        std::fputs("Usage: artplus_runtime_probe <absolute DLL path>\n", stderr);
        return 2;
    }
    if (!(argv[1][0] && argv[1][1] == L':' &&
          (argv[1][2] == L'\\' || argv[1][2] == L'/')) &&
        !(argv[1][0] == L'\\' && argv[1][1] == L'\\')) {
        std::fputs("An absolute DLL path is required.\n", stderr);
        return 2;
    }
    // Prevent unattended diagnostics from waiting on Windows error dialogs.
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX |
                 SEM_NOOPENFILEERRORBOX);
    std::wprintf(L"Loading: %ls\n", argv[1]);
    std::fflush(stdout);
    HMODULE module = LoadLibraryW(argv[1]);
    if (!module) {
        const DWORD error = GetLastError();
        wchar_t description[1024] = {};
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       nullptr, error, 0, description, 1024, nullptr);
        std::wprintf(L"FAILED: Win32 %lu (0x%08lX): %ls\n",
                     error, error, description);
        return 1;
    }
    wchar_t loaded[32768] = {};
    const DWORD length = GetModuleFileNameW(module, loaded, 32768);
    if (!length || length >= 32768) {
        std::fputs("Loaded, but could not resolve module path.\n", stderr);
        FreeLibrary(module);
        return 1;
    }
    std::wprintf(L"LOADED: %ls\n", loaded);
    std::fflush(stdout);
    if (!FreeLibrary(module)) {
        std::printf("FAILED: FreeLibrary Win32 %lu\n", GetLastError());
        return 1;
    }
    std::puts("UNLOADED");
    return 0;
}
