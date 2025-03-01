#include "src/MonitorController.h"
#include <iostream>

int main() {
    try {
        // テスト用の設定を作成
        MappingConfig testConfig;
        testConfig.minBrightness = 10;
        testConfig.maxBrightness = 90;
        testConfig.mappingPoints = {
            {20, 30},
            {50, 60}
        };

        // MonitorControllerのインスタンスを作成
        MonitorController controller;

        // テスト用のモニターID（ダミー値）
        MonitorId testId = reinterpret_cast<MonitorId>(1);

        // 設定を保存
        std::cout << "Saving test configuration...\n";
        controller.SetMappingConfig(testId, testConfig);

        // 設定を読み込み
        std::cout << "Loading test configuration...\n";
        auto loadedConfig = controller.GetMappingConfig(testId);

        // 読み込んだ設定を検証
        std::cout << "Verifying loaded configuration...\n";
        bool success = true;

        if (loadedConfig.minBrightness != testConfig.minBrightness) {
            std::cout << "Error: minBrightness mismatch. Expected "
                      << testConfig.minBrightness << ", got "
                      << loadedConfig.minBrightness << "\n";
            success = false;
        }

        if (loadedConfig.maxBrightness != testConfig.maxBrightness) {
            std::cout << "Error: maxBrightness mismatch. Expected "
                      << testConfig.maxBrightness << ", got "
                      << loadedConfig.maxBrightness << "\n";
            success = false;
        }

        if (loadedConfig.mappingPoints.size() != testConfig.mappingPoints.size()) {
            std::cout << "Error: mappingPoints size mismatch. Expected "
                      << testConfig.mappingPoints.size() << ", got "
                      << loadedConfig.mappingPoints.size() << "\n";
            success = false;
        } else {
            for (size_t i = 0; i < testConfig.mappingPoints.size(); ++i) {
                if (loadedConfig.mappingPoints[i] != testConfig.mappingPoints[i]) {
                    std::cout << "Error: mappingPoint[" << i << "] mismatch. Expected {"
                              << testConfig.mappingPoints[i].first << ", "
                              << testConfig.mappingPoints[i].second << "}, got {"
                              << loadedConfig.mappingPoints[i].first << ", "
                              << loadedConfig.mappingPoints[i].second << "}\n";
                    success = false;
                }
            }
        }

        if (success) {
            std::cout << "Test passed: JSON save/load working correctly!\n";
        } else {
            std::cout << "Test failed: JSON save/load has issues.\n";
        }

        return success ? 0 : 1;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
