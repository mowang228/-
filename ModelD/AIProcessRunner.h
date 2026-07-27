#ifndef AIPROCESSRUNNER_H
#define AIPROCESSRUNNER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QJsonObject>

class AIProcessRunner : public QObject
{
    Q_OBJECT

public:
    explicit AIProcessRunner(QObject *parent = nullptr);

    QJsonObject runPythonScript(const QString &scriptPath, const QStringList &args);

private:
    bool checkFileExists(const QString &filePath, const QString &description);
};

#endif // AIPROCESSRUNNER_H