#include "artplus/core/Logger.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QStandardPaths>
#include <QTextStream>
#include <QDebug>

namespace artplus {

namespace {

const char* levelName(LogLevel level)
{
    switch (level) {
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info: return "INFO";
    case LogLevel::Warning: return "WARN";
    case LogLevel::Error: return "ERROR";
    }
    return "INFO";
}

} // namespace

struct Logger::Impl {
    QMutex mutex;
    QString path;
    QFile file;
    QTextStream stream;
    LogLevel minimum = LogLevel::Info;
    bool fileReady = false;
};

Logger::Logger()
    : d(new Impl)
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(dir + QStringLiteral("/logs"));
    setLogFile(dir + QStringLiteral("/logs/artplus.log"));
}

Logger::~Logger()
{
    if (d->file.isOpen())
        d->file.close();
    delete d;
}

Logger& Logger::instance()
{
    static Logger logger;
    return logger;
}

void Logger::setLogFile(const QString& path)
{
    QMutexLocker lock(&d->mutex);
    if (d->file.isOpen())
        d->file.close();
    d->path = path;
    d->file.setFileName(path);
    if (d->file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        d->stream.setDevice(&d->file);
        d->fileReady = true;
    } else {
        d->fileReady = false;
    }
}

void Logger::setMinimumLevel(LogLevel level)
{
    QMutexLocker lock(&d->mutex);
    d->minimum = level;
}

void Logger::write(LogLevel level, const QString& message)
{
    QMutexLocker lock(&d->mutex);
    if (static_cast<int>(level) < static_cast<int>(d->minimum))
        return;

    const QString line = QStringLiteral("%1 [%2] %3")
                             .arg(QDateTime::currentDateTime().toString(Qt::ISODateWithMs),
                                  QString::fromLatin1(levelName(level)),
                                  message);
    if (level == LogLevel::Error)
        qCritical().noquote() << line;
    else if (level == LogLevel::Warning)
        qWarning().noquote() << line;
    else
        qDebug().noquote() << line;

    if (d->fileReady) {
        d->stream << line << '\n';
        d->stream.flush();
    }
}

QString Logger::logFilePath() const
{
    QMutexLocker lock(&d->mutex);
    return d->path;
}

void logDebug(const QString& message) { Logger::instance().write(LogLevel::Debug, message); }
void logInfo(const QString& message) { Logger::instance().write(LogLevel::Info, message); }
void logWarning(const QString& message) { Logger::instance().write(LogLevel::Warning, message); }
void logError(const QString& message) { Logger::instance().write(LogLevel::Error, message); }

} // namespace artplus
