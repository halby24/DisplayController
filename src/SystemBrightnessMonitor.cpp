#include "SystemBrightnessMonitor.h"
#include <comdef.h>
#include <iostream>
#include <sstream>

#pragma comment(lib, "wbemuuid.lib")

SystemBrightnessMonitor::SystemBrightnessMonitor()
    : m_pWbemServices(nullptr)
    , m_pWbemLocator(nullptr)
    , m_wmiInitialized(false)
    , m_isMonitoring(false)
{
    try {
        InitializeWMI();
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to initialize WMI: " << e.what() << std::endl;
        throw;
    }
}

SystemBrightnessMonitor::SystemBrightnessMonitor(IWbemServices* pWbemServices)
    : m_pWbemServices(pWbemServices)
    , m_pWbemLocator(nullptr)
    , m_wmiInitialized(true)
    , m_isMonitoring(false)
{
    if (!m_pWbemServices) {
        throw std::runtime_error("Invalid WMI service pointer");
    }
    m_pWbemServices->AddRef(); // 参照カウントを増やす
}

SystemBrightnessMonitor::~SystemBrightnessMonitor()
{
    StopMonitoring();
    CleanupWMI();
}

void SystemBrightnessMonitor::InitializeWMI()
{
    // COM初期化
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    ThrowIfFailed(hr, "Failed to initialize COM");

    // WbemLocatorの作成
    hr = CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, reinterpret_cast<void**>(&m_pWbemLocator));
    ThrowIfFailed(hr, "Failed to create WbemLocator");

    // WbemServicesへの接続
    hr = m_pWbemLocator->ConnectServer(
        _bstr_t(L"ROOT\\WMI"),
        nullptr, nullptr, nullptr, 0, nullptr, nullptr,
        &m_pWbemServices
    );
    ThrowIfFailed(hr, "Failed to connect to WMI");

    // セキュリティレベルの設定
    hr = CoSetProxyBlanket(
        m_pWbemServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
        nullptr, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE,
        nullptr, EOAC_NONE
    );
    ThrowIfFailed(hr, "Failed to set proxy blanket");

    m_wmiInitialized = true;
}

void SystemBrightnessMonitor::CleanupWMI()
{
    if (m_pWbemLocator) {
        // m_pWbemLocatorがnullでない場合は、このインスタンスがWMIを初期化したことを意味する
        if (m_pWbemServices) {
            m_pWbemServices->Release();
            m_pWbemServices = nullptr;
        }
        m_pWbemLocator->Release();
        m_pWbemLocator = nullptr;

        if (m_wmiInitialized) {
            CoUninitialize();
            m_wmiInitialized = false;
        }
    }
    // m_pWbemLocatorがnullの場合は外部から提供されたWMIサービスなので、
    // m_pWbemServicesは解放しない
}

void SystemBrightnessMonitor::StartMonitoring()
{
    if (m_isMonitoring) return;

    m_isMonitoring = true;
    m_monitoringThread = std::make_unique<std::thread>(&SystemBrightnessMonitor::MonitoringThread, this);
}

void SystemBrightnessMonitor::StopMonitoring()
{
    if (!m_isMonitoring) return;

    m_isMonitoring = false;
    if (m_monitoringThread && m_monitoringThread->joinable()) {
        m_monitoringThread->join();
    }
}

void SystemBrightnessMonitor::MonitoringThread()
{
    if (!m_wmiInitialized) return;

    // イベント監視用のクエリ
    IEnumWbemClassObject* pEnumerator = nullptr;
    HRESULT hr = m_pWbemServices->ExecNotificationQuery(
        _bstr_t("WQL"),
        _bstr_t("SELECT * FROM WmiMonitorBrightnessEvent"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        nullptr,
        &pEnumerator
    );

    if (FAILED(hr)) {
        std::cerr << "Failed to execute notification query" << std::endl;
        return;
    }

    while (m_isMonitoring) {
        IWbemClassObject* pObject = nullptr;
        ULONG returnValue = 0;

        // イベントの待機
        hr = pEnumerator->Next(1000, 1, &pObject, &returnValue);
        if (SUCCEEDED(hr) && returnValue > 0) {
            HandleBrightnessChange(pObject);
            pObject->Release();
        }
    }

    if (pEnumerator) {
        pEnumerator->Release();
    }
}

void SystemBrightnessMonitor::HandleBrightnessChange(IWbemClassObject* pObject)
{
    VARIANT vtProp;
    HRESULT hr = pObject->Get(L"Brightness", 0, &vtProp, nullptr, nullptr);

    if (SUCCEEDED(hr)) {
        if (vtProp.vt == VT_UI1) {
            int brightness = static_cast<int>(vtProp.bVal);
            if (m_brightnessCallback) {
                m_brightnessCallback(brightness);
            }
        }
        VariantClear(&vtProp);
    }
}

void SystemBrightnessMonitor::SetBrightnessChangeCallback(BrightnessChangeCallback callback)
{
    m_brightnessCallback = std::move(callback);
}

int SystemBrightnessMonitor::GetCurrentBrightness()
{
    if (!m_wmiInitialized) return -1;

    IEnumWbemClassObject* pEnumerator = nullptr;
    HRESULT hr = m_pWbemServices->ExecQuery(
        _bstr_t("WQL"),
        _bstr_t("SELECT * FROM WmiMonitorBrightness"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        nullptr,
        &pEnumerator
    );

    if (FAILED(hr)) return -1;

    IWbemClassObject* pObject = nullptr;
    ULONG returnValue = 0;
    int brightness = -1;

    if (SUCCEEDED(pEnumerator->Next(WBEM_INFINITE, 1, &pObject, &returnValue)) && returnValue > 0) {
        VARIANT vtProp;
        hr = pObject->Get(L"CurrentBrightness", 0, &vtProp, nullptr, nullptr);

        if (SUCCEEDED(hr) && vtProp.vt == VT_UI1) {
            brightness = static_cast<int>(vtProp.bVal);
        }

        VariantClear(&vtProp);
        pObject->Release();
    }

    pEnumerator->Release();
    return brightness;
}

void SystemBrightnessMonitor::SetBrightness(int brightness)
{
    if (!m_wmiInitialized) return;

    // 輝度値の範囲チェック
    if (brightness < 0 || brightness > 100) return;

    IWbemClassObject* pClass = nullptr;
    HRESULT hr = m_pWbemServices->GetObject(
        _bstr_t(L"WmiMonitorBrightnessMethod"),
        0,
        nullptr,
        &pClass,
        nullptr
    );

    if (FAILED(hr)) return;

    IWbemClassObject* pInParam = nullptr;
    hr = pClass->GetMethod(L"WmiSetBrightness", 0, &pInParam, nullptr);
    pClass->Release();

    if (FAILED(hr)) return;

    IWbemClassObject* pInstance = nullptr;
    hr = pInParam->SpawnInstance(0, &pInstance);
    pInParam->Release();

    if (FAILED(hr)) return;

    VARIANT var;
    VariantInit(&var);
    var.vt = VT_UI1;
    var.bVal = static_cast<BYTE>(brightness);
    pInstance->Put(L"Brightness", 0, &var, CIM_UINT8);

    var.vt = VT_UI4;
    var.ulVal = 0; // Timeout
    pInstance->Put(L"Timeout", 0, &var, CIM_UINT32);

    m_pWbemServices->ExecMethod(
        _bstr_t(L"WmiMonitorBrightnessMethod"),
        _bstr_t(L"WmiSetBrightness"),
        0,
        nullptr,
        pInstance,
        nullptr,
        nullptr
    );

    pInstance->Release();
}

void SystemBrightnessMonitor::ThrowIfFailed(HRESULT hr, const std::string& message)
{
    if (FAILED(hr)) {
        std::ostringstream oss;
        oss << message << " (HRESULT: 0x" << std::hex << hr << ")";
        throw std::runtime_error(oss.str());
    }
}
