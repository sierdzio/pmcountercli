#ifndef READER_H
#define READER_H

#include <QByteArray>
#include <QSerialPort>
#include <QTimer>
#include <QObject>

class PmData {
public:
    PmData(const QByteArray &packet);

    void print() const;
    bool isError() const;

private:
    bool mIsError = false;
    quint16 frameLength;
    // Standard particulate values in ug/m3
    quint16 stdPm1;
    quint16 stdPm25;
    quint16 stdPm10;
    // Atmospheric particulate values in ug/m3
    quint16 atm1;
    quint16 atm25;
    quint16 atm10;
    // Raw counts per 0.1l
    quint16 raw03;
    quint16 raw05;
    quint16 raw1;
    quint16 raw25;
    quint16 raw5;
    quint16 raw10;
    // Misc data
    quint8 version;
    quint8 errorCode;
    quint16 payloadChecksum;

    // Calculate the payload checksum (not including the payload checksum bytes)
    quint16 inputChecksum = 0x42 + 0x4d;
};

class Reader : public QObject
{
    Q_OBJECT

public:
    explicit Reader(QSerialPort *serialPort, QObject *parent = nullptr);

private slots:
    void handleReadyRead();
    void handleTimeout();
    void handleError(const QSerialPort::SerialPortError error);

private:
    QSerialPort *mSerialPort = nullptr;
    QByteArray mReadData;
    QTimer mTimer;
    const int mPacketSize = 32;
};

#endif // READER_H
