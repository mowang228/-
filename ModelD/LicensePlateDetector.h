#ifndef LICENSEPLATEDETECTOR_H
#define LICENSEPLATEDETECTOR_H

#include <QObject>
#include <QString>
#include <QRect>
#include <QList>

struct DetectedPlate
{
    QRect rect;
    float confidence;
    QString label;
};

class LicensePlateDetector : public QObject
{
    Q_OBJECT

public:
    explicit LicensePlateDetector(QObject *parent = nullptr);
    ~LicensePlateDetector() override;

    bool loadModel(const QString &modelPath);

    QList<DetectedPlate> detect(const QString &imagePath);

    QString drawDetection(const QString &imagePath, const QList<DetectedPlate> &plates, const QString &outputPath);

private:
    bool m_modelLoaded = false;
    QString m_modelPath;
};

#endif // LICENSEPLATEDETECTOR_H