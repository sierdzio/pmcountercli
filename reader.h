#ifndef READER_H
#define READER_H

#include <QByteArray>
#include <QSerialPort>
#include <QTimer>
#include <QHash>
#include <QObject>

class PmPacket {
public:
    PmPacket() {}
    PmPacket(const QByteArray &packet);

    void averageWith(const PmPacket &other);

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

private:
    template<class Type>
    void avg(Type &orig, const Type &other, const bool isOtherError);
};

class Reader : public QObject
{
    Q_OBJECT

public:
    enum Status {
        Unknown,
        Sleeping,
        WakingUp,
        Passive,
        Active
    };
    Q_ENUM(Status)

    enum CommandType {
        RequestData, //! Command works in passive mode
        MakePassive,
        MakeActive,
        Sleep,
        WakeUp
    };
    Q_ENUM(CommandType)

    struct PmCommand {
        PmCommand() {}
        PmCommand(const CommandType _type, const quint8 _cmd, const quint16 _data,
                  const bool _hasReply);

        QByteArray command() const;

        CommandType type;
        quint8 cmd = 0;
        quint16 data = 0;
        bool hasReply = false;
    };

    explicit Reader(QSerialPort *serialPort,
                    const int timeout = 5000,
                    QObject *parent = nullptr);
    PmPacket pmData() const;
    int timeout() const;
    void restart();

    void setAverageResults(const bool average);
    bool isAveragingResults() const;

    // Commands
    bool executeCommand(const CommandType type) const;

    QString status() const;

private slots:
    void handleReadyRead();
    void handleTimeout();
    void handleError(const QSerialPort::SerialPortError error);

private:
    QSerialPort *mSerialPort = nullptr;
    QByteArray mReadData;
    QTimer mTimer;
    PmPacket mPm;
    bool mAverageResults = false;
    Status mStatus = Unknown;
    const int mPacketSize = 32;
    const int mTimeout = 5000;
    const QHash<CommandType, PmCommand> mCommands = {
        { RequestData, { RequestData, 0xe2, 0, true } },
        { MakePassive, { MakePassive, 0xe1, 0, false } },
        { MakeActive,  { MakeActive,  0xe1, 0x0001, false } },
        { Sleep,       { Sleep,       0xe4, 0, false } },
        { WakeUp,      { WakeUp,      0xe4, 0x0001, false } },
    };
};

#endif // READER_H
