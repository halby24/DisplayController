#include "DummyLightSensor.h"
#include <algorithm>

DummyLightSensor::DummyLightSensor(int defaultValue)
    : m_defaultValue(std::clamp(defaultValue, 0, 100))  // 値を0-100の範囲に制限
{
}

int DummyLightSensor::GetLightLevel()
{
    return m_defaultValue;
}
