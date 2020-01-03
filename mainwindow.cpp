#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSerialPortInfo>
#include <QSerialPort>
#include <QFileDialog>

QSerialPort serial;
QFile file;
QByteArray recived;
QString mode = "";
int sizeOfFile = 0;

int scti(char ch) {
    return (ch < 0) ? (ch + 256) : ch;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QList<QSerialPortInfo> com_ports = QSerialPortInfo::availablePorts();
    QSerialPortInfo port;
    foreach(port, com_ports)
    {
        ui->comboBox->addItem(port.portName());
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    QString folder = QFileDialog::getExistingDirectory();
    if (folder.length() == 0) {
        ui->statusbar->showMessage("C: empty downloading path");
        return;
    }
    /* ВЫБОР ИМЕНИ СКАЧИВАЕМОГО ФАЙЛА */
    QString fileName = (ui->listWidget->currentItem()->text()).split(' ')[0];
    size_t sizeOfFileName = fileName.length();
    QString fullFileName = folder + fileName;
    QByteArray sizeOfFileNameBytes;

    /* ПОЛУЧЕНИЕ РАЗМЕРА ИМЕНИ СКАЧИВАЕМОГО ФАЙЛА */
    sizeOfFileNameBytes.append((sizeOfFileName >> 24) & 0xFF);
    sizeOfFileNameBytes.append((sizeOfFileName >> 16) & 0xFF);
    sizeOfFileNameBytes.append((sizeOfFileName >> 8) & 0xFF);
    sizeOfFileNameBytes.append(sizeOfFileName & 0xFF);
    file.setFileName(fullFileName);
    ui->statusbar->showMessage("downloading...");

    /* ДЕЙСТВИЯ С ПОСЛЕДОВАТЕЛЬНЫМ ПОРТОМ */
    mode = "DOWNLOAD";
    serial.write("D");
    serial.write(sizeOfFileNameBytes);
    serial.write(fileName.toUtf8().constData());
}

void MainWindow::on_pushButton_2_clicked()
{
    /* ВЫБОР ИМЕНИ ФАЙЛА ВЫГРУЖАЕМОГО ФАЙЛА */
    QString fullFileName = QFileDialog::getOpenFileName();
    if (fullFileName.length() == 0) {
        ui->statusbar->showMessage("E: empty uploading path");
        return;
    }
    QStringList qst = fullFileName.split("/");
    QString fileName = "/" + qst[qst.size() - 1];
    int sizeOfFileName = fileName.length();
    QByteArray sizeOfFileNameBytes;
    sizeOfFileNameBytes.append((sizeOfFileName >> 24) & 0xFF);
    sizeOfFileNameBytes.append((sizeOfFileName >> 16) & 0xFF);
    sizeOfFileNameBytes.append((sizeOfFileName >> 8) & 0xFF);
    sizeOfFileNameBytes.append(sizeOfFileName & 0xFF);

    /* ОТКРЫТИЕ ВЫГРУЖАЕМОГО ФАЙЛА И ПОЛУЧЕНИЕ ЕГО РАЗМЕРА */
    QFile f(fullFileName);
    f.open(QIODevice::ReadOnly);
    if (!f.isOpen()) {
        ui->statusbar->showMessage("C: can't open uploading file");
        return;
    }
    QByteArray dataBytes = f.readAll();
    f.close();
    QByteArray sizeOfFilBytes;
    sizeOfFilBytes.append((dataBytes.length() >> 24) & 0xFF);
    sizeOfFilBytes.append((dataBytes.length() >> 16) & 0xFF);
    sizeOfFilBytes.append((dataBytes.length() >> 8) & 0xFF);
    sizeOfFilBytes.append(dataBytes.length() & 0xFF);
    ui->statusbar->showMessage("uploading...");

    /* ДЕЙСТВИЯ С ПОСЛЕДОВАТЕЛЬНЫМ ПОРТОМ */
    mode = "UPLOAD";
    serial.write("U");
    serial.write(sizeOfFileNameBytes);
    serial.write(sizeOfFilBytes);
    serial.write(fileName.toUtf8().constData());
    serial.write(dataBytes);
}

void MainWindow::on_pushButton_3_clicked()
{
    // ИНИЦИАЛИЗАЦИЯ КОМ-ПОРТА
    serial.setPortName("cu.SLAB_USBtoUART");
    serial.setBaudRate(QSerialPort::Baud115200);
    serial.setDataBits(QSerialPort::Data8);
    serial.setParity(QSerialPort::NoParity);
    serial.setStopBits(QSerialPort::OneStop);
    serial.setFlowControl(QSerialPort::NoFlowControl);
    serial.open(QIODevice::ReadWrite);

    if (serial.isOpen()) {
        ui->statusbar->showMessage("port opened");
    } else {
        ui->statusbar->showMessage("E: can't open port");
    }

    mode = "MEMORY";

    serial.write("M");

    QObject::connect(&serial, &QSerialPort::readyRead, [&] {
        recived.append(serial.readAll());
        if (mode == "DOWNLOAD") {
            if (sizeOfFile == 0) {
                if (recived.length() >= 4) {
                    sizeOfFile = (scti(recived[0]) << 24) | (scti(recived[1]) << 16) | (scti(recived[2]) << 8) | scti(recived[3]);
                    recived.remove(0, 4);
                    ui->statusbar->showMessage(QString::number(sizeOfFile));
                }
            }
            if (recived.length() == sizeOfFile) {
                file.open(QFile::WriteOnly);
                file.write(recived);
                file.close();
                recived.clear();
                mode = "";
                sizeOfFile = 0;
                ui->statusbar->showMessage("downloading end.");
            }
        }
        else if (mode == "UPLOAD") {
            if (recived[0] == 'U') {
                ui->statusbar->showMessage("uploading end.");
                recived.clear();
                mode = "";
            }
        }

        else if (mode == "MEMORY") {
            if (recived.length() >= 8) {
                size_t totalMemory = (scti(recived[0]) << 24) | (scti(recived[1]) << 16) | (scti(recived[2]) << 8) | scti(recived[3]);
                size_t usedMemory = (scti(recived[4]) << 24) | (scti(recived[5]) << 16) | (scti(recived[6]) << 8) | scti(recived[7]);
                ui->memory->setRange(0, totalMemory);
                ui->memory->setValue(usedMemory);
                ui->totalMemory->setText(QString::number((int32_t)totalMemory));
                ui->usedMemory->setText(QString::number((int32_t)usedMemory));
                recived.clear();
                mode = "LIST";
                serial.write("L");
            }
        }

        else if (mode == "LIST") {
            while (recived.length()) {
                uint8_t sizeOfName = (scti(recived[0]) << 24) | (scti(recived[1]) << 16) | (scti(recived[2]) << 8) | scti(recived[3]);
                size_t sizeOfFile = sizeOfFile = (scti(recived[4]) << 24) | (scti(recived[5]) << 16) | (scti(recived[6]) << 8) | scti(recived[7]);
                recived.remove(0, 8);
                QString Name = "";
                for (size_t i = 0; i < sizeOfName; i++) {
                    Name.append(recived.at(i));
                }
                recived.remove(0, sizeOfName);
                QString extension;
                if ((Name.indexOf('.') > 0) <= 0) {
                    extension = "unknown";
                } else {
                    extension = (Name.split(".")[1]).toLower();
                }
                QString icon;
                if (extension == "txt") {
                    icon = ":/new/icons/text.png";
                } else if ((extension == "bmp") || (extension == "jpg")) {
                    icon = ":/new/icons/picture.png";
                } else if (extension == "wav") {
                    icon = ":/new/icons/music.png";
                } else {
                    icon = ":/new/icons/unknown.png";
                }
                QListWidgetItem * item = new QListWidgetItem(QIcon(icon), Name + " (" + QString::number(sizeOfFile) + " B)");
                ui->listWidget->addItem(item);
            }
        }
    });
}
