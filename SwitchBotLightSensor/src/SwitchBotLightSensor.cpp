#include "SwitchBotLightSensor.h"
#include "HttpClient.h"
#include "SwitchBotException.h"
#include "../../src/StringUtils.h"
#include <sstream>
#include <algorithm>
#include <iostream>

namespace {
    constexpr const char* API_BASE_URL = "https://api.switch-bot.com/v1.1/devices/";
}

SwitchBotLightSensor::SwitchBotLightSensor(const std::string& deviceName)
    : m_deviceName(deviceName)
    , m_config(ConfigManager::Instance())
{
    if (deviceName.empty()) {
        StringUtils::OutputErrorMessage("[SwitchBot] Error: Device name cannot be empty");
        throw ConfigurationException("デバイス名を指定してください");
    }

    try {
        std::cout << "[SwitchBot] Initializing device: " << deviceName << std::endl;

        // 設定を読み込み
        m_config.Load();
        std::cout << "[SwitchBot] Configuration loaded successfully" << std::endl;

        // キャリブレーション設定を読み込み
        try {
            m_calibration = m_config.GetDeviceCalibration(deviceName);
            std::cout << "[SwitchBot] Calibration settings loaded: min="
                      << m_calibration.minRawValue << ", max="
                      << m_calibration.maxRawValue << std::endl;
        }
        catch (const ConfigException& e) {
            StringUtils::OutputMessage(
                "[SwitchBot] Warning: Using default calibration settings - " +
                std::string(e.what())
            );
            // デフォルト値を使用
            m_calibration = CalibrationSettings();
        }

        // APIトークンとシークレットを取得して初期化
        std::string token = m_config.GetSwitchBotToken();
        std::string secret = m_config.GetSwitchBotSecret();
        if (token.empty()) {
            StringUtils::OutputErrorMessage("[SwitchBot] Error: API token not configured");
            throw ConfigurationException("SwitchBot APIトークンが設定されていません");
        }
        if (secret.empty()) {
            StringUtils::OutputErrorMessage("[SwitchBot] Error: API secret not configured");
            throw ConfigurationException("SwitchBot APIシークレットが設定されていません");
        }

        // HTTPクライアントを初期化
        m_httpClient = std::make_unique<HttpClient>(token, secret);
        std::cout << "[SwitchBot] HTTP client initialized successfully" << std::endl;
    }
    catch (const ConfigException& e) {
        StringUtils::OutputExceptionMessage(e);
        throw ConfigurationException(std::string("設定の読み込みに失敗しました: ") + e.what());
    }
}

SwitchBotLightSensor::~SwitchBotLightSensor() = default;

int SwitchBotLightSensor::GetLightLevel()
{
    try {
        std::cout << "[SwitchBot] Getting light level for device: " << m_deviceName << std::endl;

        auto status = GetDeviceStatus();
        std::cout << "[SwitchBot] Device status retrieved successfully" << std::endl;

        // lightLevel フィールドを取得
        if (!status["body"].contains("lightLevel")) {
            StringUtils::OutputErrorMessage("[SwitchBot] Error: Response does not contain light level data");
            throw SwitchBotException("デバイスの応答に照度データが含まれていません");
        }

        int rawBrightness = status["body"]["lightLevel"].get<int>();
        std::cout << "[SwitchBot] Raw brightness value: " << rawBrightness << std::endl;

        int normalizedBrightness = NormalizeLightLevel(rawBrightness);
        std::cout << "[SwitchBot] Normalized brightness value (0-100): " << normalizedBrightness << std::endl;

        return normalizedBrightness;
    }
    catch (const HttpException& e) {
        StringUtils::OutputErrorMessage("[SwitchBot] HTTP Error: " + std::string(e.what()));
        throw SwitchBotException(std::string("照度の取得に失敗しました: ") + e.what());
    }
    catch (const nlohmann::json::exception& e) {
        StringUtils::OutputErrorMessage("[SwitchBot] JSON Parse Error: " + std::string(e.what()));
        throw SwitchBotException(std::string("デバイスの応答の解析に失敗しました: ") + e.what());
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
            throw SwitchBotException("APIレスポンスの形式が不正です");
        }

        int statusCode = response["statusCode"].get<int>();
        if (statusCode != 100) {
            switch (statusCode) {
                case 401:
                    StringUtils::OutputErrorMessage("[SwitchBot] Authentication Error: Failed to authenticate with API");
                    throw AuthenticationException("APIの認証に失敗しました");
                case 404:
                    StringUtils::OutputErrorMessage("[SwitchBot] Device Error: Device not found - " + deviceId);
                    throw DeviceNotFoundException("デバイスが見つかりません: " + deviceId);
                default:
                    StringUtils::OutputErrorMessage("[SwitchBot] API Error: Request failed with status code " + std::to_string(statusCode));
                    throw SwitchBotException(
                        "APIリクエストが失敗しました（ステータスコード: " + std::to_string(statusCode) + "）",
                        statusCode
                    );
            }
        }

        return response;
    }
    catch (const ConfigException& e) {
        StringUtils::OutputErrorMessage("[SwitchBot] Configuration Error: " + std::string(e.what()));
        throw SwitchBotException(std::string("Failed to get device configuration: ") + e.what());
    }
}

int SwitchBotLightSensor::NormalizeLightLevel(int rawLevel)
{
    // キャリブレーション設定に基づいて0-100にマッピング
    int clampedRaw = std::clamp(
        rawLevel,
        m_calibration.minRawValue,
        m_calibration.maxRawValue
    );

    double normalizedValue = (static_cast<double>(clampedRaw - m_calibration.minRawValue) /
        (m_calibration.maxRawValue - m_calibration.minRawValue)) * 100.0;

    return std::clamp(static_cast<int>(normalizedValue), 0, 100);
}
