#include "net_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
  #include <windows.h>
  #include <winhttp.h>
#else
  #include <unistd.h>
#endif

bool net_http_get(const char * url, char * out_buf, size_t buf_size)
{
    if(!url || !out_buf || buf_size < 2) return false;

#ifdef _WIN32
    /* Convert URL to wide char */
    int url_wlen = MultiByteToWideChar(CP_UTF8, 0, url, -1, NULL, 0);
    if(url_wlen <= 0) return false;

    WCHAR * urlW = (WCHAR *)malloc(url_wlen * sizeof(WCHAR));
    if(!urlW) return false;
    MultiByteToWideChar(CP_UTF8, 0, url, -1, urlW, url_wlen);

    /* Crack URL */
    URL_COMPONENTSW urlComp = { 0 };
    urlComp.dwStructSize   = sizeof(urlComp);
    urlComp.dwSchemeLength    = (DWORD)-1;
    urlComp.dwHostNameLength  = (DWORD)-1;
    urlComp.dwUrlPathLength   = (DWORD)-1;
    urlComp.dwExtraInfoLength = (DWORD)-1;

    BOOL ok = WinHttpCrackUrl(urlW, 0, 0, &urlComp);
    if(!ok) {
        printf("[net] WinHttpCrackUrl failed\n");
        free(urlW);
        return false;
    }

    /* Copy host */
    WCHAR hostW[256];
    DWORD hostLen = urlComp.dwHostNameLength;
    if(hostLen > 255) hostLen = 255;
    wcsncpy(hostW, urlComp.lpszHostName, hostLen);
    hostW[hostLen] = L'\0';

    /* Build path */
    WCHAR pathW[2048];
    DWORD pathLen = urlComp.dwUrlPathLength + urlComp.dwExtraInfoLength;
    if(pathLen > 2047) pathLen = 2047;
    wcsncpy(pathW, urlComp.lpszUrlPath, pathLen);
    pathW[pathLen] = L'\0';

    free(urlW);

    /* HTTP request */
    HINTERNET hSession = WinHttpOpen(L"Dashboard/1.0",
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     NULL, NULL, 0);
    if(!hSession) return false;

    HINTERNET hConnect = WinHttpConnect(hSession, hostW, urlComp.nPort, 0);
    if(!hConnect) { WinHttpCloseHandle(hSession); return false; }

    DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", pathW,
                                            NULL, NULL, NULL, flags);
    if(!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    ok = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    if(ok) ok = WinHttpReceiveResponse(hRequest, NULL);

    size_t total = 0;
    if(ok) {
        DWORD avail;
        while(WinHttpQueryDataAvailable(hRequest, &avail) && avail > 0) {
            DWORD to_read = (total + avail + 1 > buf_size)
                          ? (DWORD)(buf_size - total - 1) : avail;
            DWORD read = 0;
            WinHttpReadData(hRequest, out_buf + total, to_read, &read);
            total += read;
            if(total >= buf_size - 1) break;
        }
    }
    out_buf[total] = '\0';

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    if(!ok) {
        printf("[net] HTTP GET failed: %s\n", url);
        return false;
    }
    printf("[net] Received %zu bytes from %s\n", total, url);

#else
    /* Linux: curl via popen */
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "curl -s --max-time 15 \"%s\" 2>/dev/null", url);
    FILE * f = popen(cmd, "r");
    if(!f) { printf("[net] popen failed\n"); return false; }
    size_t total = fread(out_buf, 1, buf_size - 1, f);
    out_buf[total] = '\0';
    int rc = pclose(f);
    if(rc != 0 || total == 0) {
        printf("[net] curl failed (exit %d) for: %s\n", rc, url);
        return false;
    }
    printf("[net] Received %zu bytes from %s\n", total, url);
#endif
    return true;
}
