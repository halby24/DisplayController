#include "DummyLightSensor.h"

DummyLightSensor::DummyLightSensor()
    : m_gen(m_rd())
    , m_dist(20, 80)  // 20-80の範囲で変動する値を生成
{
}

int DummyLightSensor::GetLightLevel()
{
    return m_dist(m_gen);
}
