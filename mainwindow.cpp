#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QSerialPortInfo>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , serial(new QSerialPort(this))
    , plot1(new QCustomPlot(this))
    , plot2(new QCustomPlot(this))
    , time(0)
    , sampleCounter(0)
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

    // Configurar layout para dos gráficos
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(plot1);
    layout->addWidget(plot2);
    QWidget *container = new QWidget;
    container->setLayout(layout);
    setCentralWidget(container);

    // Configurar gráfico 1 (Canal 1)
    plot1->addGraph();
    plot1->graph(0)->setPen(QPen(Qt::blue));
    plot1->xAxis->setLabel("Tiempo (s)");
    plot1->yAxis->setLabel("Canal 1");
    plot1->xAxis->setRange(0, 10);
    plot1->yAxis->setRange(-25000, 25000);
    plot1->setInteraction(QCP::iRangeDrag, true);
    plot1->setInteraction(QCP::iRangeZoom, true);
    plot1->setOpenGl(true);

    // Configurar gráfico 2 (Canal 2)
    plot2->addGraph();
    plot2->graph(0)->setPen(QPen(Qt::red));
    plot2->xAxis->setLabel("Tiempo (s)");
    plot2->yAxis->setLabel("Canal 2");
    plot2->xAxis->setRange(0, 10);
    plot2->yAxis->setRange(-1000, 1000); // Ajusta según valores de canal2
    plot2->setInteraction(QCP::iRangeDrag, true);
    plot2->setInteraction(QCP::iRangeZoom, true);
    plot2->setOpenGl(true);

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

            plot1->graph(0)->setData(xData, yData1);
            plot2->graph(0)->setData(xData, yData2);
            plot1->xAxis->setRange(time, 10, Qt::AlignRight);
            plot2->xAxis->setRange(time, 10, Qt::AlignRight);

            if (lastReplot.elapsed() >= 50) {
                plot1->replot(QCustomPlot::rpQueuedReplot);
                plot2->replot(QCustomPlot::rpQueuedReplot);
                lastReplot.restart();
            }
        }

        time += 0.001;
    }

    if (buffer.size() > 512) {
        buffer.clear();
    }
}
