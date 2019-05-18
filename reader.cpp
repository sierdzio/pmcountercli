#include "reader.h"

#include <QDataStream>
#include <QDebug>

Reader::Reader(QSerialPort *serialPort,
               const int timeout,
               QObject *parent)
    : QObject(parent), mSerialPort(serialPort), mTimeout(timeout)
{
    connect(mSerialPort, &QSerialPort::readyRead, this, &Reader::handleReadyRead);
    connect(mSerialPort, &QSerialPort::errorOccurred, this, &Reader::handleError);
    connect(&mTimer, &QTimer::timeout, this, &Reader::handleTimeout);
    mTimer.setSingleShot(true);
    mTimer.start(mTimeout);
}

PmPacket Reader::pmData() const
{
    return mPm;
}

int Reader::timeout() const
{
    return mTimeout;
}

void Reader::restart()
{
    mTimer.start(mTimeout);
}

void Reader::setAverageResults(const bool average)
{
    mAverageResults = average;
}

bool Reader::isAveragingResults() const
{
    return mAverageResults;
}

void Reader::handleReadyRead()
{
    mReadData.append(mSerialPort->readAll());

    //if (!mTimer.isActive())
    //    mTimer.start(mTimeout);

    if (mReadData.isEmpty() == false) {
        //qDebug() << tr("Data successfully received from port %1")
        //                .arg(mSerialPort->portName())
        //         << mReadData;

        while (mReadData.size() >= mPacketSize) {
            if (mReadData.at(0) == 0x42 and mReadData.at(1) == 0x4d) {
                const QByteArray packet(mReadData.left(mPacketSize));
                mReadData = mReadData.mid(mPacketSize);
                if (mAverageResults) {
                    mPm.averageWith(PmPacket(packet));
                } else {
                    mPm = PmPacket(packet);
                }
                mPm.print();
            } else {
                // Remove first character
                mReadData = mReadData.mid(1, -1);
            }
        }
    }
}

void Reader::handleTimeout()
{
    if (mReadData.isEmpty()) {
        qDebug() << "Timeout! Closing port";
        mSerialPort->close();
    }
}

void Reader::handleError(const QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ReadError) {
        qDebug() << tr("An I/O error occurred while reading the data from port "
                       "%1, error: %2").arg(mSerialPort->portName(),
                             mSerialPort->errorString());
        mSerialPort->close();
    }
}

PmPacket::PmPacket(const QByteArray &packet)
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
        const int dataBytesLength = stream.device()->pos(); // Should be 29
        qDebug() << "Packet position" << dataBytesLength;
        //stream.skipRawData(2); // skip 2 bytes
        stream >> payloadChecksum;

        //qDebug() << "Pos:" << frameLength << ":" << stream.device()->pos();

        // Calculate the payload checksum (not including the payload checksum bytes)
        quint16 inputChecksum = 0;
        for (int i = 0; i < dataBytesLength; ++i) {
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

void PmPacket::averageWith(const PmPacket &other)
{
    if (mIsError && other.isError()) {
        qDebug() << "Both data packets contain errors";
        return;
    }

    const bool isOtherError = other.isError();
    avg(stdPm1, other.stdPm1, isOtherError);
    avg(stdPm25, other.stdPm25, isOtherError);
    avg(stdPm10, other.stdPm10, isOtherError);

    avg(atm1, other.atm1, isOtherError);
    avg(atm25, other.atm25, isOtherError);
    avg(atm10, other.atm10, isOtherError);

    avg(raw03, other.raw03, isOtherError);
    avg(raw05, other.raw05, isOtherError);
    avg(raw1, other.raw1, isOtherError);
    avg(raw25, other.raw25, isOtherError);
    avg(raw5, other.raw5, isOtherError);
    avg(raw10, other.raw10, isOtherError);
}

void PmPacket::print() const
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

bool PmPacket::isError() const
{
    return mIsError;
}

template<class Type>
void PmPacket::avg(Type &orig, const Type &other, const bool isOtherError)
{
    if (mIsError && isOtherError) {
        qDebug() << "ERROR! Both values which you try to average are in error state";
        return;
    }

    if (mIsError == false && isOtherError == true) {
        return;
    }

    if (mIsError == true && isOtherError == false) {
        orig = other;
        return;
    }

    orig += other;
    orig /= Type(2);
}
