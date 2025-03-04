#ifndef SWITCHBOT_HTTP_CLIENT_H
#define SWITCHBOT_HTTP_CLIENT_H

#include <string>
#include <nlohmann/json.hpp>

class HttpClient {
public:
    HttpClient(const std::string& token, const std::string& secret);
    ~HttpClient();

    nlohmann::json Get(const std::string& endpoint);

private:
    std::string m_token;
    std::string m_secret;
    void* m_curl;  // CURL*のハンドル

    void SetupHeaders(void* request, const std::string& timestamp, const std::string& signature, const std::string& nonce);
    std::string GetTimestamp();
    void Initialize();
    void Cleanup();
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

    std::string generateUUID();
};

class HttpException : public std::runtime_error {
public:
    explicit HttpException(const std::string& message)
        : std::runtime_error(message) {}
};

#endif // SWITCHBOT_HTTP_CLIENT_H
