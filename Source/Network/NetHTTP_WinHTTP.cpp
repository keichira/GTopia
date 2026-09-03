#include "../IO/Log.h"
#include "../Utils/StringUtils.h"
#include "NetHTTP.h"

#if defined(_WIN32) && !defined(NETHTTP_USE_CURL)

#include <windows.h>
#include <winhttp.h>

NetHTTP::NetHTTP() : m_port(80), m_isSSL(false), m_status(0), m_error(HTTP_ERROR_NONE) {}

NetHTTP::~NetHTTP()
{
    Kill();
}

bool NetHTTP::ExecuteRequest(const string& method, const string& path)
{
    m_body.clear();
    m_header.clear();
    m_error = HTTP_ERROR_NONE;

    string formattedPath = path;
    if (formattedPath.empty() || formattedPath[0] != '/')
    {
        formattedPath = "/" + formattedPath;
    }

    DWORD dwAccessType = WINHTTP_ACCESS_TYPE_DEFAULT_PROXY; // deprecated on Windows 8.1+
#ifdef WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY
    dwAccessType = WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY;
#endif

    HINTERNET hSession = WinHttpOpen(L"NetHTTP/1.0", dwAccessType, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession)
    {
        LOGGER_LOG_ERROR("WinHttpOpen Failed. Error: %lu", GetLastError());
        Error(HTTP_ERROR_CONNECT_FAIL);
        return false;
    }

    /*DWORD dwProtocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
    if (!WinHttpSetOption(hSession, WINHTTP_OPTION_SECURE_PROTOCOLS, &dwProtocols, sizeof(dwProtocols)))
    {
        LOGGER_LOG_ERROR("WinHttpSetOption SECURE_PROTOCOLS Warning. Error: %lu", GetLastError());
    }*/

    std::wstring wServer = UTF8ToUTF16(m_server);
    HINTERNET hConnect = WinHttpConnect(hSession, wServer.c_str(), m_port, 0);
    if (!hConnect)
    {
        LOGGER_LOG_ERROR("WinHttpConnect Failed. Error: %lu", GetLastError());
        WinHttpCloseHandle(hSession);
        Error(HTTP_ERROR_CONNECT_FAIL);
        return false;
    }

    std::wstring wMethod = UTF8ToUTF16(method);
    std::wstring wPath = UTF8ToUTF16(formattedPath);
    DWORD dwFlags = (m_isSSL || m_port == 443) ? WINHTTP_FLAG_SECURE : 0;

    HINTERNET hRequest =
        WinHttpOpenRequest(hConnect, wMethod.c_str(), wPath.c_str(), NULL, WINHTTP_NO_REFERER, NULL, dwFlags);
    if (!hRequest)
    {
        LOGGER_LOG_ERROR("WinHttpOpenRequest Failed. Error: %lu", GetLastError());
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        Error(HTTP_ERROR_CONNECT_FAIL);
        return false;
    }

    if (dwFlags & WINHTTP_FLAG_SECURE)
    {
        DWORD dwSecFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
        WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &dwSecFlags, sizeof(dwSecFlags));
    }

    std::wstring wHeaders;
    for (auto& [key, val] : m_headers)
    {
        wHeaders += UTF8ToUTF16(key + ": " + val + "\r\n");
    }

    LPCWSTR pHeadersPtr = wHeaders.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : wHeaders.c_str();
    DWORD dwHeadersLen = wHeaders.empty() ? 0 : (DWORD)-1L; // woah...

    LPVOID pPostData = WINHTTP_NO_REQUEST_DATA;
    DWORD dwPostLen = 0;

    if (method == "POST" && !m_postData.empty())
    {
        pPostData = (LPVOID)m_postData.data();
        dwPostLen = (DWORD)m_postData.size();
    }

    BOOL bResults = WinHttpSendRequest(hRequest, pHeadersPtr, dwHeadersLen, pPostData, dwPostLen, dwPostLen, 0);
    if (bResults)
    {
        bResults = WinHttpReceiveResponse(hRequest, NULL);
    }

    if (!bResults)
    {
        LOGGER_LOG_ERROR("WinHTTP Request Failed! LastError Code: %lu", GetLastError());

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        Error(HTTP_ERROR_FAIL_CONNECT);
        return false;
    }

    DWORD statusCode = 0;
    DWORD dwSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX,
                        &statusCode, &dwSize, WINHTTP_NO_HEADER_INDEX);
    m_status = (uint16)statusCode;

    char buffer[8192];
    DWORD dwDownloaded = 0;
    while (WinHttpReadData(hRequest, buffer, sizeof(buffer), &dwDownloaded) && dwDownloaded > 0)
    {
        HandleDataReceive(buffer, dwDownloaded);
    }

    if (m_file.IsOpen())
        m_file.Close();

    m_headers.clear();
    m_postData.clear();
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    if (m_status == 0)
        Error(HTTP_ERROR_FAIL_CONNECT);
    else if (m_status >= 400 && m_status < 500)
        Error(HTTP_ERROR_CLIENT);
    else if (m_status >= 500)
        Error(HTTP_ERROR_SERVER);

    return m_error == HTTP_ERROR_NONE;
}

#endif