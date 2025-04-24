#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QElapsedTimer>
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

private:
    Ui::MainWindow *ui;
    QSerialPort *serial;
    QCustomPlot *plot;
    QByteArray buffer;
    QVector<double> xData, yData1, yData2;
    double time;
    QElapsedTimer lastReplot;
};

#endif // MAINWINDOW_H
