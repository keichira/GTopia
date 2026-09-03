#include "../IO/Log.h"
#include "../Utils/StringUtils.h"
#include "../Utils/Timer.h"
#include "NetHTTP.h"

#ifdef NETHTTP_USE_SOCKET

NetHTTP::NetHTTP()
    : m_error(HTTP_ERROR_NONE), m_port(80), m_state(HTTP_STATE_IDLE), m_chunked(false), m_contentLength(0), m_status(0),
      m_pNetClient(nullptr)
{
    m_netSocket.GetEvents().Register(SOCKET_EVENT_TYPE_RECEIVE,
                                     Delegate<NetClient*>::Create<NetHTTP, &NetHTTP::OnDataReceive>(this));
    m_netSocket.GetEvents().Register(SOCKET_EVENT_TYPE_CONNECT,
                                     Delegate<NetClient*>::Create<NetHTTP, &NetHTTP::OnConnect>(this));
    m_netSocket.GetEvents().Register(SOCKET_EVENT_TYPE_DISCONNECT,
                                     Delegate<NetClient*>::Create<NetHTTP, &NetHTTP::OnDisconnect>(this));
}

NetHTTP::~NetHTTP()
{
    Kill();
}

void NetHTTP::OnConnect(NetClient* pClient)
{
    if (!pClient || m_pNetClient)
    {
        Error(HTTP_ERROR_SOCKET);
        return;
    }
    m_pNetClient = pClient;
}

void NetHTTP::OnDisconnect(NetClient* pClient)
{
    if (m_pNetClient == pClient)
        m_pNetClient = nullptr;
}

void NetHTTP::OnDataReceive(NetClient* pClient)
{
    uint32 size = pClient->recvQueue.GetDataSize();
    if (size == 0)
        return;

    string data(size, 0);
    pClient->recvQueue.Peek(data.data(), size);

    if (m_state == HTTP_STATE_READ_HEAD)
    {
        usize headerEndPos = data.find("\r\n\r\n");
        if (headerEndPos != string::npos)
        {
            headerEndPos += 4;
            if (pClient->recvQueue.GetDataSize() >= headerEndPos)
            {
                m_header.append(data.data(), headerEndPos);
                pClient->recvQueue.Skip(headerEndPos);

                ParseHeader(m_header);
                m_state = HTTP_STATE_READ_BODY;

                size = pClient->recvQueue.GetDataSize();
                if (size == 0)
                    return;
                data.resize(size);
                pClient->recvQueue.Peek(data.data(), size);
            }
        }
    }

    if (m_state == HTTP_STATE_READ_BODY)
    {
        if (m_chunked)
        {
            usize bodyLineEnd = data.find("\r\n");
            if (bodyLineEnd != -1)
            {
                uint32 chunkSize = 0;
                ToUInt(data.substr(0, bodyLineEnd), chunkSize, 16);

                if (chunkSize == 0)
                {
                    pClient->recvQueue.Skip(2);
                    m_state = HTTP_STATE_COMPLETE;
                    return;
                }

                if (pClient->recvQueue.GetDataSize() < (bodyLineEnd + 2) + (chunkSize + 2))
                    return;

                pClient->recvQueue.Skip(bodyLineEnd + 2);
                HandleDataReceive(data.data() + bodyLineEnd + 2, chunkSize);
                pClient->recvQueue.Skip(chunkSize + 2);
            }
        }
        else if (m_contentLength != 0)
        {
            if (pClient->recvQueue.GetDataSize() < m_contentLength)
                return;

            HandleDataReceive(data.data(), m_contentLength);
            pClient->recvQueue.Skip(m_contentLength);
            m_state = HTTP_STATE_COMPLETE;
        }
    }
}

bool NetHTTP::ExecuteRequest(const string& method, const string& path)
{
    Clear();

    string request = method + " " + EncodeURL(path) + " HTTP/1.1\r\n";
    request += "Host: " + m_server + "\r\n";
    request += "Accept: */*\r\n";

    for (const auto& [key, val] : m_headers)
    {
        request += key + ": " + val + "\r\n";
    }

    if (method == "POST")
    {
        if (m_headers.find("Content-Type") == m_headers.end())
            request += "Content-Type: application/x-www-form-urlencoded\r\n";

        request += "Content-Length: " + ToString(m_postData.size()) + "\r\n";
    }

    request += "Connection: close\r\n\r\n";

    if (method == "POST" && !m_postData.empty())
    {
        request += m_postData;
    }

    int16 val = m_netSocket.Connect(m_server, m_port, false);
    if (val < 0)
    {
        Error(HTTP_ERROR_CONNECT_FAIL);
        return false;
    }

    UpdateSocketRequest(request);
    return m_error == HTTP_ERROR_NONE;
}

void NetHTTP::UpdateSocketRequest(const string& requestToSend)
{
    m_state = HTTP_STATE_READ_HEAD;
    bool sentPacket = false;
    Timer startTimer;

    while (m_state != HTTP_STATE_COMPLETE && m_error == HTTP_ERROR_NONE)
    {
        m_netSocket.Update(true);
        SleepMS(10);

        if ((!sentPacket || !m_pNetClient) && startTimer.GetElapsedTime() >= HTTP_CLIENT_CONNECT_MS)
        {
            Error(HTTP_ERROR_CONNECT_FAIL);
            break;
        }

        if (!sentPacket && m_pNetClient)
        {
            m_pNetClient->Send((void*)requestToSend.data(), requestToSend.size());
            m_postData.clear();
            sentPacket = true;
        }

        if (startTimer.GetElapsedTime() >= HTTP_TIMEOUT_MS)
        {
            Error(HTTP_ERROR_TIME_EXCEED);
            break;
        }
    }

    if (m_file.IsOpen() && m_state == HTTP_STATE_COMPLETE && m_error == HTTP_ERROR_NONE)
    {
        m_file.Close();
    }

    m_netSocket.CloseAllClients();
}

void NetHTTP::ParseHeader(const string& header)
{
    if (header.empty())
        return;

    auto lines = Split(header, '\n');
    m_status = ToUInt(Split(lines[0], ' ')[1]);

    for (auto& line : lines)
    {
        if (line.empty())
            continue;
        if (line.back() == '\r')
            line.pop_back();

        usize colon = line.find(':');
        if (colon == string::npos)
            continue;

        string key = line.substr(0, colon);
        string value = line.substr(colon + 1);

        while (!value.empty() && value[0] == ' ')
            value.erase(0, 1);
        while (!value.empty() && (value.back() == ' ' || value.back() == '\r'))
            value.pop_back();

        if (key == "Transfer-Encoding" && value == "chunked")
            m_chunked = true;

        if (key == "Content-Length")
            m_contentLength = ToUInt(value);
    }

    if (m_status == 0)
        Error(HTTP_ERROR_FAIL_CONNECT);
    else if (m_status >= 400 && m_status < 500)
        Error(HTTP_ERROR_CLIENT);
    else if (m_status >= 500)
        Error(HTTP_ERROR_SERVER);
}

#endif