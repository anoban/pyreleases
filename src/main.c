#define __PARSE_TEST__ 0

#include <helper.h>
#include <pyreleases.h>
#include <stdio.h>
#include <stdlib.h>
// lookup https://learn.microsoft.com/en-us/previous-versions/hf4y5e3w(v=vs.140)?redirectedfrom=MSDN

int wmain(void) {
    if (!ActivateVirtualTerminalEscapes())
        fputws(L"Win32 Virtual Terminal Escape sequences are not enabled! Programme output will fall back to black and white!", stderr);

    DWORD response_size = 0;

#if (__PARSE_TEST__ == 1)
    char* const restrict html_text = Open(L"./Python Releases for Windows.txt", &response_size);
    puts(html_text);
#else
    wchar_t       SERVER[BUFF_SIZE]       = L"www.python.org";
    wchar_t       ACCESS_POINT[BUFF_SIZE] = L"/downloads/windows/";
    const hint3_t handles                 = HttpGet(SERVER, ACCESS_POINT);

    // ReadHttpResponse will handle if handles are NULLs, no need for external error handling here.
    char* const restrict html_text        = ReadHttpResponse(handles, &response_size);
#endif

    // LocateStableReleasesDiv will handle NULL returns from ReadHttpResponse internally,
    // so again no need for main to handle errors explicitly.
    // in case of a NULL input, returned range will be {0, 0}.
    const range_t stable = LocateStableReleasesDiv(html_text, HTTP_RESPONSE_SIZE); // works correctly :)

#if defined(DEBUG) || defined(_DEBUG)
    wprintf_s(L"Response size :: %lu bytes. Stable releases :: {%6llu - %6llu}\n", response_size, stable.begin, stable.end);
#endif

    if (!stable.begin && !stable.end) {
        fputws(L"Error: Call to LocateStableReleasesDiv failed!", stderr);
        goto CLEANUP;
    }

    // zero out the buffer downstream of the end of stable releases (i.e pre releases)
    // memset(html_text + stable.end, 0U, response_size - stable.end);

    const results_t parsed = ParseStableReleases(html_text + stable.begin, stable.end - stable.begin);

    // may happen due to malloc failures or invalid inputs.
    if (!parsed.begin) {
        fputws(L"Error: Call to ParseStableReleases failed!", stderr);
        goto CLEANUP;
    }

    char system_python[BUFF_SIZE] = { 0 };
    if (!GetSystemPythonExeVersion(system_python, BUFF_SIZE)) fputws(L"Error: Call to GetSystemPythonVersion failed!", stderr);

#if defined(DEBUG) || defined(_DEBUG) || defined(__PARSE_TEST__)
    wprintf_s(
        L"%llu python releases have been deserialized.\n"
        L"Installed python version is %S\n",
        parsed.count,
        system_python
    );
#endif

    // PrintReleases will handle empty instances of py_version internally.
    PrintReleases(parsed, system_python);

    free(html_text);
    free(parsed.begin);
    return EXIT_SUCCESS;

CLEANUP:
    free(html_text);
    return EXIT_FAILURE;
}
