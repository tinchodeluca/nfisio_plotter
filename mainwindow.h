#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QElapsedTimer>
#include <QFile>
#include "qcustomplot/qcustomplot.h"

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
    void readData();
    void onConnectClicked();
    void onPlayStopClicked();
    void onRecordClicked();
    void onConfigClicked();

private:
    void updateAutorange(QCustomPlot *plot, const QVector<double> &data);

    Ui::MainWindow *ui;
    QSerialPort *serial;
    QCustomPlot *plot1;
    QCustomPlot *plot2;
    QByteArray buffer;
    QVector<double> xData, yData1, yData2;
    QVector<double> allXData, allYData1, allYData2;
    double time;
    QElapsedTimer lastReplot;
    int sampleCounter;
    bool isPlaying;
    bool isRecording;
    QFile *file;
    bool invertCanal1;
    bool invertCanal2;
    bool autorangeCanal1;
    bool autorangeCanal2;
    double manualMinCanal1, manualMaxCanal1;
    double manualMinCanal2, manualMaxCanal2;
};

#endif // MAINWINDOW_H
