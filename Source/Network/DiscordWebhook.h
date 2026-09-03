#pragma once

#include "../Utils/GameConfig.h"
#include "NetHTTP.h"
#include <ctime>

struct DiscordAttachment
{
    string fileName;
    std::vector<uint8> buffer;
};

struct DiscordEmbedAuthor
{
    string name;
    string url;
    string iconUrl;
};

struct DiscordEmbedField
{
    string name;
    string value;
    bool isInline = false;
};

struct DiscordEmbed
{
    string title;
    string description;
    string url;
    uint32 color = 0x000000;

    DiscordEmbedAuthor author;

    string footerText;
    string footerIconUrl;

    string imageUrl;
    string thumbnailUrl;
    string timestamp;

    std::vector<DiscordEmbedField> fields;

    void SetAuthor(const DiscordEmbedAuthor& embedAuthor) { author = embedAuthor; }

    void AddField(DiscordEmbedField&& field) { fields.push_back(std::move(field)); }
    void AddField(const DiscordEmbedField& field) { fields.push_back(field); }

    void SetTimestampToNow()
    {
        time_t currentTime = std::time(nullptr);
        tm timeInfo{};
        GetTimeUTC(&timeInfo, &currentTime);

        char buffer[32];
        strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &timeInfo);
        timestamp = buffer;
    }
};

class DiscordWebhook
{
public:
    DiscordWebhook();
    DiscordWebhook(const string& webhookUrl);
    ~DiscordWebhook();

public:
    void SetWebhookUrl(const string& webhookUrl);

    DiscordWebhook& SetUsername(const string& username);
    DiscordWebhook& SetAvatarUrl(const string& avatarUrl);
    DiscordWebhook& SetContent(const string& content);

    DiscordWebhook& AddEmbed(const DiscordEmbed& embed);
    DiscordWebhook& AddAttachment(const string& fileName, const std::vector<uint8>& buffer);
    DiscordWebhook& AddAttachmentFromFile(const string& filePath);

    bool Send();
    bool SendContent(const string& content);

    void Clear();

    string GetLastResponse() const { return m_lastResponse; }
    uint16 GetLastStatus() const { return m_lastStatus; }

private:
    string EscapeJSON(const string& str);
    string BuildJSONPayload();
    string BuildMultipartPayload(const string& boundary);

private:
    string m_webhookUrl;
    string m_host;
    string m_path;

    string m_username;
    string m_avatarUrl;
    string m_content;

    std::vector<DiscordEmbed> m_embeds;
    std::vector<DiscordAttachment> m_attachments;

    NetHTTP m_http;
    string m_lastResponse;
    uint16 m_lastStatus;
};

class DiscordWebhookManager
{
public:
    DiscordWebhookManager();
    ~DiscordWebhookManager();

public:
    static DiscordWebhookManager* GetInstance()
    {
        static DiscordWebhookManager instance;
        return &instance;
    }

public:
    void Kill();

    void RegisterFromConfig(std::vector<DiscordWebhookConfigSchema>& webhooks, eConfigServerType serverType);

    bool Register(const string& customID, const string& webhookUrl);
    DiscordWebhook* GetWebhook(const string& customID);

    uint32 GetWebhookCount() { return m_webhooks.size(); }

private:
    std::unordered_map<string, DiscordWebhook*> m_webhooks;
};

DiscordWebhookManager* GetDiscordWebhookManager();