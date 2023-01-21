#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "cxxmidi/cxxmidi/file.hpp"
#include "cxxmidi/cxxmidi/output/default.hpp"
#include "cxxmidi/cxxmidi/player/player_async.hpp"
#include "cxxmidi/cxxmidi/player/player_sync.hpp"
#include "cxxmidi/cxxmidi/note.hpp"

#include <QWidget>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->connectButton, &QPushButton::clicked, this, &MainWindow::connectButtonHit);
    connect(ui->refreshButton, &QPushButton::clicked, this, &MainWindow::updateRoomList);
    connect(ui->addNewRoomButton, &QPushButton::clicked, this, &MainWindow::createRoomAndJoin);

    dt = cxxmidi::converters::Us2dt(
        500000,
        500000,
        500
    );

    for (int chuj = 0; chuj < 10; ++chuj) {
        track.push_back(
            cxxmidi::Event(0, cxxmidi::Message::kNoteOn, cxxmidi::Note::MiddleC(), 100)
        );
                track.push_back(
                    cxxmidi::Event(dt, cxxmidi::Message::kNoteOn, cxxmidi::Note::MiddleC() + 1, 100)
                );
                track.push_back(
                    cxxmidi::Event(dt, cxxmidi::Message::kNoteOn, cxxmidi::Note::MiddleC() + 2, 100)
                );
                track.push_back(
                    cxxmidi::Event(dt, cxxmidi::Message::kNoteOn, cxxmidi::Note::MiddleC() + 3, 100)
                );
    }


    track.push_back(
                cxxmidi::Event(0, cxxmidi::Message::kMeta, cxxmidi::Message::kEndOfTrack));

    cxxmidi::output::Default output;

    cxxmidi::player::PlayerAsync player(&output);

    player.SetFile(&f);

    player.Play();
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
    send("list | ");
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
    QString command = QString::fromUtf8(ba);
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
    auto splitCommand = command.split("|");
    bool handledAction = false;

    QString action = "";
    std::vector<QString> params = {};

    for (auto x : splitCommand) {
        if (!handledAction) {
            action = x;
            handledAction = true;
        } else {
            params.push_back(x);
        }
    }

    if (action == "JOIN") {
        ui->roomIdLabel->setText("XD");
        ui->addNewRoomButton->setEnabled(false);
    }

}

void MainWindow::send(char * data) {
    sock->write(data);
    printToDebug(data, "SENT");
}

void MainWindow::createRoomAndJoin() {
    send("create | ");
    toJoin = true;
}
