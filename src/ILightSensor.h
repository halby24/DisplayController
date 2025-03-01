#ifndef DISPLAYCONTROLLER_ILIGHTSENSOR_H
#define DISPLAYCONTROLLER_ILIGHTSENSOR_H

class ILightSensor {
public:
    virtual ~ILightSensor() = default;

    /**
     * @brief 現在の照度レベルを取得
     * @return 0-100の範囲の照度値
     */
    virtual int GetLightLevel() = 0;
};

#endif // DISPLAYCONTROLLER_ILIGHTSENSOR_H
