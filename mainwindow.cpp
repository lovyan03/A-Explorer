#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSerialPortInfo>
#include <QSerialPort>
#include <QFileDialog>
#include <QMessageBox>

bool connected = false;
QSerialPort serial;
QFile file;
size_t totalMemory;
size_t usedMemory;
QByteArray recived;
QString mode = "";
int sizeOfFile = 0;
QByteArray dataBytes;
int writeBytesLength;
size_t sizeOfData = 0;

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

QString getExtension(QString Name) {
    QString extension;
    if ((Name.indexOf('.') > 0) <= 0) {
        extension = "";
    } else {
        extension = (Name.split(".")[1]).toLower();
    }
    return extension;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    on_pushButton_6_clicked();
    setWindowFlags(Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint);

    ui->tableWidget->setColumnCount(3);
    ui->tableWidget->setColumnWidth(0, 28);
    ui->tableWidget->setColumnWidth(1, 230);
    ui->tableWidget->setColumnWidth(2, 77);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
}

MainWindow::~MainWindow()
{
    delete ui;
}

// download
void MainWindow::on_pushButton_clicked()
{
    QMessageBox msgBox;
    if (!connected) {
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setText("connect to device, please");
        msgBox.exec();
        return;
    }
    if (mode != "") {
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("device is busy now");
        msgBox.exec();
        return;
    }
    if (ui->tableWidget->currentRow() == -1) {
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setText("select file, please");
        msgBox.exec();
        return;
    }
    QString folder = QFileDialog::getExistingDirectory();
    if (folder.length() == 0) {
        ui->statusbar->showMessage("C: empty downloading path");
        return;
    }
    /* ВЫБОР ИМЕНИ СКАЧИВАЕМОГО ФАЙЛА */
    QString fileName = ui->tableWidget->selectedItems().at(1)->text();
    size_t sizeOfFileName = fileName.length();
    QString savingName_ = fileName;
    savingName_.replace('/', '-');
    QString savingName = folder + "/" + savingName_.remove(0, 1);
    ui->statusbar->showMessage(savingName);

    file.setFileName(savingName);

    QByteArray sizeOfFileNameBytes;
    /* ПОЛУЧЕНИЕ РАЗМЕРА ИМЕНИ СКАЧИВАЕМОГО ФАЙЛА */
    sizeOfFileNameBytes.append((sizeOfFileName >> 24) & 0xFF);
    sizeOfFileNameBytes.append((sizeOfFileName >> 16) & 0xFF);
    sizeOfFileNameBytes.append((sizeOfFileName >> 8) & 0xFF);
    sizeOfFileNameBytes.append(sizeOfFileName & 0xFF);

    ui->statusbar->showMessage("downloading, please wait...");

    /* ДЕЙСТВИЯ С ПОСЛЕДОВАТЕЛЬНЫМ ПОРТОМ */
    mode = "DOWNLOAD";
    serial.write("D");
    serial.write(sizeOfFileNameBytes);
    serial.write(fileName.toUtf8().constData());
}

// upload
void MainWindow::on_pushButton_2_clicked()
{
    QMessageBox msgBox;
    if (!connected) {
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setText("connect to device, please");
        msgBox.exec();
        return;
    }
    if (mode != "") {
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("device is busy now :" + mode);
        msgBox.exec();
        return;
    }
    /* ВЫБОР ИМЕНИ ФАЙЛА ВЫГРУЖАЕМОГО ФАЙЛА */
    QString fullFileName = QFileDialog::getOpenFileName();
    if (fullFileName.length() == 0) {
        ui->statusbar->showMessage("E: empty uploading path");
        return;
    }
    QStringList qst = fullFileName.split("/");
    QString fileName = qst[qst.size() - 1];
    // фильтр для защиты всей памяти от неверного формата имени
    fileName.remove(QRegExp("[^A-Za-z0-9_.-']"));
    fileName = fileName.mid(0, 256);
    if ( (fileName.indexOf('.') > -1) && ((fileName.split('.')[0].length() == 0) || (fileName.split('.')[1].length() == 0))) {
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText("incorrect file name");
        msgBox.exec();
        return;
    }
    fileName.prepend("/");

    size_t sizeOfFileName = fileName.length();

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

    if ((size_t)sizeOfFile >= (totalMemory - usedMemory)) {
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("E: not enough free space");
        msgBox.exec();
        f.close();
        return;
    }
    QByteArray sizeOfFilBytes;
    sizeOfFilBytes.append((dataBytes.length() >> 24) & 0xFF);
    sizeOfFilBytes.append((dataBytes.length() >> 16) & 0xFF);
    sizeOfFilBytes.append((dataBytes.length() >> 8) & 0xFF);
    sizeOfFilBytes.append(dataBytes.length() & 0xFF);
    ui->statusbar->showMessage("uploading...");

    /* ДЕЙСТВИЯ С ПОСЛЕДОВАТЕЛЬНЫМ ПОРТОМ */
    recived.clear();
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
    ui->tableWidget->clear();
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
            if (mode == "REBOOT") {
               mode = "";
               recived.clear();
            }

           if  (mode == "DOWNLOAD") {
                if (sizeOfFile == 0) {
                    if (recived.length() == 1) {
                        if (recived[0] == 'd') {
                            recived.clear();
                            mode = "";
                            sizeOfFile = 0;
                            ui->statusbar->showMessage("E: can't download the file");
                        }
                    }
                    else if (recived.length() >= 4) {
                        sizeOfFile = (scti(recived[0]) << 24) | (scti(recived[1]) << 16) | (scti(recived[2]) << 8) | scti(recived[3]);
                        recived.remove(0, 4);
                    }
                }
                else if (recived.length() == sizeOfFile) {
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
                    totalMemory = (scti(recived[0]) << 24) | (scti(recived[1]) << 16) | (scti(recived[2]) << 8) | scti(recived[3]);
                    usedMemory = (scti(recived[4]) << 24) | (scti(recived[5]) << 16) | (scti(recived[6]) << 8) | scti(recived[7]);
                    ui->memory->setRange(0, totalMemory);
                    ui->memory->setValue(totalMemory - usedMemory);
                    ui->totalMemory->setText(QString::number(totalMemory));
                    ui->availableMemory->setText(QString::number((totalMemory - usedMemory)));
                    recived.clear();
                    mode = "LIST";
                    serial.write("L");
                }
            }

            else if (mode == "LIST") {
               if ((recived.length() >= 4) && (sizeOfData == 0)) {
                    ui->tableWidget->setRowCount(0);
                    ui->tableWidget->clear();
                    sizeOfData = (scti(recived[0]) << 24) | (scti(recived[1]) << 16) | (scti(recived[2]) << 8) | scti(recived[3]);
                    recived.remove(0, 4);
               }
               if (((size_t)recived.length() == sizeOfData) && sizeOfData) {
                   // вот тут всё самое интересное! ;)
                   while (recived.length()) {
                       uint8_t sizeOfName = (scti(recived[0]) << 24) | (scti(recived[1]) << 16) | (scti(recived[2]) << 8) | scti(recived[3]);
                       size_t sizeOfFile = sizeOfFile = (scti(recived[4]) << 24) | (scti(recived[5]) << 16) | (scti(recived[6]) << 8) | scti(recived[7]);
                       recived.remove(0, 8);
                       QString Name = "";
                       for (size_t i = 0; i < sizeOfName; i++) {
                           Name.append(recived.at(i));
                       }
                       recived.remove(0, sizeOfName);
                       QString extension = getExtension(Name);

                       QString iconPath;
                       if (extension == "txt") {
                           iconPath = ":/new/icons/text.png";
                       } else if ((extension == "bmp") || (extension == "jpg") || (extension == "png")) {
                           iconPath = ":/new/icons/picture.png";
                       } else if ((extension == "wav") || (extension == "mp3") || (extension == "mid")) {
                           iconPath = ":/new/icons/music.png";
                       } else if ((extension == "ini") || (extension == "js") || (extension == "css") || (extension == "xml")) {
                           iconPath = ":/new/icons/engine.png";
                       } else if ((extension == "html") || (extension == "htm")) {
                           iconPath = ":/new/icons/world.png";
                       } else {
                           iconPath = ":/new/icons/unknown.png";
                       }
                       QIcon icon(iconPath);
                       QTableWidgetItem *icon_item = new QTableWidgetItem;
                       icon_item->setIcon(icon);
                       int row = ui->tableWidget->rowCount();
                       ui->tableWidget->insertRow(row);
                       ui->tableWidget->setItem(row, 0, icon_item);
                       ui->tableWidget->setItem(row, 1, new QTableWidgetItem(Name));
                       ui->tableWidget->setItem(row, 2, new QTableWidgetItem(QString::number(sizeOfFile)));
                       ui->tableWidget->setHorizontalHeaderLabels(QStringList() << "" << "Name" << "Size, Bytes");
                       ui->tableWidget->verticalHeader()->hide();
                   }
                   mode = "";
                   sizeOfData = 0;
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

            else if (mode == "EXECUTE") {
                if (recived.length() == 1) {
                    if (recived[0] == 'X') {
                        ui->statusbar->showMessage("file succefull executed");
                    }
                    else if (recived[0] == 'x') {
                        ui->statusbar->showMessage("E: file not executed");
                    }
                }
                recived.clear();
                mode = "";
            }
            // more, baby
        });
        connected = true;
    } else {

        ui->pushButton_3->setText("connect");
        QObject::disconnect(&serial);

        ui->tableWidget->clear();
        ui->tableWidget->setRowCount(0);

        recived.clear();
        serial.close();
        connected = false;
    }
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
    QMessageBox msgBox;
    if (!connected) {
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setText("connect to device, please");
        msgBox.exec();
        return;
    }
    if (mode != "") {
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("device is busy now");
        msgBox.exec();
        return;
    }
    if (ui->tableWidget->currentRow() == -1) {
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setText("select file, please");
        msgBox.exec();
        return;
    }
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setText("Do you want to delete " + ui->tableWidget->selectedItems().at(1)->text() + "?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    int r = msgBox.exec();
    if (r == QMessageBox::Yes) {
        QString Name = ui->tableWidget->selectedItems().at(1)->text();
        QByteArray sizeOfFilename;
        sizeOfFilename.append((Name.length() >> 24) & 0xFF);
        sizeOfFilename.append((Name.length() >> 16) & 0xFF);
        sizeOfFilename.append((Name.length() >> 8) & 0xFF);
        sizeOfFilename.append(Name.length() & 0xFF);
        mode = "REMOVE";
        serial.write("R");
        serial.write(sizeOfFilename);
        serial.write(Name.toUtf8().constData());
    }
}

void MainWindow::on_pushButton_5_clicked()
{
    QMessageBox msgBox;
    if (!connected) {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setText("connect to device, please");
        msgBox.exec();
        return;
    }
    if (mode != "") {
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("device is busy now");
        msgBox.exec();
        return;
    }
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setText("Please, confirm to erasing ALL files");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    int r = msgBox.exec();
    if (r == QMessageBox::Yes) {
        mode = "ERASE";
        serial.write("E");
    }
}

void MainWindow::on_pushButton_7_clicked()
{
    QMessageBox msgBox;
    if (!connected) {
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setText("connect to device, please");
        msgBox.exec();
        return;
    }
    if (mode != "") {
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("device is busy now");
        msgBox.exec();
        return;
    }
    if (ui->tableWidget->currentRow() == -1) {
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setText("select file, please");
        msgBox.exec();
        return;
    }
    QString Name = ui->tableWidget->selectedItems().at(1)->text();
    if (getExtension(Name) == "") {
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setText("unsupported file for execution");
        msgBox.exec();
        return;
    }
    QByteArray sizeOfFilename;
    sizeOfFilename.append((Name.length() >> 24) & 0xFF);
    sizeOfFilename.append((Name.length() >> 16) & 0xFF);
    sizeOfFilename.append((Name.length() >> 8) & 0xFF);
    sizeOfFilename.append(Name.length()  & 0xFF);
    mode = "EXECUTE";
    serial.write("X");
    serial.write(sizeOfFilename);
    serial.write(Name.toUtf8().constData());
}

void MainWindow::on_pushButton_8_clicked()
{
    QMessageBox msgBox;
    if (!connected) {
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setText("connect to device, please");
        msgBox.exec();
        return;
    }
    if (mode != "") {
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("device is busy now");
        msgBox.exec();
        return;
    }
    mode = "REBOOT";
    serial.write("Q");
}
