#include "NetHTTP.h"
#include "../IO/Log.h"
#include "../Utils/Timer.h"
#include "../Utils/StringUtils.h"

NetHTTP::NetHTTP()
: m_error(HTTP_ERROR_NONE), m_port(80), m_state(HTTP_STATE_IDLE), m_chunked(false), m_contentLength(0)
{
    m_netSocket.GetEvents().Register(
        SOCKET_EVENT_TYPE_RECEIVE,
        Delegate<NetClient*>::Create<NetHTTP, &NetHTTP::OnDataReceive>(this)
    );
}

NetHTTP::~NetHTTP()
{
    m_netSocket.Kill();
}

void NetHTTP::Init(const string& server)
{
    m_server = server;

    if(m_server.find("http://") != -1) {
        m_server = m_server.substr(7);
        m_port = 80;
    }
    else if(m_server.find("https://") != -1) {
        m_server = m_server.substr(8);
        m_port = 443;
        m_netSocket.CreateSSLCtx();
    }

    int32 pos = m_server.find(":");
    if(pos != -1) {
        m_port = (uint16)ToUInt(m_server.substr(pos + 1));
        m_server = m_server.substr(0, pos);
    }
}

void NetHTTP::OnDataReceive(NetClient* pClient)
{
    uint32 size = pClient->recvQueue.GetDataSize();
    if(size == 0) {
        return;
    }

    string data(size, 0);
    pClient->recvQueue.Peek(data.data(), size);

    if(m_state == HTTP_STATE_READ_HEAD) {
        int32 headerEndPos = data.find("\r\n\r\n");
        
        if(headerEndPos != -1) {
            headerEndPos += 4;
        
            if(pClient->recvQueue.GetDataSize() >= headerEndPos) {
                m_header.append(data.data(), headerEndPos);

                pClient->recvQueue.Skip(headerEndPos);
        
                ParseHeader(m_header);
                m_state = HTTP_STATE_READ_BODY;
            }
        }
    }
    else if(m_state == HTTP_STATE_READ_BODY) {
        if(m_chunked) {
            int32 bodyLineEnd = data.find("\r\n");

            if(bodyLineEnd != -1) {
                uint32 chunkSize = (uint32)stoul(data.substr(0, bodyLineEnd), nullptr, 16); // LOL add that to StringUtils
    
                if(chunkSize == 0) {
                    pClient->recvQueue.Skip(2);
                    m_state = HTTP_STATE_COMPLETE;
                    return;
                }
    
                if(pClient->recvQueue.GetDataSize() < (bodyLineEnd + 2) + (chunkSize + 2)) {
                    return;
                }
                pClient->recvQueue.Skip(bodyLineEnd + 2);
    
                m_body.append(data.data() + bodyLineEnd + 2, chunkSize);

                pClient->recvQueue.Skip(chunkSize);
                pClient->recvQueue.Skip(2);
            }
        }
        else if(m_contentLength != 0) {
            if(pClient->recvQueue.GetDataSize() < m_contentLength) {
                return;
            }

            m_body.append(data.data(), m_contentLength);
            pClient->recvQueue.Skip(m_contentLength);
            m_state = HTTP_STATE_COMPLETE;
        }
    }
}

void NetHTTP::OnClientDisconnect(NetClient* pClient)
{
    if(m_state != HTTP_STATE_IDLE) {
        Error(HTTP_ERROR_SOCKET);
    }
}

bool NetHTTP::Get(const string& path)
{
    Clear();

    string header =
    "GET " + EncodeURL(path) + " HTTP/1.1\r\n"
    "Host: " + m_server + "\r\n"
    "Accept: */*\r\n"
    "Connection: close\r\n"
    "\r\n";

    int16 val = m_netSocket.Connect(m_server, m_port, false);
    
    NetClient* pClient = m_netSocket.GetClient(val);
    if(!pClient) {
        Error(HTTP_ERROR_CONNECT_FAIL);
        return false;
    }

    pClient->Send(header.data(), header.size());

    Update();
    return true;
}

void NetHTTP::Update()
{
    m_state = HTTP_STATE_READ_HEAD;

    uint64 startTime = Time::GetSystemTime();
    while(m_state != HTTP_STATE_COMPLETE && m_error == HTTP_ERROR_NONE)
    {
        m_netSocket.Update(true);
        SleepMS(40);

        if(Time::GetSystemTime() - startTime >= HTTP_TIMEOUT_MS) {
            Error(HTTP_ERROR_TIME_EXCEED);
            break;
        }
    }
}

void NetHTTP::ParseHeader(const string& header)
{
    if(header.empty()) {
        return;
    }

    auto lines = Split(header, '\n');
    uint16 resultCode = ToUInt(Split(lines[0], ' ')[1]);
    m_status = resultCode;

    for (auto& line : lines)
    {
        if(line.empty()) {
            continue;
        }

        if(line.back() == '\r') {
            line.pop_back();
        }
    
        uint32 colon = line.find(':');
        if(colon == -1) {
            continue;
        }
    
        string key = line.substr(0, colon);
        string value = line.substr(colon + 1);
    
        // aint gonna lie so lazy add Trim in StringUtils
        while(!value.empty() && value[0] == ' ') {
            value.erase(0, 1);
        }
        while(!value.empty() && (value.back() == ' ' || value.back() == '\r')) {
            value.pop_back();
        }

        if(key == "Transfer-Encoding") {   
            if(value == "chunked") {
                m_chunked = true;
            }
        }

        if(key == "Content-Length") {
            m_contentLength = ToUInt(value);
        }
    }

    if(m_status >= 400 && m_status < 500) {
        Error(HTTP_ERROR_CLIENT);
    }
    else if(m_status >= 500 && m_status < 600) {
        Error(HTTP_ERROR_SERVER);
    }
}

void NetHTTP::Clear()
{
    m_header.clear();
    m_body.clear();
    m_chunked = false;
    m_contentLength = 0;
    m_state = HTTP_STATE_IDLE;
    m_error = HTTP_ERROR_NONE;
}

void NetHTTP::Error(eHTTPError error)
{
    m_error = error;
    m_netSocket.CloseAllClients();
}

string EncodeURL(const string& str)
{
    string encoded;

    for(const char& c : str) {
        if(IsDigit(c) || IsAlpha(c) || c == '-' || c == '_' || c == '.' || c == '~' || c == '/' || c == '&' || c == '?' || c == '=') {
            encoded += c;
        }
        else if(c == ' ') {
            encoded += "%20";
        }
        else {
            encoded += "%"; 
            encoded += ToHex(&c, 1);
        }
    }

    return encoded;
}
