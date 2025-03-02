#ifndef DUMMY_LIGHT_SENSOR_H
#define DUMMY_LIGHT_SENSOR_H

#include "ILightSensor.h"

/**
 * @brief テストおよびフォールバック用のダミー照度センサー
 *
 * このクラスは、実際のハードウェアセンサーの代わりに使用される
 * ダミーの照度センサーを実装します。設定された固定値を返します。
 */
class DummyLightSensor : public ILightSensor {
public:
    /**
     * @brief コンストラクタ
     * @param defaultValue 返却する照度値（0-100）
     */
    explicit DummyLightSensor(int defaultValue = 50);

    /**
     * @brief 現在の照度レベルを取得
     * @return 0-100の範囲の照度値
     */
    int GetLightLevel() override;

private:
    int m_defaultValue{50};  // 返却する固定値（デフォルト50）
};

#endif // DUMMY_LIGHT_SENSOR_H
