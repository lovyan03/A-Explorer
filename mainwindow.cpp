#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSerialPortInfo>
#include <QSerialPort>
#include <QFileDialog>
#include <QMessageBox>

bool connected = false;
QSerialPort serial;
QFile file;
QByteArray recived;
QString mode = "";
int sizeOfFile = 0;
QByteArray dataBytes;
int writeBytesLength;

int scti(char ch) {
    return (ch < 0) ? (ch + 256) : ch;
}

void rightWrite() {
    writeBytesLength = dataBytes.length();
    if (writeBytesLength) {
        serial.write("U");
    } else {
        serial.write("u");
        return;
    }
    int sizeOfPiece = 32;
    QByteArray sizeOfPieceBytes;
    int x = (writeBytesLength < sizeOfPiece) ? writeBytesLength : sizeOfPiece;
    sizeOfPieceBytes.append(x);
    serial.write(sizeOfPieceBytes);
    serial.write(dataBytes, x);
    dataBytes.remove(0, x);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    on_pushButton_6_clicked();
    setWindowFlags(Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    if (!connected || !ui->listWidget->currentItem()) return;
    QString folder = QFileDialog::getExistingDirectory();
    if (folder.length() == 0) {
        ui->statusbar->showMessage("C: empty downloading path");
        return;
    }
    /* ВЫБОР ИМЕНИ СКАЧИВАЕМОГО ФАЙЛА */
    QString fileName = (ui->listWidget->currentItem()->text()).split(':')[0];
    size_t sizeOfFileName = fileName.length();
    QString fullFileName = folder + fileName;
    QByteArray sizeOfFileNameBytes;

    /* ПОЛУЧЕНИЕ РАЗМЕРА ИМЕНИ СКАЧИВАЕМОГО ФАЙЛА */
    sizeOfFileNameBytes.append((sizeOfFileName >> 24) & 0xFF);
    sizeOfFileNameBytes.append((sizeOfFileName >> 16) & 0xFF);
    sizeOfFileNameBytes.append((sizeOfFileName >> 8) & 0xFF);
    sizeOfFileNameBytes.append(sizeOfFileName & 0xFF);
    file.setFileName(fullFileName);
    ui->statusbar->showMessage("downloading, please wait...");

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
    dataBytes = f.readAll();
    sizeOfFile = dataBytes.length();
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
}

void MainWindow::on_pushButton_3_clicked()
{
    // UI ПО-УМОЛЧАНИЮ
    ui->statusbar->showMessage("");
    ui->listWidget->clear();
    ui->memory->setValue(0);
    ui->totalMemory->setText("0");
    ui->availableMemory->setText("0");
    if (!connected) {
        ui->pushButton_3->setText("disconnect");

        // ИНИЦИАЛИЗАЦИЯ КОМ-ПОРТА
        serial.setPortName(ui->comboBox->currentText());
        serial.setBaudRate(QSerialPort::Baud115200);
        serial.setDataBits(QSerialPort::Data8);
        serial.setParity(QSerialPort::NoParity);
        serial.setStopBits(QSerialPort::OneStop);
        serial.setFlowControl(QSerialPort::NoFlowControl);

        if (!serial.open(QIODevice::ReadWrite)) {
            ui->statusbar->showMessage("E: can't use the port");
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
                    }
                }
                if (recived.length() == sizeOfFile) {
                    file.open(QFile::WriteOnly);
                    file.write(recived);
                    file.close();
                    recived.clear();
                    mode = "";
                    sizeOfFile = 0;
                    ui->statusbar->showMessage("downloading complete");
                } else {
                    int p = (recived.length() * 100) / sizeOfFile;
                    ui->statusbar->showMessage("downloaded " + QString::number(p) + "%");
                }

            }
            else if (mode == "UPLOAD") {
                if (recived[0] == 'A') {
                    rightWrite();
                    recived.clear();
                    int p = 100 - (writeBytesLength * 100 / sizeOfFile);
                    ui->statusbar->showMessage("uploaded " + QString::number(p) + "%");
                }

                if (recived[0] == 'u') {
                    ui->statusbar->showMessage("uploading complete");
                    recived.clear();
                    sizeOfFile = 0;
                    mode = "MEMORY";
                    serial.write("M");
                }
            }

            else if (mode == "MEMORY") {
                if (recived.length() >= 8) {
                    size_t totalMemory = (scti(recived[0]) << 24) | (scti(recived[1]) << 16) | (scti(recived[2]) << 8) | scti(recived[3]);
                    size_t usedMemory = (scti(recived[4]) << 24) | (scti(recived[5]) << 16) | (scti(recived[6]) << 8) | scti(recived[7]);
                    ui->memory->setRange(0, totalMemory);
                    ui->memory->setValue(totalMemory - usedMemory);
                    ui->totalMemory->setText(QString::number((int32_t)totalMemory));
                    ui->availableMemory->setText(QString::number((int32_t)(totalMemory - usedMemory)));
                    recived.clear();
                    ui->listWidget->clear();
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
                    } else if ((extension == "bmp") || (extension == "jpg") || (extension == "png")) {
                        icon = ":/new/icons/picture.png";
                    } else if ((extension == "wav") || (extension == "mp3") || (extension == "mid")) {
                        icon = ":/new/icons/music.png";
                    } else {
                        icon = ":/new/icons/unknown.png";
                    }
                    QListWidgetItem * item = new QListWidgetItem(QIcon(icon), Name); // + ": " + QString::number(sizeOfFile) + " B"
                    ui->listWidget->addItem(item);
                }
            }

            else if (mode == "REMOVE") {
                if (recived.length() == 1) {
                    if (recived[0] == 'R') {
                        mode = "MEMORY";
                        serial.write("M");
                    }
                    else if (recived[0] == 'r') {
                        ui->statusbar->showMessage("E: can't remove the file");
                    }
                    else {}
                    recived.clear();
                }
            }

            else if (mode == "ERASE") {
                if (recived.length() == 1) {
                    if (recived[0] == 'E') {
                        mode = "MEMORY";
                        serial.write("M");
                    }
                    else if (recived[0] == 'e') {
                        ui->statusbar->showMessage("E: can't erase all files");
                    }
                    else {}
                    recived.clear();
                }
            }

        });
        connected = true;
    } else {
        ui->pushButton_3->setText("connect");

        QObject::disconnect(&serial);

        serial.close();
        connected = false;
    }
}

void MainWindow::on_listWidget_itemClicked(QListWidgetItem *item)
{
    ui->statusbar->showMessage("");
}

void MainWindow::on_pushButton_6_clicked()
{
    ui->comboBox->clear();
    QList<QSerialPortInfo> com_ports = QSerialPortInfo::availablePorts();
    QSerialPortInfo port;
    foreach(port, com_ports)
    {
        ui->comboBox->addItem(port.portName());
    }
    int index = ui->comboBox->findText("cu.SLAB_USBtoUART");
    if ( index != -1 ) {
       ui->comboBox->setCurrentIndex(index);
    }
}

void MainWindow::on_pushButton_4_clicked()
{
    if (!connected || !ui->listWidget->currentItem()) return;
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setText("Please, confirm removing the file");
    msgBox.setInformativeText("Do you want to delete " + (ui->listWidget->currentItem()->text()) + "?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    int r = msgBox.exec();
    if (r == QMessageBox::Yes) {
        mode = "REMOVE";
        QByteArray sizeOfFilename;
        sizeOfFilename.append(((ui->listWidget->currentItem()->text()).length() >> 24) & 0xFF);
        sizeOfFilename.append(((ui->listWidget->currentItem()->text()).length() >> 16) & 0xFF);
        sizeOfFilename.append(((ui->listWidget->currentItem()->text()).length() >> 8) & 0xFF);
        sizeOfFilename.append((ui->listWidget->currentItem()->text()).length() & 0xFF);
        serial.write("R");
        serial.write(sizeOfFilename);
        serial.write((ui->listWidget->currentItem()->text()).toUtf8().constData());
    }
}

void MainWindow::on_pushButton_5_clicked()
{
    if (!connected) return;
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setText("Please, confirm erasing ALL files");
    msgBox.setInformativeText("Do you want to erase?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    int r = msgBox.exec();
    if (r == QMessageBox::Yes) {
        mode = "ERASE";
        serial.write("E");
    }
}
