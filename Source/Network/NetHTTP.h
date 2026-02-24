#pragma once

#include "../Precompiled.h"
#include "NetSocket.h"

/**
 * simple http made for parsing wiki
 */

#define HTTP_TIMEOUT_MS 10 * 1000

enum eHTTPError
{
    HTTP_ERROR_NONE,
    HTTP_ERROR_FAIL_CONNECT,
    HTTP_ERROR_TIME_EXCEED,
    HTTP_ERROR_CONNECT_FAIL,
    HTTP_ERROR_SOCKET,
    HTTP_ERROR_SERVER,
    HTTP_ERROR_CLIENT
};

enum eHTTPState
{
    HTTP_STATE_IDLE,
    HTTP_STATE_READ_HEAD,
    HTTP_STATE_READ_BODY,
    HTTP_STATE_COMPLETE
};

class NetHTTP {
public:
    NetHTTP();
    ~NetHTTP();

public:
    void Init(const string& server);

    void OnDataReceive(NetClient* pClient);
    void OnClientDisconnect(NetClient* pClient);
    bool Get(const string& path);

    string GetHeader() const { return m_header; }
    string GetBody() const { return m_body; }
    uint16 GetStatus() const { return m_status; }
    eHTTPError GetError() const { return m_error; }

private:
    void Update();
    void ParseHeader(const string& header);
    void Clear();
    void Error(eHTTPError error);

private:
    NetSocket m_netSocket;
    string m_server;
    string m_path;
    uint16 m_port;

    string m_header;
    string m_body;
    bool m_chunked;
    uint32 m_contentLength;
    uint16 m_status;
    
    eHTTPState m_state;
    eHTTPError m_error;
};

string EncodeURL(const string& str);