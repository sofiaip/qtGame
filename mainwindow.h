#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSerialPort/QSerialPort>
#include <QTimer>
#include <QLabel>
#include "settingsdialog.h"
#include "qpaintbox.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:

    void openSerialPort();

    void closeSerialPort();

    void myTimerOnTime();

    void dataRecived();

    void decodeData();

    void on_pushButtonEnviar_clicked();

    void on_comboBox_currentIndexChanged(int index);

    void myPaint();

    void getLedsOn(uint16_t leds);
 //   void buttons(uint32_t timer);

    //void on_pushButton_clicked();

    void on_pushButton_clicked();

private:
    Ui::MainWindow  *ui;
    QSerialPort *mySerial;
    QTimer *myTimer;
    QPaintBox *myPaintBox1;
    SettingsDialog *mySettings;
    QLabel *estado;
    typedef enum{
        START,
        HEADER_1,
        HEADER_2,
        HEADER_3,
        NBYTES,
        TOKEN,
        PAYLOAD
    }_eProtocolo;

    _eProtocolo estadoProtocolo;

    typedef enum{
        ALIVE=0xF0,
        BUTTON_EVENT=0XFA,
        GET_LEDS=0xFB,
        SET_LEDS=0xFC,
        BUTTON_STATE=0xFD,
        OTHERS
    }_eID;

    _eID estadoComandos;

    typedef enum{
        DOWN,
        BUTTON_UP,      //1
        BUTTON_FALLING, //2
        BUTTON_RISING,
    }_eButtonState;
    _eButtonState estadoButtons;


    typedef struct{
        uint8_t timeOut;
        uint8_t cheksum;
        uint8_t payLoad[50];
        uint8_t nBytes;
        uint8_t index;
    }_sDatos ;

    _sDatos rxData, txData;


    typedef struct{
        uint8_t estado;
       // _eButtonEvent event;
        int32_t timeDown;
        int32_t timeDiff;
    }_sTeclas;

    _sTeclas ourButton[4];



    typedef union {
        float f32;
        int i32;
        unsigned int ui32;
        unsigned short ui16[2];
        short i16[2];
        uint8_t ui8[4];
        char chr[4];
        unsigned char uchr[4];
    }_udat;

    _udat myWord;

    uint8_t indiceButtons;
    uint16_t leds;
    uint16_t buttons;

};
#endif // MAINWINDOW_H
