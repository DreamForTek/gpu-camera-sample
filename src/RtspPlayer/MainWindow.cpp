#include "MainWindow.h"
#include "ui_mainwindow.h"

#include "DialogOpenServer.h"

#include <QMapIterator>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(&m_timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
    m_timer.start(300);

    //openServer("rtsp://127.0.0.1:1234/live.sdp");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionOpen_RTSP_server_triggered()
{
    DialogOpenServer dlg;
    if(m_rtspServer.get())
        dlg.setUrl(m_rtspServer->url());
    if(dlg.exec()){
        openServer(dlg.url());
    }
}

void MainWindow::openServer(const QString &url)
{
    ui->widgetPlay->setReceiver(nullptr);
    m_rtspServer.reset(new RTSPServer);

    connect(m_rtspServer.get(), SIGNAL(startStopServer(bool)), this, SLOT(onStartStopServer(bool)), Qt::QueuedConnection);

    m_rtspServer->startServer(url, QMap<QString,QVariant>());
    ui->widgetPlay->setReceiver(m_rtspServer.get());
    ui->statusbar->showMessage("Try to open local server", 2000);
}

void MainWindow::openClient(const QString &url)
{
    ui->widgetPlay->setReceiver(nullptr);
    m_rtspServer.reset(new RTSPServer);

    connect(m_rtspServer.get(), SIGNAL(startStopServer(bool)), this, SLOT(onStartStopServer(bool)), Qt::QueuedConnection);

    QMap<QString,QVariant> params;
    params["client"] = 1;
	params["mjpeg_fastvideo"] = ui->rbFastvideoJpeg->isChecked();
	params["h264_cuvid"] = ui->rbCuvid->isChecked();
	params["libx264"] = true;
	params["ctp"] = ui->rbCtp->isChecked();

    m_rtspServer->startServer(url, params);
    ui->widgetPlay->setReceiver(m_rtspServer.get());
    ui->statusbar->showMessage("Try to open remote server", 2000);
}

void MainWindow::on_actionClose_RTSP_server_triggered()
{
    ui->widgetPlay->setReceiver(nullptr);
    m_rtspServer.reset();

	ui->gbDecodersH264->setEnabled(true);
	ui->gbTransportProtocol->setEnabled(true);

    ui->statusbar->showMessage("Rtsp server is close");
}

void MainWindow::on_actionOpen_RTSP_client_triggered()
{
    DialogOpenServer dlg;
    dlg.setName("Open client");
    if(m_rtspServer.get())
        dlg.setUrl(m_rtspServer->url());
    else
        dlg.setUrl("");
    if(dlg.exec()){
        openClient(dlg.url());
		ui->actionPlay->setChecked(true);
		ui->gtgWidget->start();
    }
}

void MainWindow::on_pb_openRtsp_clicked()
{
    QString url = ui->le_rtsp_address->text();
    if(url.isEmpty()){
        ui->statusbar->showMessage("Url is empty");
        return;
    }

	openClient(url);
}

void MainWindow::onTimeout()
{
    if(m_rtspServer.get()){
        if(m_rtspServer->isError()){
            ui->statusbar->showMessage(m_rtspServer->errorStr());
        }else{
            if(m_rtspServer->done()){
                ui->statusbar->showMessage("Rtsp server is close");
            }
        }

		if(m_rtspServer->isServerOpened()){
			ui->gbDecodersH264->setEnabled(false);
			ui->gbTransportProtocol->setEnabled(false);
		}else{
			ui->gbDecodersH264->setEnabled(true);
			ui->gbTransportProtocol->setEnabled(true);
		}

        ui->lb_count_frames->setText(QString::number(m_rtspServer->framesCount()));
        ui->lb_fps->setText(QString::number(ui->widgetPlay->fps(), 'f', 1) + " frames/s");
        ui->lb_bitrate->setText(QString::number((ui->widgetPlay->bytesReaded() * 8)/1000, 'f', 1) + " kB/s");

		QString sdur;

		QMap<QString, double> durations = m_rtspServer->durations();

		{
			sdur += "Decoding: \n";

			QMapIterator<QString, double> it(durations);
			while(it.hasNext()){
				it.next();

				sdur += it.key() + " = " + QString::number(it.value(), 'f', 3) + " ms\n";
			}
		}

		{
			sdur += "\nShow: \n";

			durations = ui->widgetPlay->durations();
			QMapIterator<QString, double> it(durations);
			while(it.hasNext()){
				it.next();

				sdur += it.key() + " = " + QString::number(it.value(), 'f', 3) + " ms\n";
			}
		}

		ui->lb_durations->setText(sdur);
    }
}

void MainWindow::on_pb_stopRtsp_clicked()
{
    on_actionClose_RTSP_server_triggered();
}

void MainWindow::onStartStopServer(bool start)
{
    if(start){
        ui->gtgWidget->start();
        ui->statusbar->showMessage("RTSP started");
    }else{
        ui->gtgWidget->stop();
        ui->statusbar->showMessage("RTSP stopped");
    }
}

void MainWindow::on_actionPlay_toggled(bool arg1)
{
	if(arg1)
	{
		if(m_rtspServer.get()){
			m_rtspServer->startDecode();
		}
		//ui->widgetPlay->start();
		ui->gtgWidget->start();
	}
	else
	{
		if(m_rtspServer.get()){
			m_rtspServer->stopDecode();
		}
//		ui->widgetPlay->stop();
		ui->gtgWidget->stop();
	}
}

void MainWindow::on_rbJpegTurbo_clicked(bool checked)
{
	if(checked){
		if(m_rtspServer.get()){
			m_rtspServer->setUseFastVideo(false);
		}
	}
}

void MainWindow::on_rbFastvideoJpeg_clicked(bool checked)
{
	if(checked){
		if(m_rtspServer.get()){
			m_rtspServer->setUseFastVideo(true);
		}
	}
}
