#include <QCoreApplication>
#include <QSerialPort>
#include <QDebug>

#include "reader.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QSerialPort port;
    port.setBaudRate(QSerialPort::Baud9600);
    //port.setBreakEnabled();
    //port.setDataBits(QSerialPort::Data8);
    //port.setDataTerminalReady();
    //port.setFlowControl();
    //port.setParity();
    //port.setPort(const QSerialPortInfo &serialPortInfo);
    /*!
     * https://download.kamami.com/p564008-p564008-PMS7003%20series%20data%20manua_English_V2.5.pdf
     *
     * https://askubuntu.com/questions/112568/how-do-i-allow-a-non-default-user-to-use-serial-device-ttyusb0
     *
     * dmesg | grep tty
     */
    port.setPortName(QStringLiteral("/dev/ttyUSB0"));
    //port.setReadBufferSize();
    //port.setRequestToSend();
    port.setStopBits(QSerialPort::OneStop);

    if (!port.open(QSerialPort::ReadOnly)) {
        qFatal("Serial port could not be opened");
    }

    Reader reader(&port);
    return a.exec();
}
