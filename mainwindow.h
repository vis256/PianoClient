#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QTimer>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    QTcpSocket * sock {nullptr};
    QTimer * connTimeoutTimer{nullptr};

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void socketConnected();
    void socketDisconnected();
    void socketError(QTcpSocket::SocketError);
    void socketReadable();


    void printToDebug(QString command, QString type);

    void handleKeyPress(int key);
    void connectButtonHit();

    void handleCommand(QString command);

    void updateRoomList();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
