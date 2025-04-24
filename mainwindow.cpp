#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSettings>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QTextStream>
#include <QDebug>
#include <QFormLayout>
#include <QDialog>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>

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
    , file(new QTemporaryFile("tempXXXXXX.csv", this))
    , invertCanal1(false)
    , invertCanal2(false)
    , autorangeCanal1(true)
    , autorangeCanal2(true)
    , manualMinCanal1(-25000)
    , manualMaxCanal1(25000)
    , manualMinCanal2(-1000)
    , manualMaxCanal2(1000)
    , headerTime("Tiempo")
    , headerCanal1("Canal1")
    , headerCanal2("Canal2")
{
    ui->setupUi(this);

    statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
    statusBar->showMessage("Desconectado | Stopped | Not Recording");

    QSettings settings("MyApp", "SerialPlotter");
    QString lastPort = settings.value("lastPort", "COM9").toString();
    int lastBaud = settings.value("lastBaud", 230400).toInt();
    serial->setPortName(lastPort);
    serial->setBaudRate(lastBaud);

    connect(serial, &QSerialPort::readyRead, this, &MainWindow::readData);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    QPushButton *connectButton = new QPushButton(this);
    QPushButton *playStopButton = new QPushButton(this);
    QPushButton *recordButton = new QPushButton(this);
    QPushButton *configButton = new QPushButton(this);

    // Agregar iconos SVG directamente con QIcon
    QSize iconSize(32, 32);
    connectButton->setIcon(QIcon(":/icons/link-solid.svg"));
    playStopButton->setIcon(QIcon(":/icons/play-solid.svg"));
    recordButton->setIcon(QIcon(":/icons/floppy-disk-solid.svg"));
    configButton->setIcon(QIcon(":/icons/gears-solid.svg"));

    // Aplicar estilos CSS a los botones: fondo transparente, iconos blancos, texto oculto por defecto
    QString buttonStyle = R"(
        QPushButton {
            background-color: transparent; /* Sin fondo */
            border: none; /* Sin bordes */
            padding: 5px;
            min-width: 40px;
            min-height: 40px;
        }
        QPushButton:hover {
            background-color: rgba(200, 200, 200, 50); /* Fondo gris claro al pasar el mouse */
        }
        QPushButton:pressed {
            background-color: rgba(150, 150, 150, 100); /* Fondo gris un poco más oscuro al presionar */
        }
        QPushButton::text {
            color: transparent; /* Ocultar el texto por defecto */
        }
        QPushButton:hover::text {
            color: white; /* Mostrar el texto al pasar el mouse */
        }
    )";
    connectButton->setStyleSheet(buttonStyle);
    playStopButton->setStyleSheet(buttonStyle);
    recordButton->setStyleSheet(buttonStyle);
    configButton->setStyleSheet(buttonStyle);

    // Mostrar el icono en el botón y asignar el texto (que se mostrará en hover)
    connectButton->setIconSize(iconSize);
    playStopButton->setIconSize(iconSize);
    recordButton->setIconSize(iconSize);
    configButton->setIconSize(iconSize);
/*
    connectButton->setText("Connect");
    playStopButton->setText("Play/Stop");
    recordButton->setText("Record");
    configButton->setText("Config");*/

    buttonLayout->addWidget(connectButton);
    buttonLayout->addWidget(playStopButton);
    buttonLayout->addWidget(recordButton);
    buttonLayout->addWidget(configButton);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(plot1);
    mainLayout->addWidget(plot2);
    QWidget *container = new QWidget;
    container->setLayout(mainLayout);
    setCentralWidget(container);

    connect(connectButton, &QPushButton::clicked, this, [this, connectButton]() { onConnectClicked(connectButton); });
    connect(playStopButton, &QPushButton::clicked, this, [this, playStopButton]() { onPlayStopClicked(playStopButton); });
    connect(recordButton, &QPushButton::clicked, this, &MainWindow::onRecordClicked);
    connect(configButton, &QPushButton::clicked, this, &MainWindow::onConfigClicked);

    plot1->addGraph();
    plot1->graph(0)->setPen(QPen(Qt::blue));
    plot1->xAxis->setLabel("Tiempo (s)");
    plot1->yAxis->setLabel("Canal 1 (ECG)");
    plot1->xAxis->setRange(0, 10);
    plot1->yAxis->setRange(manualMinCanal1, manualMaxCanal1);
    plot1->setInteraction(QCP::iRangeDrag, true);
    plot1->setInteraction(QCP::iRangeZoom, true);
    plot1->setOpenGl(false);
    plot1->setBackground(QBrush(QColor(255, 220, 220)));

    plot2->addGraph();
    plot2->graph(0)->setPen(QPen(Qt::red));
    plot2->xAxis->setLabel("Tiempo (s)");
    plot2->yAxis->setLabel("Canal 2 (PPG)");
    plot2->xAxis->setRange(0, 10);
    plot2->yAxis->setRange(manualMinCanal2, manualMaxCanal2);
    plot2->setInteraction(QCP::iRangeDrag, true);
    plot2->setInteraction(QCP::iRangeZoom, true);
    plot2->setOpenGl(false);
    plot2->setBackground(QBrush(QColor(255, 220, 220)));

    lastReplot.start();
}

MainWindow::~MainWindow()
{
    delete ui;
    if (serial->isOpen()) {
        serial->close();
    }
    delete serial;
}

void MainWindow::onConnectClicked(QPushButton *connectButton)
{
    QSize iconSize(32, 32);
    if (serial->isOpen()) {
        serial->close();
        connectButton->setIcon(QIcon(":/icons/link-solid.svg"));
    } else {
        if (serial->open(QIODevice::ReadOnly)) {
            connectButton->setIcon(QIcon(":/icons/link-slash-solid.svg"));
        } else {
            statusBar->showMessage("Error al conectar: " + serial->errorString());
            return;
        }
    }
    connectButton->setIconSize(iconSize);
    statusBar->showMessage(QString("%1 | %2 | %3")
                               .arg(serial->isOpen() ? "Conectado" : "Desconectado")
                               .arg(isPlaying ? "Playing" : "Stopped")
                               .arg(isRecording ? "Recording" : "Not Recording"));
}

void MainWindow::onPlayStopClicked(QPushButton *playStopButton)
{
    isPlaying = !isPlaying;
    QSize iconSize(32, 32);
    if (isPlaying) {
        playStopButton->setIcon(QIcon(":/icons/stop-solid.svg"));
    } else {
        playStopButton->setIcon(QIcon(":/icons/play-solid.svg"));
    }
    playStopButton->setIconSize(iconSize);
    statusBar->showMessage(QString("%1 | %2 | %3")
                               .arg(serial->isOpen() ? "Conectado" : "Desconectado")
                               .arg(isPlaying ? "Playing" : "Stopped")
                               .arg(isRecording ? "Recording" : "Not Recording"));
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

    // Nuevos campos para las cabeceras
    QLineEdit *headerTimeEdit = new QLineEdit(headerTime);
    QLineEdit *headerCanal1Edit = new QLineEdit(headerCanal1);
    QLineEdit *headerCanal2Edit = new QLineEdit(headerCanal2);
    layout->addRow("Cabecera Tiempo:", headerTimeEdit);
    layout->addRow("Cabecera ECG:", headerCanal1Edit);
    layout->addRow("Cabecera PPG:", headerCanal2Edit);

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

        // Si el puerto está abierto, ciérralo para aplicar los nuevos ajustes
        if (serial->isOpen()) {
            serial->close();
        }
        serial->setPortName(portCombo->currentText());
        serial->setBaudRate(baudCombo->currentText().toInt());
        // Reabre el puerto si estaba conectado
        if (statusBar->currentMessage().contains("Conectado")) {
            if (!serial->open(QIODevice::ReadOnly)) {
                statusBar->showMessage("Error al reconectar: " + serial->errorString());
            }
        }

        invertCanal1 = invertCanal1Check->isChecked();
        invertCanal2 = invertCanal2Check->isChecked();
        autorangeCanal1 = autorangeCanal1Check->isChecked();
        autorangeCanal2 = autorangeCanal2Check->isChecked();

        // Validar y actualizar los rangos manuales
        bool ok;
        double newMinCanal1 = minCanal1Edit->text().toDouble(&ok);
        if (ok) manualMinCanal1 = newMinCanal1;

        double newMaxCanal1 = maxCanal1Edit->text().toDouble(&ok);
        if (ok) manualMaxCanal1 = newMaxCanal1;

        double newMinCanal2 = minCanal2Edit->text().toDouble(&ok);
        if (ok) manualMinCanal2 = newMinCanal2;

        double newMaxCanal2 = maxCanal2Edit->text().toDouble(&ok);
        if (ok) manualMaxCanal2 = newMaxCanal2;

        // Asegurarse de que el rango sea válido (mínimo < máximo)
        if (manualMinCanal1 >= manualMaxCanal1) {
            manualMinCanal1 = manualMaxCanal1 - 1;
        }
        if (manualMinCanal2 >= manualMaxCanal2) {
            manualMinCanal2 = manualMaxCanal2 - 1;
        }

        if (!autorangeCanal1) {
            plot1->yAxis->setRange(manualMinCanal1, manualMaxCanal1);
        }
        if (!autorangeCanal2) {
            plot2->yAxis->setRange(manualMinCanal2, manualMaxCanal2);
        }

        // Actualizar las cabeceras
        headerTime = headerTimeEdit->text().isEmpty() ? "Tiempo" : headerTimeEdit->text();
        headerCanal1 = headerCanal1Edit->text().isEmpty() ? "Canal1" : headerCanal1Edit->text();
        headerCanal2 = headerCanal2Edit->text().isEmpty() ? "Canal2" : headerCanal2Edit->text();
    }
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

        //qDebug() << "Canal1:" << canal1 << "Canal2:" << canal2;

        allXData.append(time);
        allYData1.append(canal1);
        allYData2.append(canal2);

        sampleCounter++;
        if (sampleCounter % 10 == 0) {
            xData.append(time);
            yData1.append(canal1);
            yData2.append(canal2);

            const int maxPoints = 10000;
            if (xData.size() > maxPoints) {
                xData.remove(0, xData.size() - maxPoints);
                yData1.remove(0, yData1.size() - maxPoints);
                yData2.remove(0, yData2.size() - maxPoints);
            }

            QVector<double> shiftedXData;
            double windowSize = 10.0;
            double latestTime = xData.last();
            for (int i = 0; i < xData.size(); ++i) {
                double shiftedX = xData[i] - (latestTime - windowSize);
                while (shiftedX < 0) shiftedX += windowSize;
                while (shiftedX > windowSize) shiftedX -= windowSize;
                shiftedXData.append(shiftedX);
            }

            plot1->graph(0)->setData(shiftedXData, yData1, true);
            plot2->graph(0)->setData(shiftedXData, yData2, true);

            plot1->xAxis->setRange(0, windowSize);
            plot2->xAxis->setRange(0, windowSize);

            if (autorangeCanal1) updateAutorange(plot1, yData1);
            if (autorangeCanal2) updateAutorange(plot2, yData2);

            if (lastReplot.elapsed() >= 50) {
                plot1->replot();
                plot2->replot();
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

void MainWindow::onRecordClicked()
{
    if (isRecording) {
        isRecording = false;
        if (file->isOpen()) {
            file->close();
        }
        QString savePath = QFileDialog::getSaveFileName(this, "Guardar datos", "", "CSV Files (*.csv)");
        if (!savePath.isEmpty()) {
            QFile::copy(file->fileName(), savePath);
        }
    } else {
        if (!file->open(QIODevice::WriteOnly | QIODevice::Text)) {
            statusBar->showMessage("Error al abrir archivo temporal");
            return;
        }
        isRecording = true;
        QTextStream stream(file);
        stream << headerTime << "," << headerCanal1 << "," << headerCanal2 << "\n";
    }
    statusBar->showMessage(QString("%1 | %2 | %3")
                               .arg(serial->isOpen() ? "Conectado" : "Desconectado")
                               .arg(isPlaying ? "Playing" : "Stopped")
                               .arg(isRecording ? "Recording" : "Not Recording"));
}

void MainWindow::updateAutorange(QCustomPlot *plot, const QVector<double> &data)
{
    if (data.isEmpty()) return;
    double min = *std::min_element(data.constBegin(), data.constEnd());
    double max = *std::max_element(data.constBegin(), data.constEnd());
    double range = max - min;
    plot->yAxis->setRange(min - range * 0.1, max + range * 0.1);
}
