#include "HttpClient.h"
#include <curl/curl.h>
#include <sstream>
#include <chrono>
#include <random>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <iostream>

// レスポンスデータを格納するコールバック関数
size_t HttpClient::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    auto* response = static_cast<std::string*>(userp);
    response->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

namespace {
    // Base64エンコード
    std::string Base64Encode(const unsigned char* input, size_t length) {
        static const std::string base64_chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";

        std::string ret;
        int i = 0;
        int j = 0;
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];

        while (length--) {
            char_array_3[i++] = *(input++);
            if (i == 3) {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;

                for(i = 0; i < 4; i++)
                    ret += base64_chars[char_array_4[i]];
                i = 0;
            }
        }

        if (i) {
            for(j = i; j < 3; j++)
                char_array_3[j] = '\0';

            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

            for (j = 0; j < i + 1; j++)
                ret += base64_chars[char_array_4[j]];

            while((i++ < 3))
                ret += '=';
        }

        return ret;
    }
}

HttpClient::HttpClient(const std::string& token, const std::string& secret)
    : m_token(token)
    , m_secret(secret)
    , m_curl(nullptr)
{
    Initialize();
}

HttpClient::~HttpClient() {
    Cleanup();
}

void HttpClient::Initialize() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    m_curl = curl_easy_init();
    if (!m_curl) {
        throw HttpException("Failed to initialize CURL");
    }
}

void HttpClient::Cleanup() {
    if (m_curl) {
        curl_easy_cleanup(m_curl);
        m_curl = nullptr;
    }
    curl_global_cleanup();
}

std::string HttpClient::GetTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return std::to_string(ms.count());
}

void HttpClient::SetupHeaders(void* request, const std::string& signature, const std::string& nonce) {
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("Authorization: Bearer " + m_token).c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, ("t: " + GetTimestamp()).c_str());
    headers = curl_slist_append(headers, ("sign: " + signature).c_str());
    headers = curl_slist_append(headers, ("nonce: " + nonce).c_str());

    curl_easy_setopt(request, CURLOPT_HTTPHEADER, headers);
}

nlohmann::json HttpClient::Get(const std::string& endpoint) {
    if (!m_curl) {
        std::cerr << "[SwitchBot] Error: CURL not initialized" << std::endl;
        throw HttpException("CURL not initialized");
    }

    std::cout << "[SwitchBot] Making API request to: " << endpoint << std::endl;

    // ランダムなnonceを生成
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 999999);
    std::string nonce = std::to_string(dis(gen));

    // 署名を生成
    std::string timestamp = GetTimestamp();
    std::string signStr = m_token + timestamp + nonce;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    HMAC(EVP_sha256(), m_secret.c_str(), m_secret.length(),
         (unsigned char*)signStr.c_str(), signStr.length(), hash, nullptr);
    std::string signature = Base64Encode(hash, SHA256_DIGEST_LENGTH);

    std::cout << "[SwitchBot] Request details:" << std::endl;
    std::cout << "  - Timestamp: " << timestamp << std::endl;
    std::cout << "  - Nonce: " << nonce << std::endl;
    std::cout << "  - Token: " << m_token.substr(0, 5) << "..." << m_token.substr(m_token.length() - 5) << std::endl;

    std::string response_string;
    curl_easy_setopt(m_curl, CURLOPT_URL, endpoint.c_str());
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &response_string);
    curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 2L);

    SetupHeaders(m_curl, signature, nonce);

    std::cout << "[SwitchBot] Sending request..." << std::endl;
    CURLcode res = curl_easy_perform(m_curl);
    if (res != CURLE_OK) {
        std::string error = curl_easy_strerror(res);
        std::cerr << "[SwitchBot] CURL request failed: " << error << std::endl;
        throw HttpException(std::string("CURL request failed: ") + error);
    }

    long http_code = 0;
    curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &http_code);
    std::cout << "[SwitchBot] HTTP response code: " << http_code << std::endl;

    if (http_code != 200) {
        std::cerr << "[SwitchBot] HTTP request failed with code: " << http_code << std::endl;
        std::cerr << "[SwitchBot] Response content: " << response_string << std::endl;

        std::string error_message = "HTTP request failed with code: " + std::to_string(http_code);
        if (http_code == 401) {
            error_message += " (Unauthorized - Please check your token and secret)";
        }
        throw HttpException(error_message);
    }

    try {
        auto json_response = nlohmann::json::parse(response_string);
        std::cout << "[SwitchBot] Response parsed successfully" << std::endl;
        return json_response;
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "[SwitchBot] Failed to parse JSON response: " << e.what() << std::endl;
        std::cerr << "[SwitchBot] Raw response: " << response_string << std::endl;
        throw HttpException(std::string("Failed to parse JSON response: ") + e.what());
    }
}
