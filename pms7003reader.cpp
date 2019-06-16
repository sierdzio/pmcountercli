#include "pms7003reader.h"

#include <QDataStream>
#include <QMetaEnum>
#include <QDebug>

Pms7003Reader::Pms7003Reader(const QString &port, QObject *parent)
    : QObject(parent)
{
    mSerialPort = new QSerialPort(port, this);
    mSerialPort->setBaudRate(QSerialPort::Baud9600);
    mSerialPort->setBaudRate(QSerialPort::OneStop);

    if (mSerialPort->open(QSerialPort::ReadWrite) == false) {
        qWarning() << "Could not open the device port for reading"
                   << mSerialPort->portName() << mSerialPort->baudRate()
                   << mSerialPort->stopBits() << mSerialPort->errorString();
        delete mSerialPort;
        return;
    }

    connect(mSerialPort, &QSerialPort::readyRead,
            this, &Pms7003Reader::handleReadyRead);
    connect(mSerialPort, &QSerialPort::errorOccurred,
            this, &Pms7003Reader::handleError);
}

PmPacket Pms7003Reader::pmData() const
{
    return mPm;
}

bool Pms7003Reader::isPortOpen() const
{
    if ((mSerialPort.isNull() == false) and mSerialPort->isOpen())
        return true;
    return false;
}

QSerialPort::SerialPortError Pms7003Reader::portError() const
{
    if (mSerialPort.isNull()) {
        return QSerialPort::NotOpenError;
    }

    return mSerialPort->error();
}

QString Pms7003Reader::portErrorString() const
{
    if (mSerialPort.isNull()) {
        return tr("Serial port is not open");
    }

    return mSerialPort->errorString();
}

void Pms7003Reader::setAverageResults(const bool average)
{
    mAverageResults = average;
}

bool Pms7003Reader::isAveragingResults() const
{
    return mAverageResults;
}

bool Pms7003Reader::executeCommand(const CommandType type) const
{
    if (mSerialPort.isNull()
        or mSerialPort->isOpen() == false
        or mSerialPort->isWritable() == false
        or mCommands.contains(type) == false) {
        return false;
    }

    const PmCommand &command = mCommands.value(type);
    const QByteArray commandData = command.command();
    qDebug() << "Executing command:" << type << commandData.toHex();
    const qint64 written = mSerialPort->write(commandData);

    if (written == qint64(commandData.length())) {
        return true;
    }

    return false;
}

QString Pms7003Reader::status() const
{
    QString result;
    result = QString(QMetaEnum::fromType<Status>().valueToKey(int(mStatus)));
    return result;
}

void Pms7003Reader::handleReadyRead()
{
    if (mSerialPort.isNull()) {
        return;
    }

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

void Pms7003Reader::handleError(const QSerialPort::SerialPortError error)
{
    if (mSerialPort.isNull()) {
        return;
    }

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
        const qint64 dataBytesLength = stream.device()->pos(); // Should be 29
        //qDebug() << "Packet position" << dataBytesLength;
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

Pms7003Reader::PmCommand::PmCommand(const Pms7003Reader::CommandType _type,
                                    const quint8 _cmd,
                                    const quint16 _data,
                                    const bool _hasReply)
    : type(_type), cmd(_cmd), data(_data), hasReply(_hasReply)
{
}

QByteArray Pms7003Reader::PmCommand::command() const
{
    QByteArray result;
    QDataStream stream(&result, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    const quint16 integrityCheck = 0x42 + 0x4d + cmd + data;
    stream << 0x42 << 0x4d << cmd << data << integrityCheck;
    return result;
}
