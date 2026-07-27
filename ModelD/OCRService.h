#ifndef OCRSERVICE_H
#define OCRSERVICE_H

#include <QObject>
#include <QString>
#include <QVector>

class OCRService : public QObject
{
    Q_OBJECT

public:
    explicit OCRService(QObject *parent = nullptr);
    ~OCRService() override = default;

    QString recognizePlate(const QString &imagePath);

    QString recognizePlate(const QVector<QString> &plateImages);
};

#endif // OCRSERVICE_H