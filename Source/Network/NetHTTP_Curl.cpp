#include "../IO/Log.h"
#include "../Utils/StringUtils.h"
#include "NetHTTP.h"

#ifdef NETHTTP_USE_CURL

#include <curl/curl.h>

static usize CurlWriteCallback(void* contents, usize size, usize nmemb, void* pThis)
{
    usize totalSize = size * nmemb;
    ((NetHTTP*)(pThis))->HandleDataReceive((const char*)(contents), (uint32)totalSize);
    return totalSize;
}

static usize CurlHeaderCallback(void* contents, usize size, usize nmemb, void* pThis)
{
    usize totalSize = size * nmemb;
    ((NetHTTP*)(pThis))->HandleHeaderReceive((const char*)(contents), (uint32)totalSize);
    return totalSize;
}

NetHTTP::NetHTTP() : m_port(80), m_isSSL(false), m_status(0), m_error(HTTP_ERROR_NONE)
{
    static bool curlInitialized = false;
    if (!curlInitialized)
    {
        curl_global_init(CURL_GLOBAL_ALL);
        curlInitialized = true;
    }
}

NetHTTP::~NetHTTP()
{
    Kill();
}

bool NetHTTP::ExecuteRequest(const string& method, const string& path)
{
    m_body.clear();
    m_header.clear();
    m_error = HTTP_ERROR_NONE;

    CURL* curl = curl_easy_init();
    if (!curl)
    {
        Error(HTTP_ERROR_CONNECT_FAIL);
        return false;
    }

    string protocol = (m_isSSL || m_port == 443) ? "https://" : "http://";
    string portStr = "";
    if ((m_isSSL && m_port != 443) || (!m_isSSL && m_port != 80))
    {
        portStr = ":" + ToString(m_port);
    }

    string formattedPath = path;
    if (formattedPath.empty() || formattedPath[0] != '/')
    {
        formattedPath = "/" + formattedPath;
    }

    string fullUrl = protocol + m_server + portStr + formattedPath;

    curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, CurlHeaderCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, this);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    curl_easy_setopt(curl, CURLOPT_USERAGENT, "NetHTTP/1.0");

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);

    struct curl_slist* headerList = NULL;
    for (auto& [key, val] : m_headers)
    {
        string headerLine = key + ": " + val;
        headerList = curl_slist_append(headerList, headerLine.c_str());
    }

    if (headerList)
    {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
    }

    if (method == "POST")
    {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, m_postData.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)m_postData.size());
    }

    CURLcode res = curl_easy_perform(curl);
    if (headerList)
    {
        curl_slist_free_all(headerList);
    }

    if (res != CURLE_OK)
    {
        LOGGER_LOG_ERROR("NetHTTP_Curl: Error %s", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        Error(HTTP_ERROR_FAIL_CONNECT);
        return false;
    }

    long responseCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
    m_status = (uint16)responseCode;

    if (m_file.IsOpen())
        m_file.Close();

    m_headers.clear();
    m_postData.clear();
    curl_easy_cleanup(curl);

    if (m_status == 0)
        Error(HTTP_ERROR_FAIL_CONNECT);
    else if (m_status >= 400 && m_status < 500)
        Error(HTTP_ERROR_CLIENT);
    else if (m_status >= 500)
        Error(HTTP_ERROR_SERVER);

    return m_error == HTTP_ERROR_NONE;
}

#endif