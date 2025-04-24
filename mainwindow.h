#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include "qcustomplot.h"  // Usa comillas, el INCLUDEPATH se encarga del resto
#include <QElapsedTimer>
#include <QFile>
#include <QStatusBar>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void readData();
    void onConnectClicked();
    void onPlayStopClicked();
    void onRecordClicked();
    void onConfigClicked();

private:
    Ui::MainWindow *ui;
    QSerialPort *serial;
    QCustomPlot *plot1;
    QCustomPlot *plot2;
    QStatusBar *statusBar;
    QByteArray buffer;
    QVector<double> xData, yData1, yData2;
    QVector<double> allXData, allYData1, allYData2;
    double time;
    int sampleCounter;
    bool isPlaying;
    bool isRecording;
    QFile *file;
    bool invertCanal1;
    bool invertCanal2;
    bool autorangeCanal1;
    bool autorangeCanal2;
    double manualMinCanal1;
    double manualMaxCanal1;
    double manualMinCanal2;
    double manualMaxCanal2;
    QElapsedTimer lastReplot;
    QString headerTime;
    QString headerCanal1;
    QString headerCanal2;

    void updateAutorange(QCustomPlot *plot, const QVector<double> &data);
};

#endif // MAINWINDOW_H
