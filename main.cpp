#include <QCoreApplication>
#include <QDebug>

#include "reader.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    /*!
     * https://download.kamami.com/p564008-p564008-PMS7003%20series%20data%20manua_English_V2.5.pdf
     *
     * https://askubuntu.com/questions/112568/how-do-i-allow-a-non-default-user-to-use-serial-device-ttyusb0
     *
     * sudo nano /etc/udev/rules.d/50-myusb.rules
     *
     * KERNEL=="ttyUSB[0-9]*",MODE="0666"
     * KERNEL=="ttyACM[0-9]*",MODE="0666"
     *
     * dmesg | grep tty
     */

    Pms7003Reader reader(QStringLiteral("/dev/ttyUSB0"));
    return a.exec();
}
