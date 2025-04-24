#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QSerialPortInfo>
#include <QDialog>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QFormLayout>
#include <QSettings>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , serial(new QSerialPort(this))
    , plot1(new QCustomPlot(this))
    , plot2(new QCustomPlot(this))
    , time(0)
    , sampleCounter(0)
    , isPlaying(false)
    , isRecording(false)
    , file(new QFile("datos.csv", this))
    , invertCanal1(false)
    , invertCanal2(false)
    , autorangeCanal1(true)
    , autorangeCanal2(true)
    , manualMinCanal1(-25000)
    , manualMaxCanal1(25000)
    , manualMinCanal2(-1000)
    , manualMaxCanal2(1000)
{
    ui->setupUi(this);

    QSettings settings("MyApp", "SerialPlotter");
    QString lastPort = settings.value("lastPort", "COM9").toString();
    int lastBaud = settings.value("lastBaud", 230400).toInt();

    serial->setPortName(lastPort);
    serial->setBaudRate(lastBaud);

    connect(serial, &QSerialPort::readyRead, this, &MainWindow::readData);

    // Botones
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    QPushButton *connectButton = new QPushButton("Connect", this);
    QPushButton *playStopButton = new QPushButton("Play/Stop", this);
    QPushButton *recordButton = new QPushButton("Record", this);
    QPushButton *configButton = new QPushButton("Config", this);
    buttonLayout->addWidget(connectButton);
    buttonLayout->addWidget(playStopButton);
    buttonLayout->addWidget(recordButton);
    buttonLayout->addWidget(configButton);

    // Layout principal
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(plot1);
    mainLayout->addWidget(plot2);
    QWidget *container = new QWidget;
    container->setLayout(mainLayout);
    setCentralWidget(container);

    // Conectar botones
    connect(connectButton, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(playStopButton, &QPushButton::clicked, this, &MainWindow::onPlayStopClicked);
    connect(recordButton, &QPushButton::clicked, this, &MainWindow::onRecordClicked);
    connect(configButton, &QPushButton::clicked, this, &MainWindow::onConfigClicked);

    // Configurar gráfico 1 (Canal 1)
    plot1->addGraph();
    plot1->graph(0)->setPen(QPen(Qt::blue));
    plot1->xAxis->setLabel("Tiempo (s)");
    plot1->yAxis->setLabel("Canal 1");
    plot1->xAxis->setRange(0, 10);
    plot1->yAxis->setRange(manualMinCanal1, manualMaxCanal1);
    plot1->setInteraction(QCP::iRangeDrag, true);
    plot1->setInteraction(QCP::iRangeZoom, true);
    plot1->setOpenGl(true);

    // Configurar gráfico 2 (Canal 2)
    plot2->addGraph();
    plot2->graph(0)->setPen(QPen(Qt::red));
    plot2->xAxis->setLabel("Tiempo (s)");
    plot2->yAxis->setLabel("Canal 2");
    plot2->xAxis->setRange(0, 10);
    plot2->yAxis->setRange(manualMinCanal2, manualMaxCanal2);
    plot2->setInteraction(QCP::iRangeDrag, true);
    plot2->setInteraction(QCP::iRangeZoom, true);
    plot2->setOpenGl(true);

    lastReplot.start();
}

MainWindow::~MainWindow()
{
    serial->close();
    if (file->isOpen()) {
        file->close();
    }
    delete file;
    delete ui;
}

void MainWindow::readData()
{
    if (!isPlaying) return;

    buffer.append(serial->readAll());
    //qDebug() << "Datos recibidos (hex):" << buffer.toHex();

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

        canal1 = invertCanal1 ? -canal1 : canal1;
        canal2 = invertCanal2 ? -canal2 : canal2;

       // qDebug() << "Canal1:" << canal1 << "Canal2:" << canal2;

        allXData.append(time);
        allYData1.append(canal1);
        allYData2.append(canal2);

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

            // Forzar la actualización de los datos
            plot1->graph(0)->setData(xData, yData1, true);
            plot2->graph(0)->setData(xData, yData2, true);
            plot1->xAxis->setRange(time - 10, time);
            plot2->xAxis->setRange(time - 10, time);

            if (autorangeCanal1) updateAutorange(plot1, yData1);
            if (autorangeCanal2) updateAutorange(plot2, yData2);

            if (lastReplot.elapsed() >= 50) {
                plot1->replot(); // Reploteo con prioridad
                plot2->replot(); // Reploteo con prioridad
                lastReplot.restart();
            }
        }

        if (isRecording && file->isOpen()) {
            QTextStream stream(file);
            stream << time << "," << canal1 << "," << canal2 << "\n";
            file->flush();
        }

        time += 0.001;
    }

    if (buffer.size() > 512) {
        buffer.clear();
    }
}

void MainWindow::updateAutorange(QCustomPlot *plot, const QVector<double> &data)
{
    if (data.isEmpty()) return;

    const int windowSize = 1000;
    double minVal = data.last();
    double maxVal = data.last();

    int start = qMax(0, data.size() - windowSize);
    for (int i = start; i < data.size(); ++i) {
        minVal = qMin(minVal, data[i]);
        maxVal = qMax(maxVal, data[i]);
    }

    double range = maxVal - minVal;
    double margin = range * 0.2;
    plot->yAxis->setRange(minVal - margin, maxVal + margin);
}

void MainWindow::onConnectClicked()
{
    if (serial->isOpen()) {
        serial->close();
        qDebug() << "Puerto cerrado";
    } else {
        if (serial->open(QIODevice::ReadOnly)) {
            qDebug() << "Puerto abierto correctamente";
        } else {
            qDebug() << "Error al abrir puerto:" << serial->errorString();
        }
    }
}

void MainWindow::onPlayStopClicked()
{
    isPlaying = !isPlaying;
    qDebug() << "Play/Stop:" << (isPlaying ? "Playing" : "Stopped");
    if (isPlaying) {
        lastReplot.start();
    }
}

void MainWindow::onRecordClicked()
{
    if (isRecording) {
        isRecording = false;
        if (file->isOpen()) {
            file->close();
        }
        qDebug() << "Grabación detenida";
    } else {
        isRecording = true;
        if (file->open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(file);
            stream << "Tiempo,Canal1,Canal2\n";
            qDebug() << "Grabación iniciada";
        }
    }
}

void MainWindow::onConfigClicked()
{
    QDialog configDialog(this);
    configDialog.setWindowTitle("Configuración");

    QFormLayout *layout = new QFormLayout;

    QComboBox *portCombo = new QComboBox;
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        portCombo->addItem(info.portName());
    }
    portCombo->setCurrentText(serial->portName());
    layout->addRow("Puerto:", portCombo);

    QComboBox *baudCombo = new QComboBox;
    baudCombo->addItems({"9600", "19200", "38400", "57600", "115200", "230400"});
    baudCombo->setCurrentText(QString::number(serial->baudRate()));
    layout->addRow("Baud Rate:", baudCombo);

    QCheckBox *invertCanal1Check = new QCheckBox("Invertir Canal 1");
    invertCanal1Check->setChecked(invertCanal1);
    layout->addRow(invertCanal1Check);

    QCheckBox *invertCanal2Check = new QCheckBox("Invertir Canal 2");
    invertCanal2Check->setChecked(invertCanal2);
    layout->addRow(invertCanal2Check);

    QCheckBox *autorangeCanal1Check = new QCheckBox("Autorange Canal 1");
    autorangeCanal1Check->setChecked(autorangeCanal1);
    layout->addRow(autorangeCanal1Check);

    QCheckBox *autorangeCanal2Check = new QCheckBox("Autorange Canal 2");
    autorangeCanal2Check->setChecked(autorangeCanal2);
    layout->addRow(autorangeCanal2Check);

    QLineEdit *minCanal1Edit = new QLineEdit(QString::number(manualMinCanal1));
    QLineEdit *maxCanal1Edit = new QLineEdit(QString::number(manualMaxCanal1));
    layout->addRow("Canal 1 Mín:", minCanal1Edit);
    layout->addRow("Canal 1 Máx:", maxCanal1Edit);

    QLineEdit *minCanal2Edit = new QLineEdit(QString::number(manualMinCanal2));
    QLineEdit *maxCanal2Edit = new QLineEdit(QString::number(manualMaxCanal2));
    layout->addRow("Canal 2 Mín:", minCanal2Edit);
    layout->addRow("Canal 2 Máx:", maxCanal2Edit);

    QPushButton *okButton = new QPushButton("Aceptar");
    QPushButton *cancelButton = new QPushButton("Cancelar");
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addRow(buttonLayout);

    configDialog.setLayout(layout);

    connect(okButton, &QPushButton::clicked, &configDialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &configDialog, &QDialog::reject);

    if (configDialog.exec() == QDialog::Accepted) {
        QSettings settings("MyApp", "SerialPlotter");
        settings.setValue("lastPort", portCombo->currentText());
        settings.setValue("lastBaud", baudCombo->currentText().toInt());

        serial->setPortName(portCombo->currentText());
        serial->setBaudRate(baudCombo->currentText().toInt());

        invertCanal1 = invertCanal1Check->isChecked();
        invertCanal2 = invertCanal2Check->isChecked();
        autorangeCanal1 = autorangeCanal1Check->isChecked();
        autorangeCanal2 = autorangeCanal2Check->isChecked();

        manualMinCanal1 = minCanal1Edit->text().toDouble();
        manualMaxCanal1 = maxCanal1Edit->text().toDouble();
        manualMinCanal2 = minCanal2Edit->text().toDouble();
        manualMaxCanal2 = maxCanal2Edit->text().toDouble();

        if (!autorangeCanal1) {
            plot1->yAxis->setRange(manualMinCanal1, manualMaxCanal1);
        }
        if (!autorangeCanal2) {
            plot2->yAxis->setRange(manualMinCanal2, manualMaxCanal2);
        }

        if (serial->isOpen()) {
            serial->close();
            if (serial->open(QIODevice::ReadOnly)) {
                qDebug() << "Puerto reabierto correctamente";
            }
        }
    }
}
