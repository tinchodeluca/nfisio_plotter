#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QSerialPortInfo>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , serial(new QSerialPort(this))
    , plot(new QCustomPlot(this))
    , time(0)
{
    ui->setupUi(this);

    qDebug() << "Puertos disponibles:";
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        qDebug() << "Puerto:" << info.portName();
    }

    serial->setPortName("COM9");
    serial->setBaudRate(230400);
    if (serial->open(QIODevice::ReadOnly)) {
        qDebug() << "Puerto COM9 abierto correctamente";
    } else {
        qDebug() << "Error al abrir COM9:" << serial->errorString();
    }
    connect(serial, &QSerialPort::readyRead, this, &MainWindow::readData);

    setCentralWidget(plot);
    plot->addGraph();
    plot->addGraph();
    plot->graph(0)->setPen(QPen(Qt::blue));
    plot->graph(1)->setPen(QPen(Qt::red));
    plot->xAxis->setLabel("Tiempo (s)");
    plot->yAxis->setLabel("Valor");
    plot->xAxis->setRange(0, 10);
    plot->yAxis->setRange(-25000, 25000);
    plot->setInteraction(QCP::iRangeDrag, true);
    plot->setInteraction(QCP::iRangeZoom, true);
    plot->setOpenGl(true);

    lastReplot.start();
}

MainWindow::~MainWindow()
{
    serial->close();
    delete ui;
}

void MainWindow::readData()
{
    buffer.append(serial->readAll());
    const QByteArray header = QByteArray::fromHex("EFBEADDE");
    static int sampleCounter = 0;

    while (buffer.size() >= 12) {
        int headerPos = buffer.indexOf(header);
        if (headerPos == -1) {
            buffer.clear();
            return;
        }
        if (headerPos > 0) {
            buffer.remove(0, headerPos);
        }
        if (buffer.size() < 12) {
            return;
        }

        qint32 canal1 = *(qint32*)(buffer.constData() + 4);
        qint32 canal2 = *(qint32*)(buffer.constData() + 8);
        buffer.remove(0, 12);

        qDebug() << "Canal1:" << canal1 << "Canal2:" << canal2;

        sampleCounter++;
        if (sampleCounter % 10 == 0) {
            xData.append(time);
            yData1.append(canal1);
            yData2.append(canal2);

            const int maxPoints = 1000;
            if (xData.size() > maxPoints) {
                xData.remove(0, xData.size() - maxPoints);
                yData1.remove(0, yData1.size() - maxPoints);
                yData2.remove(0, yData2.size() - maxPoints);
            }

            plot->graph(0)->setData(xData, yData1);
            plot->graph(1)->setData(xData, yData2);
            plot->xAxis->setRange(time, 10, Qt::AlignRight);

            if (lastReplot.elapsed() >= 50) {
                plot->replot(QCustomPlot::rpQueuedReplot);
                lastReplot.restart();
            }
        }

        time += 0.001;
    }

    // Reducir acumulación en el buffer
    if (buffer.size() > 512) {
        buffer.clear();
    }
}
