#ifndef DISPLAYCONTROLLER_DUMMYLIGHTSENSOR_H
#define DISPLAYCONTROLLER_DUMMYLIGHTSENSOR_H

#include "ILightSensor.h"
#include <random>

class DummyLightSensor : public ILightSensor {
public:
    DummyLightSensor();
    int GetLightLevel() override;

private:
    std::random_device m_rd;
    std::mt19937 m_gen;
    std::uniform_int_distribution<> m_dist;
};

#endif // DISPLAYCONTROLLER_DUMMYLIGHTSENSOR_H
