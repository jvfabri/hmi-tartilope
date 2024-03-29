#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    _is_on = false;
    _xpos = 0;
    _ypos = 0;
    _current_speed_x = 100;
    _current_speed_y = 100;
    _xvel_default = 100;
    _yvel_default = 100;
    _steps_per_mm_x = 1;
    _steps_per_mm_y = 1;
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timer_info_update()));
    timer->setInterval(1000*ui->info_period->value());
    connect(ui->act_settings,SIGNAL(triggered(bool)),this,SLOT(change_page_set()));
    connect(ui->act_simplified,SIGNAL(triggered(bool)),this,SLOT(change_page_com()));
    connect(ui->act_help,SIGNAL(triggered(bool)),this,SLOT(change_page_help()));

    ui->frame_holder->deleteLater();
    if(!readJSON("preferences.json")){
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setWindowTitle("Comando para Manipulador de Tocha de Soldagem");
        msgBox.setText("\nNão existem preferências salvas!\n\nUm novo arquivo será criado e as configurações\n padrão serão utilizadas.");
        msgBox.exec();
        if(!writeJSON("preferences.json")){
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setWindowTitle("Comando para Manipulador de Tocha de Soldagem");
            msgBox.setText("\nNão foi possível salvar as configurações!");
            msgBox.exec();
        }
    }
}

MainWindow::~MainWindow()
{
    QMessageBox msgBox;
    int ret = QMessageBox::No;
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setWindowTitle("Comando para Manipulador de Tocha de Soldagem");
    msgBox.setText("Salvar as configurações atuais?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    ret = msgBox.exec();
    if(ret == QMessageBox::Yes){
        writeJSON("preferences.json");
    }
    delete ui;
}

bool MainWindow::not_idle_warning(){
    if (!ui->status_menu->title().contains("Ocioso") && !ui->status_menu->title().contains("Finalizado") && !ui->status_menu->title().contains("Interrompido")){
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setWindowTitle("Comando para Manipulador de Tocha de Soldagem");
        msgBox.setText("Máquina está ocupada, não é possível enviar o comando.");
        msgBox.exec();
        return false;
    }
    return true;
}

bool MainWindow::readJSON(QString name){
    QFile loadFile(name);
    if (!loadFile.open(QIODevice::ReadOnly)) {
        return false;
    }
    QByteArray saveData = loadFile.readAll();

    QMessageBox msgBox;
    int ret = QMessageBox::No;
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setWindowTitle("Comando para Manipulador de Tocha de Soldagem");
    msgBox.setText("Existem configurações previamente salvas, \ndeseja utilizá-las?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    ret = msgBox.exec();

    if(ret == QMessageBox::No){
        QMessageBox msgBox;
        msgBox.setText("Posição atual marcada como origem.\nConfigurações modificadas para os valores padrão.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
        _xpos = 0;
        _ypos = 0;
        _current_speed_x = 100;
        _current_speed_y = 100;
        _xvel_default = 100;
        _yvel_default = 100;
        _steps_per_mm_x = 1;
        _steps_per_mm_y = 1;
    } else if (ret == QMessageBox::Yes){
        QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
        QJsonObject json = loadDoc.object();
        if(json.contains("xpos") && json["xpos"].isDouble()) {
            _xpos = json["xpos"].toInt();
            ui->xpos->setText("X: "+QString::number(_xpos));
        }
        if(json.contains("ypos") && json["ypos"].isDouble()) {
            _ypos = json["ypos"].toInt();
            ui->ypos->setText("Y: "+QString::number(_ypos));
        }
        if(json.contains("xvel_default") && json["xvel_default"].isDouble()) {
            _xvel_default = json["xvel_default"].toInt();
            ui->xvel_default->setValue(_xvel_default);
        }
        if(json.contains("yvel_default") && json["yvel_default"].isDouble()) {
            _yvel_default = json["yvel_default"].toInt();
            ui->yvel_default->setValue(_yvel_default);
        }
        if(json.contains("steps_per_mmx") && json["steps_per_mmx"].isDouble()) {
            _steps_per_mm_x = json["steps_per_mmx"].toInt();
            ui->steps_mm_x->setValue(_steps_per_mm_x);
        }
        if(json.contains("steps_per_mmy") && json["steps_per_mmy"].isDouble()) {
            _steps_per_mm_y = json["steps_per_mmy"].toInt();
            ui->steps_mm_y->setValue(_steps_per_mm_y);
        }
        if(json.contains("info_period") && json["info_period"].isDouble()) {
            ui->info_period->setValue(json["info_period"].toDouble());
        }
        if(json.contains("jogsteps") && json["jogsteps"].isDouble()) {
            ui->jogsteps->setValue(json["jogsteps"].toDouble());
        }
        _current_speed_x = _xvel_default;
        _current_speed_y = _yvel_default;
    }
    return true;
}

bool MainWindow::writeJSON(QString name){
    QFile saveFile(name);

   if (!saveFile.open(QIODevice::WriteOnly)) {
       qWarning("Não foi possível abrir o arquivo.");
       return false;
   }

   QJsonObject saveDoc;
   saveDoc["xpos"] = _xpos ;
   saveDoc["ypos"] = _ypos ;
   saveDoc["xvel_default"] = _xvel_default;
   saveDoc["yvel_default"] = _yvel_default;
   saveDoc["steps_per_mmx"] = _steps_per_mm_x;
   saveDoc["steps_per_mmy"] = _steps_per_mm_y;
   saveDoc["info_period"] = ui->info_period->value();
   saveDoc["jogsteps"] = ui->jogsteps->value();

   saveFile.write(QJsonDocument(saveDoc).toJson());
   if (!not_idle_warning()) return true;
   QString data = "xjogsteps="+QString::number(ui->jogsteps->value()*ui->steps_mm_x->value())+"&yjogsteps="+QString::number(ui->jogsteps->value()*ui->steps_mm_y->value())+"&xv="+QString::number(ui->xvel_default->value())+"&yv="+QString::number(ui->yvel_default->value());
   QNetworkAccessManager * mgr = new QNetworkAccessManager(this);
   connect(mgr,SIGNAL(finished(QNetworkReply*)),this,SLOT(onfinish(QNetworkReply*)));
   connect(mgr,SIGNAL(finished(QNetworkReply*)),mgr,SLOT(deleteLater()));
   QNetworkRequest request(QUrl("http://192.168.4.1/CONFIG"));
   request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
   mgr->post(request,data.toLocal8Bit());

   return true;
}

void MainWindow::send_move_command(int x, int y, int xv, int yv){
    QString data = "x="+QString::number(x)+"&y="+QString::number(y)+"&xv="+QString::number((xv*ui->steps_mm_x->value()*6))+"&yv="+QString::number((yv*ui->steps_mm_y->value()*6));
    QNetworkAccessManager * mgr = new QNetworkAccessManager(this);
    connect(mgr,SIGNAL(finished(QNetworkReply*)),this,SLOT(onfinish(QNetworkReply*)));
    connect(mgr,SIGNAL(finished(QNetworkReply*)),mgr,SLOT(deleteLater()));
    QNetworkRequest request(QUrl("http://192.168.4.1/COMMAND"));
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    mgr->post(request,data.toLocal8Bit());
    qDebug() << data;
    timer->start();
}

void MainWindow::onfinishINFO(QNetworkReply *rep)
{   //handles http replies to update the system variables regularly.
    //could use the state flag from TIVA to update timer's cycle to reduce processing overhead
    QString bts = rep->readAll(), xpos = "", ypos="";
    int x,y;
    xpos = bts.mid(bts.indexOf("x=")+2,bts.indexOf("&",bts.indexOf("x=")+1)-bts.indexOf("x=")-2);
    x = xpos.toInt()/ui->steps_mm_x->value();
    ui->xpos->setText("X: "+QString::number(x));
    ypos = bts.mid(bts.indexOf("y=")+2,bts.indexOf("&",bts.indexOf("y=")+1)-bts.indexOf("y=")-2);
    y = ypos.toInt()/ui->steps_mm_y->value();
    ui->ypos->setText("Y: "+QString::number(y));
    QString status = bts.mid(bts.indexOf("&",bts.indexOf("y=")+1)+1);
    qDebug() << status << xpos <<" - "<<ypos;
    if (status=="Done\n" || status=="Halted\n"){
        _xpos = x;
        _ypos = y;
    }
    if (status=="Running\n")
        ui->status_menu->setTitle("Status: Movendo");
    else if (status=="Done\n")
        ui->status_menu->setTitle("Status: Finalizado");
    else if (status=="Halted\n")
        ui->status_menu->setTitle("Status: Interrompido");
    else
        ui->status_menu->setTitle("Status: Desconhecido");
}

void MainWindow::onfinish(QNetworkReply *rep){   
    //handles http replies to update the system variables regularly.
    //could use the state flag from TIVA to update timer's cycle to reduce processing overhead
}

void MainWindow::onfinish_sys(QNetworkReply *rep){
    if(_is_on){
        ui->connect_sys->setText("Conectar \naos Motores");
        ui->connect_sys->setStyleSheet("QPushButton {color : green}");
        _is_on = false;
        ui->move_axis_menu->setEnabled(false);
        ui->jog_menu->setEnabled(false);
        ui->linear_task_menu->setEnabled(false);
    } else {
        ui->connect_sys->setText("Desconectar");
        ui->connect_sys->setStyleSheet("QPushButton {color : red}");
        _is_on = true;
        ui->move_axis_menu->setEnabled(true);
        ui->jog_menu->setEnabled(true);
        ui->linear_task_menu->setEnabled(true);
    }
}

void MainWindow::on_halt_clicked()
{
    QNetworkAccessManager * mgr = new QNetworkAccessManager(this);
    connect(mgr,SIGNAL(finished(QNetworkReply*)),this,SLOT(onfinishINFO(QNetworkReply*)));
    connect(mgr,SIGNAL(finished(QNetworkReply*)),mgr,SLOT(deleteLater()));

    mgr->post(QNetworkRequest(QUrl("http://192.168.4.1/HALT")),"");
}

void MainWindow::timer_info_update()
{
    if (ui->info_check->isChecked()){
        QNetworkAccessManager * mgr = new QNetworkAccessManager(this);
        connect(mgr,SIGNAL(finished(QNetworkReply*)),this,SLOT(onfinishINFO(QNetworkReply*)));
        connect(mgr,SIGNAL(finished(QNetworkReply*)),mgr,SLOT(deleteLater()));
        mgr->get(QNetworkRequest(QUrl("http://192.168.4.1/INFO")));
    }
}

void MainWindow::on_connect_sys_clicked(){
    QNetworkAccessManager * mgr = new QNetworkAccessManager(this);
    connect(mgr,SIGNAL(finished(QNetworkReply*)),this,SLOT(onfinish_sys(QNetworkReply*)));
    connect(mgr,SIGNAL(finished(QNetworkReply*)),mgr,SLOT(deleteLater()));
    if (_is_on){
        mgr->post(QNetworkRequest(QUrl("http://192.168.4.1/OFF")),"");
    } else {
        mgr->post(QNetworkRequest(QUrl("http://192.168.4.1/ON")),"");
    }
}

void MainWindow::change_page_com(){
    if (ui->stackedWidget->currentIndex()==1 || ui->stackedWidget->currentIndex()==2){
        ui->position_label->setParent(ui->status_menu);
        ui->stackedWidget->setCurrentIndex(0);
    }
}

void MainWindow::change_page_set(){
    if (ui->stackedWidget->currentIndex()==0 || ui->stackedWidget->currentIndex()==2){
        ui->position_label->setParent(ui->info_placeholder);
        ui->stackedWidget->setCurrentIndex(1);
    }
}

void MainWindow::change_page_help(){
    ui->stackedWidget->setCurrentIndex(2);
}

void MainWindow::on_sendTask_clicked(){
    if (!not_idle_warning()) return;
    timer->stop();
    int xsteps=0,ysteps=0;
    float xvel=0,yvel=0;
    xsteps = ui->steps_mm_x->value()*(ui->x_finish->value() - _xpos);
    ysteps = ui->steps_mm_y->value()*(ui->y_finish->value() - _ypos);
    if(xsteps!=0 || ysteps!=0) xvel = floor(ui->speed_spin->value() * (xsteps) / sqrt(pow(xsteps,2) + pow(ysteps,2)));
    if(xsteps!=0 || ysteps!=0) yvel = floor(ui->speed_spin->value() * (ysteps) / sqrt(pow(xsteps,2) + pow(ysteps,2)));
    if(xvel==0) xvel=ui->xvel_default->value();
    if(yvel==0) yvel=ui->yvel_default->value();
    xsteps = abs(xsteps);
    ysteps = abs(ysteps);
    _current_speed_x = xvel;
    _current_speed_y = yvel;
    send_move_command(xsteps, ysteps, (int) xvel, (int) yvel);
    timer->start();
}

void MainWindow::on_xplus_clicked()
{
    QNetworkAccessManager * mgr = new QNetworkAccessManager(this);
    connect(mgr,SIGNAL(finished(QNetworkReply*)),this,SLOT(onfinish(QNetworkReply*)));
    connect(mgr,SIGNAL(finished(QNetworkReply*)),mgr,SLOT(deleteLater()));

    mgr->post(QNetworkRequest(QUrl("http://192.168.4.1/JOGX+")),"");
    _current_speed_x = 100;
    timer->start();
}

void MainWindow::on_xminus_clicked()
{
    QNetworkAccessManager * mgr = new QNetworkAccessManager(this);
    connect(mgr,SIGNAL(finished(QNetworkReply*)),this,SLOT(onfinish(QNetworkReply*)));
    connect(mgr,SIGNAL(finished(QNetworkReply*)),mgr,SLOT(deleteLater()));

    mgr->post(QNetworkRequest(QUrl("http://192.168.4.1/JOGX-")),"");
    _current_speed_x = -100;
    timer->start();
}

void MainWindow::on_yplus_clicked()
{
    QNetworkAccessManager * mgr = new QNetworkAccessManager(this);
    connect(mgr,SIGNAL(finished(QNetworkReply*)),this,SLOT(onfinish(QNetworkReply*)));
    connect(mgr,SIGNAL(finished(QNetworkReply*)),mgr,SLOT(deleteLater()));

    mgr->post(QNetworkRequest(QUrl("http://192.168.4.1/JOGY+")),"");
    _current_speed_y = 100;
    timer->start();
}

void MainWindow::on_yminus_clicked()
{
    QNetworkAccessManager * mgr = new QNetworkAccessManager(this);
    connect(mgr,SIGNAL(finished(QNetworkReply*)),this,SLOT(onfinish(QNetworkReply*)));
    connect(mgr,SIGNAL(finished(QNetworkReply*)),mgr,SLOT(deleteLater()));

    mgr->post(QNetworkRequest(QUrl("http://192.168.4.1/JOGY-")),"");
    _current_speed_y = -100;
    timer->start();
}


void MainWindow::on_xvel_default_valueChanged(int arg1)
{
    _xvel_default = arg1;
}

void MainWindow::on_yvel_default_valueChanged(int arg1)
{
    _yvel_default = arg1;
}

void MainWindow::on_movex_button_clicked()
{
    if (!not_idle_warning()) return;
    timer->stop();
    int x = abs(ui->movex->value())*_steps_per_mm_x;
    _current_speed_x = (ui->movex->value()>=0) ? (_xvel_default): (-_xvel_default);
    send_move_command(x, 0, _current_speed_x , _yvel_default);
    timer->start();
}

void MainWindow::on_movey_button_clicked()
{
    if (!not_idle_warning()) return;
    timer->stop();
    int y = abs(ui->movey->value())*_steps_per_mm_y;
    _current_speed_y = (ui->movey->value()>=0) ? (_yvel_default): (-_yvel_default);
    send_move_command(0, y, _xvel_default, _current_speed_y);
    timer->start();
}

void MainWindow::on_save_settings_clicked()
{
    QMessageBox msgBox;
    int ret = QMessageBox::No;
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setWindowTitle("Comando para Manipulador de Tocha de Soldagem");
    msgBox.setText("Arquivo existente será sobrescrito e a máquina será configurada.\nContinuar?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    ret = msgBox.exec();
    if(ret == QMessageBox::Yes){
        writeJSON("preferences.json");
    }
}

void MainWindow::on_info_period_valueChanged(double arg1)
{
    timer->setInterval((1000*arg1));
}

void MainWindow::on_set_ref_clicked()
{
    _xpos = 0;
    _ypos = 0;
    ui->xpos->setText("X: "+QString::number(_xpos));
    ui->ypos->setText("Y: "+QString::number(_ypos));
    QNetworkAccessManager * mgr = new QNetworkAccessManager(this);
    connect(mgr,SIGNAL(finished(QNetworkReply*)),this,SLOT(onfinish(QNetworkReply*)));
    connect(mgr,SIGNAL(finished(QNetworkReply*)),mgr,SLOT(deleteLater()));
    mgr->post(QNetworkRequest(QUrl("http://192.168.4.1/ORIGIN")),"");

}


void MainWindow::on_steps_mm_y_valueChanged(int arg1)
{
    _steps_per_mm_y = arg1;
}

void MainWindow::on_steps_mm_x_valueChanged(int arg1)
{
    _steps_per_mm_x = arg1;
}
