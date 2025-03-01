#ifndef SWITCHBOT_LIGHT_SENSOR_H
#define SWITCHBOT_LIGHT_SENSOR_H

#include <memory>
#include <string>
#include "../../src/ILightSensor.h"
#include "../../src/ConfigManager.h"
#include <nlohmann/json.hpp>

#ifdef SWITCHBOT_EXPORTS
#define SWITCHBOT_API __declspec(dllexport)
#else
#define SWITCHBOT_API __declspec(dllimport)
#endif

class HttpClient;
class SWITCHBOT_API SwitchBotLightSensor : public ILightSensor {
private:
    std::string m_deviceName;
    std::unique_ptr<HttpClient> m_httpClient;
    ConfigManager& m_config;

public:
    explicit SwitchBotLightSensor(const std::string& deviceName);
    virtual ~SwitchBotLightSensor() override;

    virtual int GetLightLevel() override;

private:
    nlohmann::json GetDeviceStatus();
    int NormalizeLightLevel(int rawLevel);
    std::string GenerateNonce();
};

#endif // SWITCHBOT_LIGHT_SENSOR_H
