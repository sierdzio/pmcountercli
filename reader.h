#ifndef READER_H
#define READER_H

#include <QByteArray>
#include <QSerialPort>
#include <QTimer>
#include <QObject>

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
