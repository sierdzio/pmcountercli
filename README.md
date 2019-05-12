# PM Counter CLI

This is a simple reader for Particulate Matter sesnsor PMS7003, written with Qt
and C++.

Plantower PMS7003 specification can be downloaded from [here](https://download.kamami.com/p564008-p564008-PMS7003%20series%20data%20manua_English_V2.5.pdf).

# Hardware

This project assumes that PMS7003 sensor is connected to PC via USB serial port
chip (you can get it [here](ttps://botland.com.pl/pl/czujniki-czystosci-powietrza/13445-adapter-idc-10pin-127mm-microusb-dla-czujnika-pms7003.html)).

# Build

This is a standard qmake project. Just run `qmake && make` or run the project
in Qt Creator.

Only QtCore and QtSerialPort modules are necessary.

# Giants

This project is build on the shoulders of these giants :-)

* serial port parsing was adapted from this [Python code by Mark Benson](https://github.com/MarkJB/python-pms7003).
