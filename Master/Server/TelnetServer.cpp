#include "TelnetServer.h"
#include "IO/File.h"
#include "IO/Log.h"
#include "Utils/StringUtils.h"
#include "Utils/ConfigDB.h"

#include "../Command/Telnet/TelnetCommandBase.h"
#include "../Command/Telnet/SetRole.h"

TelnetClient::TelnetClient(NetClient* pClient)
: NetEntity(ENTITY_TYPE_TELNET), m_adminLevel(0), m_displayName("Unknown"), m_authed(false), m_isBusy(false)
{
    m_pClient = pClient;
}

TelnetClient::~TelnetClient()
{
}

void TelnetClient::SendMessage(const string& message, bool line)
{
    if(!m_pClient || message.empty()) {
        return;
    }

    string msgToSend = message;

    if(line) {
        msgToSend += "\r\n";
    }

    if(m_authed) {
        msgToSend += "> ";
    }

    m_pClient->Send(msgToSend.data(), msgToSend.size());
}

TelnetServer::TelnetServer()
: m_pNetSocket(nullptr), m_port(0), m_skipIPCheck(false)
{
}

TelnetServer::~TelnetServer()
{
}

bool TelnetServer::Init()
{
    SAFE_DELETE(m_pNetSocket);

    if(m_host.empty() || m_port == 0) {
        return false;
    }

    m_pNetSocket = new NetSocket();

    if(!m_pNetSocket->Init(m_host, m_port, 100)) {
        return false;
    }

    m_pNetSocket->GetEvents().Register(
        SOCKET_EVENT_TYPE_RECEIVE,
        Delegate<NetClient*>::Create<TelnetServer, &TelnetServer::OnClientReveice>(this)
    );

    m_pNetSocket->GetEvents().Register(
        SOCKET_EVENT_TYPE_CONNECT,
        Delegate<NetClient*>::Create<TelnetServer, &TelnetServer::OnClientConnect>(this)
    );

    m_pNetSocket->GetEvents().Register(
        SOCKET_EVENT_TYPE_DISCONNECT,
        Delegate<NetClient*>::Create<TelnetServer, &TelnetServer::OnClientDisconnect>(this)
    );

    RegisterCommands();

    return true;
}

void TelnetServer::RegisterCommands()
{
    RegisterCommand<SetRole>();
}

void TelnetServer::Kill()
{
    SAFE_DELETE(m_pNetSocket);

    for(auto& [_, pNetClient] : m_clients) {
        SAFE_DELETE(pNetClient);
    }
    m_clients.clear();
}

void TelnetServer::Update()
{
    if(!m_pNetSocket) {
        return;
    }

    m_pNetSocket->Update(false);

    if(m_lastClientUpdateTime.GetElapsedTime() <= 5000) {
        return;
    }

    for(auto& [_, pNetClient] : m_clients) {
        if(!pNetClient) {
            return;
        }

        if(
            pNetClient->IsAuthed() && pNetClient->GetLastActionTime().GetElapsedTime() >= 3600 * 1000 ||
            !pNetClient->IsAuthed() && pNetClient->GetLastActionTime().GetElapsedTime() >= 5 * 60 * 1000
        ) {
            pNetClient->CloseConnection();
        }
    }

    m_lastClientUpdateTime.Reset();
}

void TelnetServer::OnClientConnect(NetClient* pClient)
{
    if(!pClient) {
        return;
    }

    if(pClient->ip.empty() || pClient->data) {
        pClient->status = SOCKET_CLIENT_CLOSE;
    }

    if(IsIPRateLimited(pClient->ip)) {
        LOGGER_LOG_INFO("[Telnet] Client IP: %s rate limited closing connection", pClient->ip.c_str());
        pClient->status = SOCKET_CLIENT_CLOSE;
        return;
    }

    LOGGER_LOG_INFO("[Telnet] Client IP: %s connected to NetSocket", pClient->ip.c_str());
    if(!m_skipIPCheck && (pClient->ip.empty() || !IsTrustedIP(pClient->ip))) {
        LOGGER_LOG_WARN("[Telnet] Client IP: %s is not trusted! closing connection", pClient->ip.c_str());
        pClient->status = SOCKET_CLIENT_CLOSE;
        return;
    }

    TelnetClient* pNetClient = new TelnetClient(pClient);
    pClient->data = pNetClient;

    m_clients.insert_or_assign(pNetClient->GetNetID(), pNetClient);

    pNetClient->SendMessage("\r\n****************************************\r\n", true);
    pNetClient->SendMessage("*        GTOPIA ADMIN SERVER         *\r\n", true);
    pNetClient->SendMessage("****************************************\r\n\r\n", true);
    pNetClient->SendMessage("Please enter your password to login: ", false);
}

void TelnetServer::OnClientReveice(NetClient* pClient)
{
    if(!pClient) {
        return;
    }

    TelnetClient* pNetClient = (TelnetClient*)pClient->data;
    if(!pNetClient) {
        pClient->status = SOCKET_CLIENT_CLOSE;
        return;
    }

    uint32 dataSize = pClient->recvQueue.GetDataSize();
    if(dataSize < 3) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(pClient->recvMutex);

        char c;
        while(pClient->recvQueue.GetDataSize() > 0) {
            pClient->recvQueue.Read(&c, 1);
    
            if(c == '\n') {
                string& inputBuffer = pNetClient->GetInputBuffer();
                if(!inputBuffer.empty() && inputBuffer.back() == '\r') {
                    inputBuffer.pop_back();
                }
    
                if(!inputBuffer.empty()) {
                    HandleCommand(pNetClient, inputBuffer);
                    inputBuffer.clear();
                }
                continue;
            }
    
            if(c == '\b' || c == 0x7F) {
                if(!pNetClient->GetInputBuffer().empty()) {
                    pNetClient->GetInputBuffer().pop_back();
                }
                continue;
            }
    
            if(c >= 32 && c <= 126) {
                pNetClient->GetInputBuffer().push_back(c);
            }
        }
    }
}

void TelnetServer::OnClientDisconnect(NetClient* pClient)
{
    if(!pClient) {
        return;
    }

    TelnetClient* pNetClient = (TelnetClient*)pClient->data;
    if(!pNetClient) {
        return;
    }

    RemoveClient(pNetClient->GetNetID());
}

bool TelnetServer::LoadTelnetConfigFromFile(const string& filePath)
{
    ConfigDB cfg;
    if(!cfg.Load(filePath))
        return false;

    for(auto& line : cfg.Lines())
    {
        const string& key = line.GetString(0);

        if(key == "telnet_host")
        {
            if(!line.Require(1))
                return false;

            m_host = line.GetString(1);
        }

        if(key == "telnet_port")
        {
            if(!line.Require(1))
                return false;

            m_port = line.GetUInt(1);
        }

        if(key == "skip_ip_check")
        {
            uint32 val = line.GetUInt(1);
            m_skipIPCheck = val == 0 ? 0 : 1;
        }

        if(key == "add_account")
        {
            if(!line.Require(3))
                return false;

            TelnetClientConfig config;
            config.displayName = line.GetString(1);
            config.password = line.GetString(2);
            config.adminLevel = line.GetInt(3);

            m_clientConfig.push_back(std::move(config));
        }

        if(key == "allow_ip")
        {
            if(m_clientConfig.empty())
                continue;

            for(uint8 i = 1; i < line.GetArgSize(); ++i)
            {
                const string& ip = line.GetString(i);
                if(ip.empty())
                    continue;

                m_clientConfig.back().allowedIPs.push_back(ip);

                if(!m_skipIPCheck)
                {
                    bool ipExists = false;
                    for(auto& trustedIP : m_trustedIPs) 
                    {
                        if(trustedIP == ip) 
                        {
                            ipExists = true;
                            break;
                        }
                    }

                    if(!ipExists)
                    {
                        m_trustedIPs.push_back(ip);
                    }
                }
            }
        }
    }

    return true;
}

TelnetClientConfig* TelnetServer::GetClientConfigByPassword(const string& password)
{
    for(auto& client : m_clientConfig) {
        if(client.password == password) {
            return &client;
        }
    }

    return nullptr;
}

TelnetClient* TelnetServer::GetClientByName(const string& name)
{
    for(auto& [_, pNetClient] : m_clients) {
        if(!pNetClient) {
            continue;
        }

        if(pNetClient->GetDisplayName() == name) {
            return pNetClient;
        }
    }

    return nullptr;
}

TelnetClient* TelnetServer::GetClientByNetID(uint32 netID)
{
    auto it = m_clients.find(netID);
    if(it != m_clients.end()) {
        return it->second;
    }

    return nullptr;
}

void TelnetServer::RemoveClient(uint32 netID)
{
    auto it = m_clients.find(netID);
    if(it == m_clients.end())
        return;

    TelnetClient* pNetClient = it->second;
    if(!pNetClient)
        return;

    if(pNetClient->IsAuthed()) 
    {
        LOGGER_LOG_INFO("[Telnet] Closing connection between IP: %s Name: %s", pNetClient->GetIP().c_str(), pNetClient->GetDisplayName().c_str())
    }

    pNetClient->CloseConnection();
    SAFE_DELETE(pNetClient);
    m_clients.erase(it);
}

bool TelnetServer::IsTrustedIP(const string& ip)
{
    for(auto& trustedIP : m_trustedIPs) {
        if(trustedIP == ip) {
            return true;
        }
    }

    return false;
}

bool TelnetServer::IsIPRateLimited(const string& ip)
{
    auto it = m_rateLimits.find(ip);
    if(it == m_rateLimits.end()) {
        return false;
    }

    if(it->second.GetElapsedTime() <= 20 * 1000) {
        return true;
    }

    m_rateLimits.erase(it);
    return false;
}

void TelnetServer::ApplyRateLimit(const string& ip)
{
    auto it = m_rateLimits.find(ip);
    if(it != m_rateLimits.end()) {
        it->second.Reset();
        return;
    }

    m_rateLimits.insert_or_assign(ip, Timer());
}

void TelnetServer::HandleCommand(TelnetClient* pNetClient, const string& command)
{
    if(!pNetClient || command.empty() || command.size() > 150) {
        return;
    }

    if(pNetClient->IsAuthed() && pNetClient->GetLastActionTime().GetElapsedTime() <= 2 * 1000) {
        pNetClient->SendMessage("Slowdown, you are executing commands too fast!", true);
        return;
    }
    pNetClient->GetLastActionTime().Reset();

    if(pNetClient->IsBusy()) {
        pNetClient->SendMessage("Currently theres ongoing situation please wait it to be completed.", true);
        return;
    }

    if(!pNetClient->IsAuthed()) {
        if(pNetClient->GetPassTryCount() > 4) {
            ApplyRateLimit(pNetClient->GetIP());
            RemoveClient(pNetClient->GetNetID());
            LOGGER_LOG_WARN("[Telnet] Client IP: %s, failed to login because of incorrect password, closing connection", pNetClient->GetIP());
            return;
        }

        if(command.find(" ") != string::npos) {
            pNetClient->SendMessage("Oops, your password seems wrong!\r\nPlease enter your password to login: ", false);
            pNetClient->IncreasePassTry();
            return;
        }

        TelnetClientConfig* pClientConfig = GetClientConfigByPassword(command);
        if(!pClientConfig) {
            pNetClient->SendMessage("Oops, your password seems wrong!\r\nPlease enter your password to login: ", false);
            pNetClient->IncreasePassTry();
            return;
        }

        if(!m_skipIPCheck && !pClientConfig->IsTrustedIP(pNetClient->GetIP())) {
            LOGGER_LOG_WARN("[Telnet] Client %s tried to login to %s but IP is not trusted", pNetClient->GetIP().c_str(), pClientConfig->displayName.c_str())
            pNetClient->SendMessage("Oops, your password seems wrong!\r\nPlease enter your password to login: ", false);
            pNetClient->IncreasePassTry();
            return;
        }

        TelnetClient* pTarget = GetClientByName(pClientConfig->displayName);
        if(pTarget) {
            LOGGER_LOG_WARN("[Telnet] Client %s tried to login to %s but account already online in %s", pNetClient->GetIP().c_str(), pClientConfig->displayName.c_str(), pTarget->GetIP().c_str());
            pTarget->SendMessage("\r\n\r\n!WARNING!\r\nSomeone tried to login to your account, if it wasnt you tell it to developers ASAP!\r\n\r\n", true);
            pNetClient->SendMessage("Sorry this account is already active, if its not you tell it to developers.", true);
            pNetClient->CloseConnection();
            return;
        }

        pNetClient->SetDisplayName(pClientConfig->displayName);
        pNetClient->SetAdminLevel(pClientConfig->adminLevel);
        pNetClient->SetAuthed(true);

        pNetClient->SendMessage("\r\nWelcome back " + pNetClient->GetDisplayName() + ", listening you. (/help for help list): ", true);
        LOGGER_LOG_INFO("[Telnet] Client IP: %s Name: %s", pNetClient->GetIP().c_str(), pNetClient->GetDisplayName().c_str());
        return;
    }

    auto args = Split(command, ' ');
    ExecuteCommand(pNetClient, args);
}

void TelnetServer::ExecuteCommand(TelnetClient* pNetClient, std::vector<string>& args)
{
    if(!pNetClient) {
        return;
    }

    if(pNetClient->GetAdminLevel() == 0) {
        pNetClient->SendMessage("Unknown command.", true);
    }
    
    if(args.empty()) {
        pNetClient->SendMessage("Unknown command.", true);
        return;
    }

    uint32 hashCmd = HashString(args[0].substr(1));
    if(!m_commands.HasHandler(hashCmd)) {
        pNetClient->SendMessage("Unknown command.", true);
        return;
    }

    LOGGER_LOG_INFO("[Telnet] Client IP: %s Name: %s executed: %s", pNetClient->GetIP().c_str(), pNetClient->GetDisplayName().c_str(), JoinString(args, " ").c_str());

    m_commands.Dispatch(
        hashCmd,
        pNetClient, args
    );
}

TelnetServer* GetTelnetServer() { return TelnetServer::GetInstance(); }
