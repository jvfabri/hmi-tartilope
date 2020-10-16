#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QMessageBox>
#include <QFile>
#include <QFileDialog>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QDebug>
#include <QErrorMessage>
#include <QTimer>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include "math.h"

#define JOG_STEPS 100;


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    int _xpos, _current_speed_x,  _xvel_default;
    int _ypos, _current_speed_y,  _yvel_default;
    int _steps_per_mm;
    bool _is_on;

private slots:
    void onfinish(QNetworkReply *rep);
    void onfinish_sys(QNetworkReply *rep);
    void onfinishINFO(QNetworkReply *rep);
    void on_send_file_clicked();
    void on_halt_clicked();
    void on_sendTask_clicked();
    void on_ledButton_clicked();
    void on_connect_sys_clicked();
    void on_xplus_clicked();
    void on_xminus_clicked();
    void on_yplus_clicked();
    void on_yminus_clicked();
    void on_movex_button_clicked();
    void on_movey_button_clicked();
    void timer_info_update();
    void change_page();
    void on_steps_mm_valueChanged(int arg1);
    void on_xvel_default_valueChanged(int arg1);
    void on_yvel_default_valueChanged(int arg1);
    void on_save_settings_clicked();
    void on_info_period_valueChanged(double arg1);
    void on_set_ref_clicked();

private:
    Ui::MainWindow *ui;
    QTimer * timer;
    void send_move_command(int x, int y, int xv, int yv);
    bool readJSON(QString name);
    bool writeJSON(QString name);
    void not_idle_warning();
};

#endif // MAINWINDOW_H
