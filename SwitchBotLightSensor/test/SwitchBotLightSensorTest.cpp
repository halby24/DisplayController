#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "SwitchBotLightSensor.h"
#include "HttpClient.h"
#include "SwitchBotException.h"

using namespace testing;

// HttpClientのモッククラス
class MockHttpClient : public HttpClient {
public:
    explicit MockHttpClient(const std::string& token) : HttpClient(token) {}
    MOCK_METHOD(nlohmann::json, Get, (const std::string& endpoint), (override));
};

class SwitchBotLightSensorTest : public Test {
protected:
    const std::string TEST_TOKEN = "test-token";
    const std::string TEST_DEVICE_ID = "test-device";
    std::unique_ptr<MockHttpClient> mockClient;
    std::unique_ptr<SwitchBotLightSensor> sensor;

    void SetUp() override {
        mockClient = std::make_unique<MockHttpClient>(TEST_TOKEN);
        sensor = std::make_unique<SwitchBotLightSensor>(TEST_TOKEN, TEST_DEVICE_ID);
    }
};

// 正常系テスト
TEST_F(SwitchBotLightSensorTest, GetLightLevel_Success) {
    // 正常なレスポンスを設定
    nlohmann::json response = {
        {"statusCode", 100},
        {"body", {
            {"deviceId", "test-device"},
            {"deviceType", "WoLight"},
            {"brightness", 500}  // 50%の明るさ
        }},
        {"message", "success"}
    };

    // モックの振る舞いを設定
    EXPECT_CALL(*mockClient, Get(_))
        .WillOnce(Return(response));

    // テスト実行
    int level = sensor->GetLightLevel();

    // 結果の検証（500/1000 * 100 = 50）
    EXPECT_EQ(level, 50);
}

// エラー系テスト - 認証エラー
TEST_F(SwitchBotLightSensorTest, GetLightLevel_AuthenticationError) {
    nlohmann::json response = {
        {"statusCode", 401},
        {"message", "Authentication failed"}
    };

    EXPECT_CALL(*mockClient, Get(_))
        .WillOnce(Return(response));

    EXPECT_THROW({
        try {
            sensor->GetLightLevel();
        }
        catch (const AuthenticationException& e) {
            EXPECT_STREQ(e.what(), "Authentication failed");
            throw;
        }
    }, AuthenticationException);
}

// エラー系テスト - デバイスが見つからない
TEST_F(SwitchBotLightSensorTest, GetLightLevel_DeviceNotFound) {
    nlohmann::json response = {
        {"statusCode", 404},
        {"message", "Device not found"}
    };

    EXPECT_CALL(*mockClient, Get(_))
        .WillOnce(Return(response));

    EXPECT_THROW({
        try {
            sensor->GetLightLevel();
        }
        catch (const DeviceNotFoundException& e) {
            EXPECT_THAT(e.what(), HasSubstr("Device not found"));
            throw;
        }
    }, DeviceNotFoundException);
}

// エラー系テスト - 不正なレスポンス形式
TEST_F(SwitchBotLightSensorTest, GetLightLevel_InvalidResponse) {
    nlohmann::json response = {
        {"statusCode", 100},
        {"body", {}}  // brightnessフィールドがない
    };

    EXPECT_CALL(*mockClient, Get(_))
        .WillOnce(Return(response));

    EXPECT_THROW({
        try {
            sensor->GetLightLevel();
        }
        catch (const SwitchBotException& e) {
            EXPECT_THAT(e.what(), HasSubstr("does not contain brightness data"));
            throw;
        }
    }, SwitchBotException);
}

// 値の正規化テスト
TEST_F(SwitchBotLightSensorTest, GetLightLevel_Normalization) {
    // テストケース: 最大値、最小値、中間値
    std::vector<std::pair<int, int>> testCases = {
        {0, 0},      // 最小値
        {1000, 100}, // 最大値
        {500, 50},   // 中間値
        {750, 75},   // 75%
        {250, 25}    // 25%
    };

    for (const auto& [input, expected] : testCases) {
        nlohmann::json response = {
            {"statusCode", 100},
            {"body", {
                {"deviceId", "test-device"},
                {"deviceType", "WoLight"},
                {"brightness", input}
            }},
            {"message", "success"}
        };

        EXPECT_CALL(*mockClient, Get(_))
            .WillOnce(Return(response));

        int level = sensor->GetLightLevel();
        EXPECT_EQ(level, expected)
            << "Input: " << input << ", Expected: " << expected << ", Actual: " << level;
    }
}

// 設定エラーテスト
TEST_F(SwitchBotLightSensorTest, Constructor_EmptyToken) {
    EXPECT_THROW({
        SwitchBotLightSensor sensor("", TEST_DEVICE_ID);
    }, ConfigurationException);
}

TEST_F(SwitchBotLightSensorTest, Constructor_EmptyDeviceId) {
    EXPECT_THROW({
        SwitchBotLightSensor sensor(TEST_TOKEN, "");
    }, ConfigurationException);
}
