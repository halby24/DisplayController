#ifndef DISPLAYCONTROLLER_ILIGHTSENSOR_H
#define DISPLAYCONTROLLER_ILIGHTSENSOR_H

// DLLエクスポート/インポートマクロ
#ifdef _WIN32
    #ifdef LIGHTSENSOR_EXPORTS
        #define LIGHTSENSOR_API __declspec(dllexport)
    #else
        #define LIGHTSENSOR_API __declspec(dllimport)
    #endif
#else
    #define LIGHTSENSOR_API
#endif

class LIGHTSENSOR_API ILightSensor {
public:
    virtual ~ILightSensor() = default;

    /**
     * @brief 現在の照度レベルを取得
     * @return 0-100の範囲の照度値
     */
    virtual int GetLightLevel() = 0;
};

#endif // DISPLAYCONTROLLER_ILIGHTSENSOR_H
