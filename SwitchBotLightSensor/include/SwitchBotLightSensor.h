#ifndef SWITCHBOT_LIGHT_SENSOR_H
#define SWITCHBOT_LIGHT_SENSOR_H

#include <memory>
#include <string>
#include "../../src/ILightSensor.h"
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
    std::unique_ptr<HttpClient> m_httpClient;

public:
    SwitchBotLightSensor(const std::string& token, const std::string& deviceId);
    virtual ~SwitchBotLightSensor() override;

    virtual int GetLightLevel() override;

private:
    nlohmann::json GetDeviceStatus();
    int NormalizeLightLevel(int rawLevel);
    std::string GenerateSignature();
    std::string GenerateNonce();
};

#endif // SWITCHBOT_LIGHT_SENSOR_H
