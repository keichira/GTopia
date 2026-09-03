#pragma once

#include "../IO/File.h"
#include "../Precompiled.h"
#include <map>

#if !defined(_WIN32) && !defined(NETHTTP_USE_CURL)
#define NETHTTP_USE_SOCKET 1
#define HTTP_TIMEOUT_MS 12 * 1000
#define HTTP_CLIENT_CONNECT_MS 5 * 1000

#include "NetSocket.h"
#endif

string EncodeURL(const string& str);

enum eHTTPError
{
    HTTP_ERROR_NONE,
    HTTP_ERROR_FAIL_CONNECT,
    HTTP_ERROR_TIME_EXCEED,
    HTTP_ERROR_CONNECT_FAIL,
    HTTP_ERROR_WRITE_FILE,
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

class NetHTTP
{
public:
    NetHTTP();
    ~NetHTTP();

    void Init(const string& server);
    void Kill();
    void Clear();

    bool Get(const string& path);
    bool Post(const string& path);

    void SetHeader(const string& key, const string& value) { m_headers[key] = value; }
    void SetBody(const string& body) { m_postData = body; }
    void AddPostData(const string& key, const string& value);
    bool SetOutputFile(const string& filePath);

    string GetHeader() const { return m_header; }
    string GetBody() const { return m_body; }
    uint16 GetStatus() const { return m_status; }
    eHTTPError GetError() const { return m_error; }

    void HandleDataReceive(const char* data, uint32 size);
    void HandleHeaderReceive(const char* data, uint32 size);

private:
    bool ExecuteRequest(const string& method, const string& path);
    void Error(eHTTPError error);

#if NETHTTP_USE_SOCKET
    void OnConnect(NetClient* pClient);
    void OnDisconnect(NetClient* pClient);
    void OnDataReceive(NetClient* pClient);
    void ParseHeader(const string& header);
    void UpdateSocketRequest(const string& requestToSend);
#endif

private:
    string m_server;
    uint16 m_port;
    bool m_isSSL;

    string m_header;
    string m_body;
    string m_postData;
    uint16 m_status;

    std::map<string, string> m_headers;

    File m_file;
    eHTTPError m_error;

#ifdef NETHTTP_USE_SOCKET
    NetSocket m_netSocket;
    NetClient* m_pNetClient;
    eHTTPState m_state;
    bool m_chunked;
    uint32 m_contentLength;
#endif
};