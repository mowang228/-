#ifndef LLMSERVICE_H
#define LLMSERVICE_H

#include <QObject>
#include <QString>
#include <QStringList>

struct AnalysisResult
{
    QString plateNumber;
    QString vehicleBrand;
    QString conclusion;
    QStringList detectedPlates;
    QString resultImagePath;
};

class LLMService : public QObject
{
    Q_OBJECT

public:
    explicit LLMService(QObject *parent = nullptr);
    ~LLMService() override;

    void analyze(const QString &imagePath);

signals:
    void analysisCompleted(const AnalysisResult &result);
    void analysisFailed(const QString &errorMessage);

private:
    void onAnalyzeInternal(const QString &imagePath);

    class Impl;
    Impl *m_impl = nullptr;
};

#endif // LLMSERVICE_H