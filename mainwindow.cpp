#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    myTimer = new QTimer(this);
    mySerial = new QSerialPort(this);
    mySettings = new SettingsDialog();
    myPaintBox1 = new QPaintBox(0,0,ui->widget);
    estado = new QLabel;
    estado->setText("Desconectado............");
    ui->statusbar->addWidget(estado);
    ui->actionDesconectar->setEnabled(false);
    estadoProtocolo=START;
    ui->pushButtonEnviar->setEnabled(false);
    estadoComandos=ALIVE;


    ///Conexión de eventos
    connect(mySerial,&QSerialPort::readyRead,this, &MainWindow::dataRecived );
    connect(myTimer, &QTimer::timeout,this, &MainWindow::myTimerOnTime);
    connect(ui->actionEscaneo_de_Puertos, &QAction::triggered, mySettings, &SettingsDialog::show);
    connect(ui->actionConectar,&QAction::triggered,this, &MainWindow::openSerialPort);
    connect(ui->actionConectar,&QAction::triggered,this, &MainWindow::myPaint);
    connect(ui->actionDesconectar, &QAction::triggered, this, &MainWindow::closeSerialPort);
    connect(ui->actionSalir,&QAction::triggered,this,&MainWindow::close );

    myTimer->start(10);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::openSerialPort()
{
    SettingsDialog::Settings p = mySettings->settings();
    mySerial->setPortName(p.name);
    mySerial->setBaudRate(p.baudRate);
    mySerial->setDataBits(p.dataBits);
    mySerial->setParity(p.parity);
    mySerial->setStopBits(p.stopBits);
    mySerial->setFlowControl(p.flowControl);
    mySerial->open(QSerialPort::ReadWrite);
    if(mySerial->isOpen()){
        ui->actionConectar->setEnabled(false);
        ui->actionDesconectar->setEnabled(true);
        ui->pushButtonEnviar->setEnabled(true);
        estado->setText(tr("Conectado a  %1 : %2, %3, %4, %5, %6  %7")
                                         .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
                                         .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl).arg(p.fabricante));
    }
    else{
        QMessageBox::warning(this,"Menu Conectar","No se pudo abrir el puerto Serie!!!!");
    }


}

void MainWindow::closeSerialPort()
{
    if(mySerial->isOpen()){
        mySerial->close();
        ui->actionDesconectar->setEnabled(false);
        ui->actionConectar->setEnabled(true);
        estado->setText("Desconectado................");
        ui->pushButtonEnviar->setEnabled(false);
    }
    else{
         estado->setText("Desconectado................");
         ui->pushButtonEnviar->setEnabled(false);
    }

}

void MainWindow::myTimerOnTime()
{

    if(rxData.timeOut!=0){
        rxData.timeOut--;
    }else{
        estadoProtocolo=START;
    }
}

void MainWindow::dataRecived()
{
    unsigned char *incomingBuffer;
    int count;

    count = mySerial->bytesAvailable();

    if(count<=0)
        return;

    incomingBuffer = new unsigned char[count];

    mySerial->read((char *)incomingBuffer,count);

    QString str="";

    for(int i=0; i<=count; i++){
        if(isalnum(incomingBuffer[i]))
            str = str + QString("%1").arg((char)incomingBuffer[i]);
        else
            str = str +"{" + QString("%1").arg(incomingBuffer[i],2,16,QChar('0')) + "}";
    }
    ui->textBrowser->append("MBED-->PC (" + str + ")");
    rxData.timeOut=5;
    for(int i=0;i<count; i++){
        switch (estadoProtocolo) {
            case START:
                if (incomingBuffer[i]=='U'){
                    estadoProtocolo=HEADER_1;
                    rxData.cheksum=0;
                }
                break;
            case HEADER_1:
                if (incomingBuffer[i]=='N')
                   estadoProtocolo=HEADER_2;
                else{
                    i--;
                    estadoProtocolo=START;
                }
                break;
            case HEADER_2:
                if (incomingBuffer[i]=='E')
                    estadoProtocolo=HEADER_3;
                else{
                    i--;
                   estadoProtocolo=START;
                }
                break;
        case HEADER_3:
            if (incomingBuffer[i]=='R')
                estadoProtocolo=NBYTES;
            else{
                i--;
               estadoProtocolo=START;
            }
            break;
            case NBYTES:
                rxData.nBytes=incomingBuffer[i];
               estadoProtocolo=TOKEN;
                break;
            case TOKEN:
                if (incomingBuffer[i]==':'){
                   estadoProtocolo=PAYLOAD;
                    rxData.cheksum='U'^'N'^'E'^'R'^ rxData.nBytes^':';
                    rxData.payLoad[0]=rxData.nBytes;
                    rxData.index=1;
                }
                else{
                    i--;
                    estadoProtocolo=START;
                }
                break;
            case PAYLOAD:
                if (rxData.nBytes>1){
                    rxData.payLoad[rxData.index++]=incomingBuffer[i];
                    rxData.cheksum^=incomingBuffer[i];
                }
                rxData.nBytes--;
                if(rxData.nBytes==0){
                    estadoProtocolo=START;
                    if(rxData.cheksum==incomingBuffer[i]){
                        decodeData();
                    }
                }
                break;
            default:
                estadoProtocolo=START;
                break;
        }
    }
    delete [] incomingBuffer;
}

void MainWindow::decodeData()
{
    QString str="",s,stateNum;
    for(int a=1; a < rxData.index; a++){
        switch (rxData.payLoad[1]) {
            case ALIVE:
                str = "MBED-->PC *ID Válido* (I´M ALIVE)";
                break;
            case GET_LEDS:
                str = "MBED-->PC *ID Válido* GET LEDS";
                myWord.ui8[0]=rxData.payLoad[2];
                myWord.ui8[1]=rxData.payLoad[3];
                leds=myWord.i16[0];
                myPaint();

                break;
            case SET_LEDS:
                str = "MBED-->PC SET LEDS";
                break;
            case BUTTON_EVENT:
                 str = "MBED-->PC BUTTON_EVENT RECIVED ";
                 indiceButtons=rxData.payLoad[2];
                 s.setNum(indiceButtons);
                 ourButton[indiceButtons].estado=rxData.payLoad[3];
                 stateNum.setNum(ourButton[indiceButtons].estado);
                 str= ("MBED-->PC NUMBUTTON " + s + "estado: " + stateNum +"");
                 myWord.ui8[0]=rxData.payLoad[4];
                 myWord.ui8[1]=rxData.payLoad[5];
                 myWord.ui8[2]=rxData.payLoad[6];
                 myWord.ui8[3]=rxData.payLoad[7];
                 myPaint();

            break;
            case BUTTON_STATE:
                str = "MBED-->PC *ID Válido* BUTTON STATE";
                myWord.ui8[0]=rxData.payLoad[2];
                myWord.ui8[1]=rxData.payLoad[3];
                buttons=myWord.i16[0];
                myPaint();
            break;
            default:
                str=((char *)rxData.payLoad);
                str= ("MBED-->PC *ID Invalido * (" + str + ")");
                break;
        }
    }
    ui->textBrowser->append(str);

}


void MainWindow::on_pushButtonEnviar_clicked()
{
    uint16_t auxleds=0;
    txData.index=0;
    txData.payLoad[txData.index++]='U';
    txData.payLoad[txData.index++]='N';
    txData.payLoad[txData.index++]='E';
    txData.payLoad[txData.index++]='R';
    txData.payLoad[txData.index++]=0;
    txData.payLoad[txData.index++]=':';

    switch (estadoComandos) {
    case ALIVE:
        txData.payLoad[txData.index++]=ALIVE;
        txData.payLoad[NBYTES]=0x02;
        break;
    case GET_LEDS:
        txData.payLoad[txData.index++]=GET_LEDS;
        txData.payLoad[NBYTES]=0x02;
        break;
    case SET_LEDS:
        txData.payLoad[txData.index++]=SET_LEDS;
        myWord.ui16[0]=auxleds;
        txData.payLoad[txData.index++]=myWord.ui8[0];
        txData.payLoad[txData.index++]=myWord.ui8[1];
        txData.payLoad[NBYTES]=0x04;

    case BUTTON_STATE :
        txData.payLoad[txData.index++]=BUTTON_STATE;
        txData.payLoad[NBYTES]=0x02;
    case OTHERS:
        break;
    default:
        break;
    }
   txData.cheksum=0;
   for(int a=0 ;a<txData.index;a++)
       txData.cheksum^=txData.payLoad[a];
    txData.payLoad[txData.index]=txData.cheksum;
    if(mySerial->isWritable()){
        mySerial->write((char *)txData.payLoad,txData.payLoad[NBYTES]+6);
    }

    QString str="";
    for(int i=0; i<=txData.index; i++){
        if(isalnum(txData.payLoad[i]))
            str = str + QString("%1").arg((char)txData.payLoad[i]);
        else
            str = str +"{" + QString("%1").arg(txData.payLoad[i],2,16,QChar('0')) + "}";
    }
    ui->textBrowser->append("PC-->MBED (" + str + ")");

}

//void MainWindow::buttons(uint32_t timer){
//    ui->textBrowser->
//}

void MainWindow::on_comboBox_currentIndexChanged(int index)
{
    switch (index) {
        case 1:
            estadoComandos=ALIVE;
        break;
        case 2:
            estadoComandos=GET_LEDS;
        break;
        case 3:
            estadoComandos=SET_LEDS;
        break;
        case 4:
            estadoComandos=BUTTON_STATE;
        break;
        case 5:
            estadoComandos=OTHERS;
        break;
    default:
        ;
    }
}
void MainWindow:: getLedsOn(uint16_t leds){

}
void MainWindow:: myPaint(){
    QString strvaly;
    uint8_t ancho=60, alto=60, ladoSquare=80;
    int16_t posx,posy, divide, auxButton=0;
    QPen Pen;
    QPainter paint(myPaintBox1->getCanvas());
    myPaintBox1->getCanvas()->fill(Qt::lightGray);
    paint.setPen(Pen);
    Pen.setColor(Qt::black);
    divide=myPaintBox1->width()/4;
    posx=(divide/2)-(ancho/2);
    posy=(myPaintBox1->height()/4);

    Pen.setWidth(6);
    paint.setRenderHint(QPainter::Antialiasing);
    paint.setFont(QFont("Arial",13,QFont::Bold));
    paint.drawText(posx,posy-10,"Led 1");
    posy/4;
    paint.drawEllipse(posx,posy,ancho,alto);
    posx+=divide;
    paint.drawText(posx,posy-10,"Led 2");
    paint.drawEllipse(posx,posy,ancho,alto);
    posx+=divide;
    paint.drawText(posx,posy-10,"Led 3");
    paint.drawEllipse(posx,posy,ancho,alto);
    posx+=divide;
    paint.drawText(posx,posy-10,"Led 4");
    paint.drawEllipse(posx,posy,ancho,alto);
    posx=(divide/2)-(ancho/2);
    posy=posy+120;
    auxButton=0;
    paint.setBrush((Qt::gray));
    if(auxButton==indiceButtons && (ourButton[indiceButtons].estado)==0){
        paint.setBrush((Qt::darkGray));
    }
    paint.drawRect(posx-10,posy,ladoSquare,ladoSquare);
    paint.setBrush((Qt::darkGray));
    paint.drawEllipse(posx,posy+10,ancho,alto);
    posx+=divide;

    auxButton++;
    paint.setBrush((Qt::gray));
    if(auxButton==indiceButtons && (ourButton[indiceButtons].estado)==0){
        paint.setBrush((Qt::darkGray));
    }
    paint.drawRect(posx-10,posy,ladoSquare,ladoSquare);
     paint.setBrush((Qt::darkGray));
    paint.drawEllipse(posx,posy+10,ancho,alto);
    posx+=divide;

    auxButton++;
    paint.setBrush((Qt::gray));
    if(auxButton==indiceButtons && (ourButton[indiceButtons].estado)==0){
        paint.setBrush((Qt::darkGray));
    }
    paint.drawRect(posx-10,posy,ladoSquare,ladoSquare);
    paint.setBrush((Qt::darkGray));
    paint.drawEllipse(posx,posy+10,ancho,alto);
    posx+=divide;

    auxButton++;
    paint.setBrush((Qt::gray));
    if(auxButton==indiceButtons && (ourButton[indiceButtons].estado)==0){
        paint.setBrush((Qt::darkGray));
    }
    paint.drawRect(posx-10,posy,ladoSquare,ladoSquare);
    paint.setBrush((Qt::darkGray));
    paint.drawEllipse(posx,posy+10,ancho,alto);

    myPaintBox1->update();
}


void MainWindow::on_pushButton_clicked()
{
    myPaint();
    SettingsDialog::Settings p = mySettings->settings();
    mySerial->setPortName(p.name);
    mySerial->setBaudRate(p.baudRate);
    mySerial->setDataBits(p.dataBits);
    mySerial->setParity(p.parity);
    mySerial->setStopBits(p.stopBits);
    mySerial->setFlowControl(p.flowControl);
    mySerial->open(QSerialPort::ReadWrite);
    if(mySerial->isOpen()){
        ui->actionConectar->setEnabled(false);
        ui->actionDesconectar->setEnabled(true);
        ui->pushButtonEnviar->setEnabled(true);
        estado->setText(tr("Conectado a  %1 : %2, %3, %4, %5, %6  %7")
                                         .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
                                         .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl).arg(p.fabricante));
    }
    else{
        QMessageBox::warning(this,"Menu Conectar","No se pudo abrir el puerto Serie!!!!");
    }
}

