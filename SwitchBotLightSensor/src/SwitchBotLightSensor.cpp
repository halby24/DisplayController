#include "SwitchBotLightSensor.h"
#include "HttpClient.h"
#include "SwitchBotException.h"
#include <sstream>
#include <algorithm>

namespace {
    constexpr const char* API_BASE_URL = "https://api.switch-bot.com/v1.1/devices/";
    constexpr int MAX_RAW_BRIGHTNESS = 1000; // SwitchBotの生の照度値の最大値
}

SwitchBotLightSensor::SwitchBotLightSensor(const std::string& token, const std::string& deviceId)
    : m_token(token)
    , m_deviceId(deviceId)
    , m_httpClient(std::make_unique<HttpClient>(token))
{
    if (token.empty()) {
        throw ConfigurationException("API token cannot be empty");
    }
    if (deviceId.empty()) {
        throw ConfigurationException("Device ID cannot be empty");
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
    std::stringstream url;
    url << API_BASE_URL << m_deviceId << "/status";

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
                throw DeviceNotFoundException("Device not found: " + m_deviceId);
            default:
                throw SwitchBotException(
                    "API request failed with status code: " + std::to_string(statusCode),
                    statusCode
                );
        }
    }

    return response;
}

int SwitchBotLightSensor::NormalizeLightLevel(int rawLevel)
{
    // 0-1000の値を0-100に正規化
    return std::clamp(static_cast<int>((static_cast<double>(rawLevel) / MAX_RAW_BRIGHTNESS) * 100), 0, 100);
}
