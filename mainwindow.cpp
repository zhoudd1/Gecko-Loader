#ifdef EFM32_LOADER_GUI

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "geckoloader.h"
#include "helpdialog.h"
#include <QDebug>
#include <QSerialPortInfo>
#include <QFileDialog>
#include <QSettings>
#include <QSerialPort>
#include <QMessageBox>
#include <QTextStream>
#include <QRegularExpression>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->lineASCII->hide();
    ui->textLog->setFont(QFont("Courier New", 9));

    loader = new GeckoLoader(this);
    serialPort = loader->serialPort();
    m_connected = false;

    readSettings();
    slotReloadSerialPorts();

    connect(loader, SIGNAL(output(QString)), this, SLOT(log(QString)));
    connect(serialPort, SIGNAL(readyRead()), this, SLOT(slotDataReady()));
    connect(ui->buttonHelp, SIGNAL(clicked()), this, SLOT(slotHelp()));
    connect(ui->buttonReload, SIGNAL(clicked()), this, SLOT(slotReloadSerialPorts()));
    connect(ui->buttonBrowse, SIGNAL(clicked()), this, SLOT(slotBrowse()));
    connect(ui->buttonUpload, SIGNAL(clicked()), this, SLOT(slotUpload()));
    connect(ui->buttonConnect, SIGNAL(clicked()), this, SLOT(slotConnect()));
    connect(ui->comboTransport, SIGNAL(currentIndexChanged(int)), this, SLOT(slotTransport()));

    connect(ui->lineASCII, SIGNAL(returnPressed()), this, SLOT(slotSendASCII()));

    updateInterface();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::readSettings()
{
    QSettings settings;

    ui->lineFile->setText(settings.value("lastBinaryFile").toString());
}

void MainWindow::log(const QString &text)
{
    QString output = text;
    if(output.contains(QRegularExpression("\\d+\\s+\\/\\s+\\d+")))
    {
        output = output.remove('[');
        output = output.remove(']');
        output = output.remove(' ');
        QStringList sl = output.split('/');
        int progress = (int)(sl[0].toFloat() / sl[1].toFloat() * 100.0);
        ui->progressBar->setValue(progress);
    }
    else
    {
        ui->textLog->appendPlainText(text);
    }
}

void MainWindow::slotHelp()
{
    HelpDialog dialog;
    dialog.exec();
}

void MainWindow::slotReloadSerialPorts()
{
    QStringList list;
    foreach(QSerialPortInfo info, QSerialPortInfo::availablePorts())
    {
        QString portName = info.portName();
        list.append(portName);
    }
    ui->comboPort->clear();
    ui->comboPort->addItems(list);
    updateInterface();
}

void MainWindow::slotBrowse()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Select a binary file"), QString(), "*.bin");
    ui->lineFile->setText(filePath);

    QSettings settings;
    settings.setValue("lastBinaryFile", QVariant(filePath));
}

void MainWindow::slotConnect()
{
    if(!m_connected)
    {
        m_connected = loader->open(ui->comboPort->currentText());
    }
    else
    {
        loader->close();
        m_connected = false;
    }
    updateInterface();
}

void MainWindow::slotTransport()
{
    bool transportIsUart = ui->comboTransport->currentText() == "UART";
    if(transportIsUart)
        loader->setTransport(GeckoLoader::TransportUART);
    else
        loader->setTransport(GeckoLoader::TransportUSB);
    updateInterface();
}

void MainWindow::slotUpload()
{
    disconnect(serialPort, SIGNAL(readyRead()), this, SLOT(slotDataReady()));
    if(ui->comboBootEnPol->currentText() == "HIGH")
        loader->setBootEnablePolarity(true);
    else
        loader->setBootEnablePolarity(false);
    loader->upload(ui->lineFile->text());
    connect(serialPort, SIGNAL(readyRead()), this, SLOT(slotDataReady()));
}

void MainWindow::slotSendASCII()
{
    QString text = ui->lineASCII->text();
    serialPort->write(text.toLatin1());
}

void MainWindow::slotDataReady()
{
    while(serialPort->bytesAvailable() > 0)
    {
        ui->textLog->appendPlainText(serialPort->readAll());
    }
}

void MainWindow::updateInterface()
{
    ui->buttonConnect->setDisabled(ui->comboPort->count() == 0);
    ui->buttonUpload->setEnabled(m_connected);

    if(m_connected)
        ui->buttonConnect->setText(tr("Disconnect"));
    else
        ui->buttonConnect->setText(tr("Connect"));

    ui->comboPort->setEnabled(!m_connected);

    bool transportIsUart = ui->comboTransport->currentText() == "UART";
    ui->labelBootEn->setVisible(transportIsUart);
    ui->comboBootEnPol->setVisible(transportIsUart);
}

#endif //EFM32_LOADER_GUI
