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
    _steps_per_mm = 1;
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timer_info_update()));
    timer->setInterval(1000*ui->info_period->value());
    connect(ui->act_settings,SIGNAL(triggered(bool)),this,SLOT(change_page()));
    ui->placeholder->deleteLater();
    ui->frame_holder->deleteLater();
    if(!readJSON("preferences.json")){
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("\nThe system has no saved preferences data!\n\nA new file will be created and the current\nposition will be set as origin.");
        msgBox.exec();
        if(!writeJSON("preferences.json")){
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText("\nUnable to create a new preferences file!");
            msgBox.exec();
        }
    }
}

MainWindow::~MainWindow()
{
    QMessageBox msgBox;
    int ret = QMessageBox::No;
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setText("Save current references and settings?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    ret = msgBox.exec();
    if(ret == QMessageBox::Yes){
        writeJSON("preferences.json");
    }
    delete ui;
}

void MainWindow::not_idle_warning(){
    if (!ui->status_menu->title().contains("Idle") && !ui->status_menu->title().contains("Done") && !ui->status_menu->title().contains("Halted")){
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("Machine is busy, cannot send a new task.");
        msgBox.exec();
        return;
    }
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
    msgBox.setText("There are previously saved preferences, use them?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    ret = msgBox.exec();

    if(ret == QMessageBox::No){
        QMessageBox msgBox;
        msgBox.setText("Current position set as origin.\nSettings set to default.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
        _xpos = 0;
        _ypos = 0;
        _current_speed_x = 100;
        _current_speed_y = 100;
        _xvel_default = 100;
        _yvel_default = 100;
        _steps_per_mm = 1;
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
        if(json.contains("steps_per_mm") && json["steps_per_mm"].isDouble()) {
            _steps_per_mm = json["steps_per_mm"].toInt();
            ui->steps_mm->setValue(_steps_per_mm);
        }
        if(json.contains("info_period") && json["info_period"].isDouble()) {
            qDebug() << ui->info_period->value();
            ui->info_period->setValue(json["info_period"].toDouble());
        }
        _current_speed_x = _xvel_default;
        _current_speed_y = _yvel_default;
    }
    return true;
}

bool MainWindow::writeJSON(QString name){
    QFile saveFile(name);

   if (!saveFile.open(QIODevice::WriteOnly)) {
       qWarning("Couldn't open save file.");
       return false;
   }

   QJsonObject saveDoc;
   saveDoc["xpos"] = _xpos ;
   saveDoc["ypos"] = _ypos ;
   saveDoc["xvel_default"] = _xvel_default;
   saveDoc["yvel_default"] = _yvel_default;
   saveDoc["steps_per_mm"] = _steps_per_mm;
   saveDoc["info_period"] = ui->info_period->value();
   saveFile.write(QJsonDocument(saveDoc).toJson());

   return true;
}

void MainWindow::send_move_command(int x, int y, int xv, int yv){
    QString data = "x="+QString::number(x)+"&y="+QString::number(y)+"&xv="+QString::number(xv)+"&yv="+QString::number(yv);
    QNetworkAccessManager * mgr = new QNetworkAccessManager(this);
    connect(mgr,SIGNAL(finished(QNetworkReply*)),this,SLOT(onfinish(QNetworkReply*)));
    connect(mgr,SIGNAL(finished(QNetworkReply*)),mgr,SLOT(deleteLater()));
    QNetworkRequest request(QUrl("http://192.168.4.1/COMMAND"));
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    mgr->post(request,data.toLocal8Bit());
    timer->start();
}

void MainWindow::on_send_file_clicked()
{
    QString fileName;
    if (ui->path_to_file->text().isEmpty())
        fileName = QFileDialog::getOpenFileName(this,
            tr("Open File"), "..", tr("Text files (*.txt);;Images (*.png *.xpm *.jpg);;GCode files (*.g)"));
    else
        fileName = ui->path_to_file->text();

    QFile *file = new QFile(fileName);
    int ret = QMessageBox::Cancel;
    if(!file->open(QIODevice::ReadOnly)){
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("This file does not exist or the path is wrong!\n\nTry clicking the Send File button with the text box empty\nor setting the absolute path to the file.");
        msgBox.exec();
        return;
    } else {
        QMessageBox msgBox;
        msgBox.setText("You are trying to send "+fileName+".");
        msgBox.setInformativeText("Send file to Tartilope V2F?");
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Ok);
        ret = msgBox.exec();
    }
    if(ret == QMessageBox::Cancel)
        return;

    QByteArray line;
    while (!file->atEnd()) {
            line.append( file->readLine());
    }

    QUrl url("http://192.168.4.1/uploaded");
    QNetworkRequest request(url);
    QNetworkAccessManager * manager = new QNetworkAccessManager(this);

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    multiPart->setBoundary("----WebKitFormBoundaryANqbA0PvofyLBDnK");
    QHttpPart textPart, textPart2;
    
    textPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"MAX_FILE_SIZE\""));
    textPart.setBody("100000");
    multiPart->append(textPart);

    textPart2.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"uploadedfile\"; filename=\"circle.txt\""));
    textPart2.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("text/plain"));
    textPart2.setBody(line);
    multiPart->append(textPart2);

    QNetworkReply *reply = manager->post(request,multiPart);
    multiPart->setParent(reply);
    file->setParent(multiPart);

    QMessageBox msgBox;
    msgBox.setText("File Sent!");
    msgBox.exec();
}

void MainWindow::onfinishINFO(QNetworkReply *rep)
{   //handles http replies to update the system variables regularly.
    //could use the state flag from TIVA to update timer's cycle to reduce processing overhead
    QString bts = rep->readAll(), xpos = "", ypos="";
    int x,y;
    xpos = bts.mid(bts.indexOf("x=")+2,bts.indexOf("&",bts.indexOf("x=")+1)-bts.indexOf("x=")-2);
    x = (_current_speed_x>=0) ? (_xpos+xpos.toInt()) : (_xpos-xpos.toInt());
    ui->xpos->setText("X: "+QString::number(x));
    ypos = bts.mid(bts.indexOf("y=")+2,bts.indexOf("&",bts.indexOf("y=")+1)-bts.indexOf("y=")-2);
    y = (_current_speed_y>=0) ? (_ypos+ypos.toInt()) : (_ypos-ypos.toInt());
    ui->ypos->setText("Y: "+QString::number(y));
    QString status = bts.mid(bts.indexOf("&",bts.indexOf("y=")+1)+1);
    qDebug() << status;
    if (status=="Done\n" || status=="Halted\n"){
        _xpos = x;
        _ypos = y;
        timer->stop();
    }
    ui->status_menu->setTitle("Status: "+status);
}

void MainWindow::onfinish(QNetworkReply *rep){   
    //handles http replies to update the system variables regularly.
    //could use the state flag from TIVA to update timer's cycle to reduce processing overhead
}

void MainWindow::onfinish_sys(QNetworkReply *rep){
    if(_is_on){
        ui->connect_sys->setText("Connect to TV2F");
        ui->connect_sys->setStyleSheet("QPushButton {color : green}");
        _is_on = false;
    } else {
        ui->connect_sys->setText("Disconnect");
        ui->connect_sys->setStyleSheet("QPushButton {color : red}");
        _is_on = true;
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

void MainWindow::change_page(){
    if (ui->stackedWidget->currentIndex()==1){
        ui->jog_menu->setParent(ui->jog_original);
        ui->position_label->setParent(ui->status_menu);
        ui->stackedWidget->setCurrentIndex(0);
        ui->act_settings->setText("Preferences");
        ui->act_settings->setIconText("Preferences");
    }else{
        ui->jog_menu->setParent(ui->placeholder_jog);
        ui->position_label->setParent(ui->info_placeholder);
        ui->stackedWidget->setCurrentIndex(1);
        ui->act_settings->setText("Commands");
        ui->act_settings->setIconText("Commands");
    }
}

void MainWindow::on_sendTask_clicked(){
    not_idle_warning();

    int xsteps=0,ysteps=0,xvel=0,yvel=0;
    qDebug() << ui->x_finish->value() <<'\n' << _xpos;
    xsteps = ui->x_finish->value() - _xpos;
    ysteps = ui->y_finish->value() - _ypos;
    if(xsteps!=0 || ysteps!=0) xvel = ui->speed_spin->value() * (xsteps) / sqrt(pow(xsteps,2) + pow(ysteps,2));
    if(xsteps!=0 || ysteps!=0) yvel = ui->speed_spin->value() * (ysteps) / sqrt(pow(xsteps,2) + pow(ysteps,2));
    if(xvel==0) xvel=100;
    if(yvel==0) yvel=100;
    xsteps = abs(xsteps);
    ysteps = abs(ysteps);
    _current_speed_x = xvel;
    _current_speed_y = yvel;
    send_move_command(xsteps, ysteps, xvel, yvel);
    timer->start();
}

void MainWindow::on_ledButton_clicked()
{
    QNetworkAccessManager * mgr = new QNetworkAccessManager(this);
    connect(mgr,SIGNAL(finished(QNetworkReply*)),this,SLOT(onfinish(QNetworkReply*)));
    connect(mgr,SIGNAL(finished(QNetworkReply*)),mgr,SLOT(deleteLater()));

    mgr->post(QNetworkRequest(QUrl("http://192.168.4.1/LED")),"");
}

void MainWindow::on_xplus_clicked()
{
    QNetworkAccessManager * mgr = new QNetworkAccessManager(this);
    connect(mgr,SIGNAL(finished(QNetworkReply*)),this,SLOT(onfinish(QNetworkReply*)));
    connect(mgr,SIGNAL(finished(QNetworkReply*)),mgr,SLOT(deleteLater()));

    mgr->post(QNetworkRequest(QUrl("http://192.168.4.1/JOGX+")),"");
    timer->start();
}

void MainWindow::on_xminus_clicked()
{
    QNetworkAccessManager * mgr = new QNetworkAccessManager(this);
    connect(mgr,SIGNAL(finished(QNetworkReply*)),this,SLOT(onfinish(QNetworkReply*)));
    connect(mgr,SIGNAL(finished(QNetworkReply*)),mgr,SLOT(deleteLater()));

    mgr->post(QNetworkRequest(QUrl("http://192.168.4.1/JOGX-")),"");
    timer->start();
}

void MainWindow::on_yplus_clicked()
{
    QNetworkAccessManager * mgr = new QNetworkAccessManager(this);
    connect(mgr,SIGNAL(finished(QNetworkReply*)),this,SLOT(onfinish(QNetworkReply*)));
    connect(mgr,SIGNAL(finished(QNetworkReply*)),mgr,SLOT(deleteLater()));

    mgr->post(QNetworkRequest(QUrl("http://192.168.4.1/JOGY+")),"");
    timer->start();
}

void MainWindow::on_yminus_clicked()
{
    QNetworkAccessManager * mgr = new QNetworkAccessManager(this);
    connect(mgr,SIGNAL(finished(QNetworkReply*)),this,SLOT(onfinish(QNetworkReply*)));
    connect(mgr,SIGNAL(finished(QNetworkReply*)),mgr,SLOT(deleteLater()));

    mgr->post(QNetworkRequest(QUrl("http://192.168.4.1/JOGY-")),"");
    timer->start();
}

void MainWindow::on_steps_mm_valueChanged(int arg1)
{
    _steps_per_mm = arg1;
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
    not_idle_warning();
    int x = abs(ui->movex->value())*_steps_per_mm;
    _current_speed_x = (ui->movex->value()>=0) ? (_xvel_default): (-_xvel_default);
    qDebug() << x <<'\n' << _current_speed_x;
    send_move_command(x, 0, _current_speed_x , _yvel_default);
    timer->start();
}

void MainWindow::on_movey_button_clicked()
{
    not_idle_warning();
    int y = abs(ui->movey->value())*_steps_per_mm;
    _current_speed_y = (ui->movey->value()>=0) ? (_yvel_default): (-_yvel_default);
    send_move_command(0, y, _xvel_default, _current_speed_y);
    timer->start();
}

void MainWindow::on_save_settings_clicked()
{
    QMessageBox msgBox;
    int ret = QMessageBox::No;
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setText("Existing preferences file will be overwritten.\nContinue?");
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
}
