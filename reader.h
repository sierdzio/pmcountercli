#ifndef READER_H
#define READER_H

#include <QByteArray>
#include <QSerialPort>
#include <QTimer>
#include <QObject>

class PmData {
public:
    PmData() {}
    PmData(const QByteArray &packet);

    void print() const;
    bool isError() const;

//private:
    bool mIsError = false;
    quint16 frameLength = 0;
    // Standard particulate values in ug/m3
    quint16 stdPm1 = 0;
    quint16 stdPm25 = 0;
    quint16 stdPm10 = 0;
    // Atmospheric particulate values in ug/m3
    quint16 atm1 = 0;
    quint16 atm25 = 0;
    quint16 atm10 = 0;
    // Raw counts per 0.1l
    quint16 raw03 = 0;
    quint16 raw05 = 0;
    quint16 raw1 = 0;
    quint16 raw25 = 0;
    quint16 raw5 = 0;
    quint16 raw10 = 0;
    // Misc data
    quint8 version = 0;
    quint8 errorCode = 0;
    quint16 payloadChecksum = 0;

    // Calculate the payload checksum (not including the payload checksum bytes)
    quint16 inputChecksum = 0x42 + 0x4d;
};

class Reader : public QObject
{
    Q_OBJECT

public:
    explicit Reader(QSerialPort *serialPort, QObject *parent = nullptr);
    PmData pmData() const;

private slots:
    void handleReadyRead();
    void handleTimeout();
    void handleError(const QSerialPort::SerialPortError error);

private:
    QSerialPort *mSerialPort = nullptr;
    QByteArray mReadData;
    QTimer mTimer;
    PmData mPm;
    const int mPacketSize = 32;
};

#endif // READER_H
