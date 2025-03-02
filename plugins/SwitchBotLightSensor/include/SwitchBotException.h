#ifndef SWITCHBOT_EXCEPTION_H
#define SWITCHBOT_EXCEPTION_H

#include <stdexcept>
#include <string>

class SwitchBotException : public std::runtime_error
{
public:
    explicit SwitchBotException(const std::string& message, int errorCode = 0)
        : std::runtime_error(message)
        , m_errorCode(errorCode)
    {
    }

    int GetErrorCode() const { return m_errorCode; }

private:
    int m_errorCode;
};

class ConfigurationException : public SwitchBotException
{
public:
    explicit ConfigurationException(const std::string& message)
        : SwitchBotException(message, -1)
    {
    }
};

class DeviceNotFoundException : public SwitchBotException
{
public:
    explicit DeviceNotFoundException(const std::string& message)
        : SwitchBotException(message, 404)
    {
    }
};

class AuthenticationException : public SwitchBotException
{
public:
    explicit AuthenticationException(const std::string& message)
        : SwitchBotException(message, 401)
    {
    }
};

#endif // SWITCHBOT_EXCEPTION_H