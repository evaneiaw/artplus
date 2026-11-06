#pragma once

#include <QString>

namespace artplus {

enum class LogLevel { Debug, Info, Warning, Error };

class Logger
{
public:
    static Logger& instance();

    void setLogFile(const QString& path);
    void setMinimumLevel(LogLevel level);

    void write(LogLevel level, const QString& message);

    QString logFilePath() const;

private:
    Logger();
    ~Logger();

    struct Impl;
    Impl* d;
};

void logDebug(const QString& message);
void logInfo(const QString& message);
void logWarning(const QString& message);
void logError(const QString& message);

} // namespace artplus
