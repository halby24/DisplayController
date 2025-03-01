#include "SwitchBotLightSensor.h"
#include "HttpClient.h"
#include "SwitchBotException.h"
#include <sstream>
#include <algorithm>

namespace {
    constexpr const char* API_BASE_URL = "https://api.switch-bot.com/v1.1/devices/";
    constexpr int MAX_RAW_BRIGHTNESS = 1000; // SwitchBotの生の照度値の最大値
}

SwitchBotLightSensor::SwitchBotLightSensor(const std::string& deviceName)
    : m_deviceName(deviceName)
    , m_config(ConfigManager::Instance())
{
    if (deviceName.empty()) {
        throw ConfigurationException("Device name cannot be empty");
    }

    try {
        // 設定を読み込み
        m_config.Load();

        // APIトークンとシークレットを取得して初期化
        std::string token = m_config.GetSwitchBotToken();
        std::string secret = m_config.GetSwitchBotSecret();
        if (token.empty()) {
            throw ConfigurationException("SwitchBot API token not configured");
        }
        if (secret.empty()) {
            throw ConfigurationException("SwitchBot API secret not configured");
        }

        // HTTPクライアントを初期化
        m_httpClient = std::make_unique<HttpClient>(token, secret);
    }
    catch (const ConfigException& e) {
        throw ConfigurationException(std::string("Failed to load configuration: ") + e.what());
    }
}

SwitchBotLightSensor::~SwitchBotLightSensor() = default;

int SwitchBotLightSensor::GetLightLevel()
{
    try {
        auto status = GetDeviceStatus();

        // brightness フィールドを取得
        if (!status["body"].contains("brightness")) {
            throw SwitchBotException("Device response does not contain brightness data");
        }

        int rawBrightness = status["body"]["brightness"].get<int>();
        return NormalizeLightLevel(rawBrightness);
    }
    catch (const HttpException& e) {
        throw SwitchBotException(std::string("Failed to get light level: ") + e.what());
    }
    catch (const nlohmann::json::exception& e) {
        throw SwitchBotException(std::string("Failed to parse device response: ") + e.what());
    }
}

nlohmann::json SwitchBotLightSensor::GetDeviceStatus()
{
    try {
        // デバイスIDを取得
        std::string deviceId = m_config.GetDeviceId(m_deviceName);

        std::stringstream url;
        url << API_BASE_URL << deviceId << "/status";

        auto response = m_httpClient->Get(url.str());

        // レスポンスのステータスコードを確認
        if (!response.contains("statusCode")) {
            throw SwitchBotException("Invalid API response format");
        }

        int statusCode = response["statusCode"].get<int>();
        if (statusCode != 100) {
            switch (statusCode) {
                case 401:
                    throw AuthenticationException("Authentication failed");
                case 404:
                    throw DeviceNotFoundException("Device not found: " + deviceId);
                default:
                    throw SwitchBotException(
                        "API request failed with status code: " + std::to_string(statusCode),
                        statusCode
                    );
            }
        }

        return response;
    }
    catch (const ConfigException& e) {
        throw SwitchBotException(std::string("Failed to get device configuration: ") + e.what());
    }
}

int SwitchBotLightSensor::NormalizeLightLevel(int rawLevel)
{
    // 0-1000の値を0-100に正規化
    return std::clamp(static_cast<int>((static_cast<double>(rawLevel) / MAX_RAW_BRIGHTNESS) * 100), 0, 100);
}
