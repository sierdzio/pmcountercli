#include "reader.h"

#include <QCoreApplication>
#include <QDebug>

Reader::Reader(QSerialPort *serialPort, QObject *parent)
    : QObject(parent), mSerialPort(serialPort)
{
    connect(mSerialPort, &QSerialPort::readyRead, this, &Reader::handleReadyRead);
    connect(mSerialPort, &QSerialPort::errorOccurred, this, &Reader::handleError);
    connect(&mTimer, &QTimer::timeout, this, &Reader::handleTimeout);

    mTimer.start(5000);
}

void Reader::handleReadyRead()
{
    mReadData.append(mSerialPort->readAll());

    if (!mTimer.isActive())
        mTimer.start(5000);

    if (mReadData.isEmpty() == false) {
        //qDebug() << tr("Data successfully received from port %1")
        //                .arg(mSerialPort->portName())
        //         << mReadData;

        if (mReadData.size() >= mPacketSize) {
            const QByteArray packet(mReadData.left(mPacketSize));
            mReadData = mReadData.mid(mPacketSize);
            if (packet.at(0) == 0x42 and packet.at(1) == 0x4d) {
                const int frameLength = packet.at(1) + (packet.at(0) << 8);
            // Standard particulate values in ug/m3
                const int stdPm1 = packet.at(3) + (packet.at(2) << 8);
                const int stdPm25 = packet.at(5) + (packet.at(4) << 8);
                const int stdPm10 = packet.at(7) + (packet.at(6) << 8);
            // Atmospheric particulate values in ug/m3
                const int atm1 = packet.at(9) + (packet.at(8) << 8);
                const int atm25 = packet.at(11) + (packet.at(10) << 8);
                const int atm10 = packet.at(13) + (packet.at(12) << 8);
            // Raw counts per 0.1l
                //const int raw03 = packet.at(15) + (packet.at(14) << 8);
                //const int raw05 = packet.at(17) + (packet.at(16) << 8);
                //const int raw1 = packet.at(19) + (packet.at(18) << 8);
                //const int raw25 = packet.at(21) + (packet.at(20) << 8);
                //const int raw5 = packet.at(23) + (packet.at(22) << 8);
                //const int raw10 = packet.at(25) + (packet.at(24) << 8);
            // Misc data
                const int version = packet.at(26);
                const int errorCode = packet.at(27);
                const int payloadChecksum = packet.at(29) + (packet.at(28) << 8);

            // Calculate the payload checksum (not including the payload checksum bytes)
                int inputChecksum = 0x42 + 0x4d;
                for (int i = 0; i < 27; ++i) {
                    inputChecksum = inputChecksum + packet.at(i);
                }

                if (inputChecksum != payloadChecksum) {
                    qDebug() << "Checksums do not match!" << inputChecksum
                             << "should be:" << payloadChecksum;
                }

                qDebug() << "Length:" << frameLength
                         << "\nStandard PM in ug/m3:\n"
                         << "1.0 | 2.5 | 10.0\n"
                         << stdPm1 << "   " << stdPm25 << "   " << stdPm10
                         << "\nAtmospheric PM in ug/m3:\n"
                         << atm1 << "   " << atm25 << "   " << atm10
                         << "V:" << version << "Error:" << errorCode
                    ;
            } else {
                qDebug() << "Incorrect packet!" << packet.toHex();
            }
        }
    }
}

void Reader::handleTimeout()
{
    if (mReadData.isEmpty()) {
        qDebug() << tr("No data was currently available for reading from port %1")
                                .arg(mSerialPort->portName());
        QCoreApplication::quit();
    }
}

void Reader::handleError(const QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ReadError) {
        qDebug() << tr("An I/O error occurred while reading the data from port "
                       "%1, error: %2").arg(mSerialPort->portName(),
                             mSerialPort->errorString());
        QCoreApplication::exit(1);
    }
}
