// Created by Eternity_boundary on Jan 4,2025
#include "Headers\backPackMan.h"
#include "Headers\Celestian.h"
#include "Headers\JsonRequestHandler.h"
#include "Headers\LogProcessor.h"
#include "Headers\practice.h"
#pragma warning(push)
#pragma warning(disable: _CELESTIAN_DISABLED_WARNING)
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <Qmessagebox.h>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#pragma warning(pop)

Practice::Practice(QWidget* parent)
	: QDialog(parent), networkManager(new QNetworkAccessManager(this))
{
	// 创建日志窗口
	QVBoxLayout* layout = new QVBoxLayout(this);
	logOutput = new QTextEdit(this);
	logOutput->setReadOnly(true);  // 设置为只读
	layout->addWidget(logOutput);

	QPushButton* closeButton = new QPushButton("关闭日志窗口", this);
	layout->addWidget(closeButton);

	connect(closeButton, &QPushButton::clicked, this, &Practice::close);

	// 连接主窗口的信号，接收处理后的日志数据
	Celestian* mainWindow = qobject_cast<Celestian*>(parent);
	if (mainWindow) {
		connect(mainWindow, &Celestian::newLogDataReceived, this, &Practice::appendLogMessage);
	}

	timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &Practice::sendGroupMessage);

	sendGroupMessage();

	timer->start(PRACTICE_TIMER_INTERVAL);

	// 连接关闭按钮，停止定时器并关闭窗口
	connect(closeButton, &QPushButton::clicked, this, [this]() {
		timer->stop();
		close();
		});
}

void Practice::appendLogMessage(const QString& message)
{
	QString formattedMessage = LogProcessor::processLogMessage(message, Celestian::getUserId());

	if (!formattedMessage.isEmpty()) {
		logOutput->append(formattedMessage);
		logOutput->repaint();  // 强制刷新界面
		qDebug() << "Appended to log.";
	}
	else {
		qDebug() << "Message does not match pattern.";
	}
}

void Practice::sendGroupMessage()
{
	if (Celestian::getCurrentGroupId() == -1) {
		qDebug() << "Invalid group ID, skipping API call...";
		return;
	}

	JsonRequestHandler::sendJsonRequest("修炼");
}