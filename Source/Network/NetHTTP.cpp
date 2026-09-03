#include "NetHTTP.h"
#include "../IO/Log.h"
#include "../Utils/StringUtils.h"

void NetHTTP::Init(const string& server)
{
    Kill();

    m_server = server;
    m_isSSL = false;
    m_port = 80;

    if (m_server.find("http://") == 0)
    {
        m_server = m_server.substr(7);
        m_port = 80;
        m_isSSL = false;
    }
    else if (m_server.find("https://") == 0)
    {
        m_server = m_server.substr(8);
        m_port = 443;
        m_isSSL = true;
    }
    else
    {
        m_isSSL = true;
        m_port = 443;
    }

#ifdef NETHTTP_USE_SOCKET
    if (m_isSSL)
    {
        m_netSocket.CreateSSLCtx();
    }
#endif

    usize slashPos = m_server.find('/');
    if (slashPos != string::npos)
    {
        m_server = m_server.substr(0, slashPos);
    }

    usize pos = m_server.find(":");
    if (pos != string::npos)
    {
        m_port = (uint16)ToUInt(m_server.substr(pos + 1));
        m_server = m_server.substr(0, pos);
    }
}

void NetHTTP::Kill()
{
    Clear();
#ifdef NETHTTP_USE_SOCKET
    m_netSocket.Kill();
#endif
}

void NetHTTP::Clear()
{
    m_header.clear();
    m_body.clear();
    m_postData.clear();
    m_headers.clear();
    m_status = 0;
    m_error = HTTP_ERROR_NONE;
    m_file.Close();

#ifdef NETHTTP_USE_SOCKET
    m_chunked = false;
    m_contentLength = 0;
    m_state = HTTP_STATE_IDLE;
#endif
}

void NetHTTP::Error(eHTTPError error)
{
    Clear();
    m_error = error;
#ifdef NETHTTP_USE_SOCKET
    m_pNetClient = nullptr;
#endif
}

void NetHTTP::AddPostData(const string& key, const string& value)
{
    if (!m_postData.empty())
        m_postData += "&";

    m_postData += EncodeURL(key) + "=" + EncodeURL(value);

    if (m_headers.find("Content-Type") == m_headers.end())
        m_headers["Content-Type"] = "application/x-www-form-urlencoded";
}

bool NetHTTP::SetOutputFile(const string& filePath)
{
    if (!m_file.Open(filePath, FILE_MODE_WRITE))
    {
        Error(HTTP_ERROR_WRITE_FILE);
        return false;
    }
    return true;
}

void NetHTTP::HandleDataReceive(const char* data, uint32 size)
{
    if (m_file.IsOpen())
        m_file.Write((void*)data, size);
    else
        m_body.append(data, size);
}

void NetHTTP::HandleHeaderReceive(const char* data, uint32 size)
{
    m_header.append(data, size);
}

bool NetHTTP::Get(const string& path)
{
    return ExecuteRequest("GET", path);
}
bool NetHTTP::Post(const string& path)
{
    return ExecuteRequest("POST", path);
}

string EncodeURL(const string& str)
{
    string encoded;
    for (const char& c : str)
    {
        if (IsDigit(c) || IsAlpha(c) || c == '-' || c == '_' || c == '.' || c == '~' || c == '/' || c == '&' ||
            c == '?' || c == '=')
        {
            encoded += c;
        }
        else if (c == ' ')
        {
            encoded += "%20";
        }
        else
        {
            encoded += "%";
            encoded += ToHex(&c, 1);
        }
    }
    return encoded;
}