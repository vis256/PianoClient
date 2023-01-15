#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QWidget>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->connectButton, &QPushButton::clicked, this, &MainWindow::connectButtonHit);
    updateRoomList();
    updateRoomList();
    updateRoomList();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::handleKeyPress(int key) {
    sock->write("NOTE 42");
}

void MainWindow::connectButtonHit() {
    ui->loginBox->setEnabled(false);
    ui->StatusLabel->setText( "Connecting" );

    if(sock)
        delete sock;
    sock = new QTcpSocket(this);
    connTimeoutTimer = new QTimer(this);
    connTimeoutTimer->setSingleShot(true);
    connect(connTimeoutTimer, &QTimer::timeout, [&]{
        sock->abort();
        sock->deleteLater();
        sock = nullptr;
        connTimeoutTimer->deleteLater();
        connTimeoutTimer=nullptr;
        ui->loginBox->setEnabled(true);
        ui->StatusLabel->setText( "Connection timed out" );
    });

    connect(sock, &QTcpSocket::connected, this, &MainWindow::socketConnected);
    connect(sock, &QTcpSocket::disconnected, this, &MainWindow::socketDisconnected);
    connect(sock, &QTcpSocket::errorOccurred, this, &MainWindow::socketError);
    connect(sock, &QTcpSocket::readyRead, this, &MainWindow::socketReadable);

    sock->connectToHost(ui->IPLineEdit->text(), ui->PortSpinBox->value());
    connTimeoutTimer->start(3000);

}

void MainWindow::updateRoomList() {
    QPushButton *button = new QPushButton(ui->scrollAreaWidgetContents);
}


void MainWindow::socketConnected(){
    connTimeoutTimer->stop();
    connTimeoutTimer->deleteLater();
    connTimeoutTimer=nullptr;
    ui->RoomGroup->setEnabled(true);

    ui->StatusLabel->setText( "Connected" );
}

void MainWindow::socketDisconnected(){
    ui->RoomGroup->setEnabled(false);
    ui->loginBox->setEnabled(true);

    ui->StatusLabel->setText( "Disconnected" );
}

void MainWindow::socketError(QTcpSocket::SocketError err){
    if(err == QTcpSocket::RemoteHostClosedError)
        ui->StatusLabel->setText( "Remote host closed" );
        return;
    if(connTimeoutTimer){
        connTimeoutTimer->stop();
        connTimeoutTimer->deleteLater();
        connTimeoutTimer=nullptr;
    }
    ui->RoomGroup->setEnabled(false);
    ui->loginBox->setEnabled(true);
    ui->StatusLabel->setText( "Error connecting" );
}

void MainWindow::socketReadable(){
    QByteArray ba = sock->readAll();
    QString command = QString::fromUtf8(ba).trimmed();
    printToDebug(command, "RCVD");
    handleCommand(command);
}

void MainWindow::printToDebug(QString command, QString type) {
    ui->debugTextEdit->append(
                "<div style=\"color: black;\"><b style=\"color: darkblue;\">[" +
                QDateTime::currentDateTime().toString("hh:mm:ss") +
                "] [" + type + "] </b>" + command + "</div>"
    );
}

void MainWindow::handleCommand(QString command) {
    auto c = command.split((" "))[0];

    sock->write("create");
}
