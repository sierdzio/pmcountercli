#include "reader.h"

#include <QCoreApplication>
//#include <QtEndian>
#include <QDataStream>
#include <QDebug>

Reader::Reader(QSerialPort *serialPort, QObject *parent)
    : QObject(parent), mSerialPort(serialPort)
{
    connect(mSerialPort, &QSerialPort::readyRead, this, &Reader::handleReadyRead);
    connect(mSerialPort, &QSerialPort::errorOccurred, this, &Reader::handleError);
    connect(&mTimer, &QTimer::timeout, this, &Reader::handleTimeout);

    mTimer.start(5000);
}

PmData Reader::pmData() const
{
    return mPm;
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
            mPm = PmData(packet);
            mPm.print();
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

PmData::PmData(const QByteArray &packet)
{
    if (packet.at(0) == 0x42 and packet.at(1) == 0x4d) {
        QDataStream stream(packet);
        stream.setByteOrder(QDataStream::BigEndian);
        stream.skipRawData(2); // Skip start characters (0x42 and 0x4d)
        stream >> frameLength;
        // Standard particulate values in ug/m3
        stream >> stdPm1;
        stream >> stdPm25;
        stream >> stdPm10;

//        const quint16 pm25Endian = qFromBigEndian<quint16>(packet.mid(4,2).data());
//        QDataStream stream(packet);
//        stream.setByteOrder(QDataStream::BigEndian);
//        quint16 pm25Stream = 0;
//        stream >> pm25Stream; // length
//        stream >> pm25Stream; // PM1
//        stream >> pm25Stream; // PM2.5
//        qDebug() << "Endianess test:" << stdPm25 << pm25Endian << pm25Stream;

        // Atmospheric particulate values in ug/m3
        stream >> atm1;
        stream >> atm25;
        stream >> atm10;
        // Raw counts per 0.1l
        stream >> raw03;
        stream >> raw05;
        stream >> raw1;
        stream >> raw25;
        stream >> raw5;
        stream >> raw10;
        // Misc data
        stream >> version; // = quint8(packet.at(28));
        stream >> errorCode; // = quint8(packet.at(29));
        //stream.skipRawData(2); // skip 2 bytes
        stream >> payloadChecksum;

        qDebug() << "Pos:" << frameLength << ":" << stream.device()->pos();

        // Calculate the payload checksum (not including the payload checksum bytes)
        quint16 inputChecksum = 0;
        for (int i = 0; i < 29; ++i) {
            inputChecksum = inputChecksum + quint8(packet.at(i));
        }

        if (inputChecksum != payloadChecksum) {
            qDebug() << "Checksums do not match!" << inputChecksum
                     << "should be:" << payloadChecksum;
        }
    } else {
        qDebug() << "Incorrect packet!" << packet.toHex();
        mIsError = true;
    }
}

void PmData::print() const
{
    //qDebug() << "Length:" << frameLength
    //         << "\nStandard PM in ug/m3:\n"
    //         << "1.0 | 2.5 | 10.0\n"
    //         << stdPm1 << "   " << stdPm25 << "   " << stdPm10
    //         << "\nAtmospheric PM in ug/m3:\n"
    //         << atm1 << "   " << atm25 << "   " << atm10
    //         << "V:" << version << "Error:" << errorCode;

    qDebug() << stdPm1 << "|" << stdPm25 << "|" << stdPm10;
}

bool PmData::isError() const
{
    return mIsError;
}
