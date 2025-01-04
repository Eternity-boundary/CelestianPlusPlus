#include "practice.h"
#include "Celestian.h"
#include "LogProcessor.h"
#include <QVBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

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

	QUrl apiUrl("http://localhost:3000/send_group_msg");
	QNetworkRequest request(apiUrl);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	// 构建 JSON 数据
	QJsonObject jsonData;
	jsonData["group_id"] = Celestian::getCurrentGroupId();

	QJsonArray messageArray;

	QJsonObject atMessage;
	atMessage["type"] = "at";
	atMessage["data"] = QJsonObject{ {"qq", "3889015870"} };
	messageArray.append(atMessage);

	QJsonObject textMessage;
	textMessage["type"] = "text";
	textMessage["data"] = QJsonObject{ {"text", "修仙"} };
	messageArray.append(textMessage);

	jsonData["message"] = messageArray;
	jsonData["post_type"] = "message";  // 添加 post_type 字段

	// 发送 POST 请求
	QNetworkReply* reply = networkManager->post(request, QJsonDocument(jsonData).toJson());

	// 处理响应
	connect(reply, &QNetworkReply::finished, this, [this, reply]() {
		QByteArray responseData = reply->readAll();
		qDebug() << "Server response data:" << responseData;

		if (reply->error() != QNetworkReply::NoError) {
			logOutput->append("Failed to send message: " + reply->errorString());
		}
		else {
			logOutput->append("Message sent successfully.");
		}

		reply->deleteLater();
		});
}