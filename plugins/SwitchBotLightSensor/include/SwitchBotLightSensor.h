#ifndef SWITCHBOT_LIGHT_SENSOR_H
#define SWITCHBOT_LIGHT_SENSOR_H

#include <memory>
#include <string>
#include <ILightSensor.h>
#include <ConfigManager.h>
#include <nlohmann/json.hpp>

#ifdef SWITCHBOT_EXPORTS
#define SWITCHBOT_API __declspec(dllexport)
#else
#define SWITCHBOT_API __declspec(dllimport)
#endif

class HttpClient;
class SWITCHBOT_API SwitchBotLightSensor : public ILightSensor {
private:
    std::string m_token;
    std::string m_deviceId;
    int m_retryCount;
    int m_retryInterval;
    std::unique_ptr<HttpClient> m_httpClient;
    ConfigManager& m_config;
    CalibrationSettings m_calibration;

public:
    SwitchBotLightSensor(
        const std::string& token,
        const std::string& deviceId,
        int retryCount = 3,
        int retryInterval = 1000
    );
    virtual ~SwitchBotLightSensor() override;

    virtual int GetLightLevel() override;

private:
    nlohmann::json GetDeviceStatus();
    int NormalizeLightLevel(int rawLevel);
    std::string GenerateNonce();
};

#endif // SWITCHBOT_LIGHT_SENSOR_H
