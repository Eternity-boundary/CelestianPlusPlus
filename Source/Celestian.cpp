// Created by Eternity_boundary on Jan 4,2025
#include "Headers\backPackMan.h"
#include "Headers\Celestian.h"
#include "Headers\Practice.h"
#include "Headers\JsonRequestHandler.h"
#include "Headers\LogProcessor.h"
#include "ui_Celestian.h"
#pragma warning(push)
#pragma warning(disable: _CELESTIAN_DISABLED_WARNING)
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#pragma warning(pop)

qint64 Celestian::userId = -1; // 初始化 userId
int Celestian::currentGroupId = -1;// 初始化 currentGroupId
Celestian::Celestian(QWidget* parent)
	: QMainWindow(parent)
	, ui(new Ui::CelestianClass)
	, networkManager(new QNetworkAccessManager(this))
	, isRunning(false)
{
	ui->setupUi(this);

	ui->progressBar->setVisible(false);

	connect(ui->pushButton, &QPushButton::clicked, this, &Celestian::onPushButtonClicked);
	connect(networkManager, &QNetworkAccessManager::finished, this, &Celestian::handleApiResponse);
	connect(ui->tableWidget, &QTableWidget::itemDoubleClicked, this, &Celestian::onTableItemDoubleClicked);
	connect(ui->practice, &QPushButton::clicked, this, &Celestian::onPracticeButtonClicked);
	connect(ui->pack, &QPushButton::clicked, this, &Celestian::onPackButtonClicked);
	connect(ui->act, &QPushButton::clicked, this, &Celestian::onActButtonClicked);
	connect(this, &Celestian::dataReceived, this, &Celestian::onHarvestDataReceived);
	connect(ui->signIn, &QPushButton::clicked, this, &Celestian::onSignButtonClicked);
	connect(ui->bank, &QPushButton::clicked, this, &Celestian::onBankButtonClicked);
	connect(heartBeatTimer, &QTimer::timeout, this, [this]() {
		qDebug() << "heartBeat timed out";
		online_status_flag = false;
		updateStatusIndicator(online_status_flag);
		QMessageBox::warning(nullptr, "警告", "未检测到有效上报，程序将在受限模式下运行，部分功能将被禁用");
		});

	startHttpServer();
	getLoginInfo(); // 程序启动时请求 login info
	QTimer::singleShot(5000, this, [this]() {
		Q_ASSERT(Celestian::userId != -1);
		qDebug() << "User ID is valid, proceeding...";
		});
	heartBeatTimer = new QTimer(this);
	heartBeatTimer->setInterval(60000);
	heartBeatTimer->setSingleShot(true);
}

Celestian::~Celestian()
{
	delete ui;
}

qint64 Celestian::getUserId()
{
	return userId;
}

int Celestian::getCurrentGroupId()
{
	return currentGroupId;
}

void Celestian::setCurrentGroupId(int id)
{
	currentGroupId = id;
}

void Celestian::onPushButtonClicked()
{
	QUrl apiUrl("http://localhost:3000/get_group_list");
	QNetworkRequest request(apiUrl);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	QJsonObject requestBody;
	requestBody["no_cache"] = true;

	networkManager->post(request, QJsonDocument(requestBody).toJson());
}

QString Celestian::processServerReport(const QByteArray& requestData)
{
	int jsonStartIndex = requestData.indexOf("\r\n\r\n") + 4;
	if (jsonStartIndex <= 4 || jsonStartIndex >= requestData.size()) {
		qWarning() << "Failed to extract JSON data from request";
		return QString();
	}

	QByteArray jsonData = requestData.mid(jsonStartIndex);
	qDebug() << "Extracted JSON data:" << jsonData;

	QJsonParseError parseError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);
	if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
		qWarning() << "Failed to parse JSON data:" << parseError.errorString();
		return QString();
	}

	QJsonObject jsonObj = jsonDoc.object();
	QString postType = jsonObj.value("post_type").toString();

	if (postType == "message") {
		QString rawMessage = QString::fromUtf8(jsonObj.value("raw_message").toString().toUtf8());
		qDebug() << "Parsed message from server:" << rawMessage;
		return rawMessage;  // 返回格式化消息
	}
	else if (postType == "meta_event") {
		QString metaEventType = jsonObj.value("meta_event_type").toString();
		if (metaEventType == "heartbeat") {
			qDebug() << "Heartbeat event received";
			bool online_status_flag = true;
			updateStatusIndicator(online_status_flag);
			heartBeatTimer->start();
		}
	}
	else {
		qDebug() << "Received unknown post_type or invalid format";
	}

	return QString();
}

void Celestian::startHttpServer()
{
	QTcpServer* server = new QTcpServer(this);

	connect(server, &QTcpServer::newConnection, this, [this, server]() {
		QTcpSocket* client = server->nextPendingConnection();
		if (client) {
			connect(client, &QTcpSocket::readyRead, this, [this, client]() {
				QByteArray requestData = client->readAll();
				qDebug() << "Received raw request data:" << requestData;

				QString formattedMessage = processServerReport(requestData);
				if (!formattedMessage.isEmpty()) {
					emit newLogDataReceived(formattedMessage); // 触发信号，将消息发送给日志窗口
					emit dataReceived(formattedMessage); // 触发信号，将消息发送给背包窗口
				}

				QByteArray response = "HTTP/1.1 200 OK\r\n"
					"Content-Type: text/plain\r\n"
					"Content-Length: 17\r\n"
					"\r\n"
					"Request received";
				client->write(response);
				client->disconnectFromHost();
				});
		}
		});

	if (!server->listen(QHostAddress::Any, 7777)) {
		QMessageBox::critical(this, "Error", "Failed to start HTTP server on port 7777.");
	}
	else {
		qDebug() << "HTTP server started on port" << server->serverPort();
	}
}

bool loginInfoRequested = false;

void Celestian::getLoginInfo()
{
	if (loginInfoRequested) {
		qDebug() << "Login info request already sent";
		return;  // 防止重复请求
	}

	loginInfoRequested = true;
	qDebug() << "Sending request to get login info";

	QUrl apiUrl("http://localhost:3000/get_login_info");
	QNetworkRequest request(apiUrl);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	request.setRawHeader("Connection", "close");

	// 使用局部 QNetworkAccessManager
	QNetworkAccessManager* localManager = new QNetworkAccessManager(this);
	QNetworkReply* reply = localManager->get(request);

	connect(reply, &QNetworkReply::finished, this, [this, reply, localManager]() {
		QByteArray responseData = reply->readAll();
		qDebug() << "Raw response data in handleApiResponse:" << responseData;

		if (reply->error() != QNetworkReply::NoError) {
			qWarning() << "Failed to get login info:" << reply->errorString();
			reply->deleteLater();
			localManager->deleteLater();  // 删除局部 manager
			return;
		}

		if (responseData.isEmpty()) {
			qWarning() << "Empty response data, possible keep-alive issue.";
			reply->deleteLater();
			localManager->deleteLater();  // 删除局部 manager
			return;
		}

		QJsonParseError parseError;
		QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData, &parseError);
		if (parseError.error != QJsonParseError::NoError) {
			qWarning() << "Invalid JSON response:" << parseError.errorString();
			QMessageBox::warning(this, "Parse Error", "Invalid JSON response: " + parseError.errorString());
			reply->deleteLater();
			localManager->deleteLater();  // 删除局部 manager
			return;
		}

		QJsonObject jsonResponse = jsonDoc.object();
		if (jsonResponse.value("status").toString() != "ok" || jsonResponse.value("retcode").toInt() != 0) {
			qWarning() << "Unexpected API response";
			QMessageBox::warning(this, "API Error", "Unexpected API response");
			reply->deleteLater();
			localManager->deleteLater();  // 删除局部 manager
			return;
		}

		QJsonObject data = jsonResponse.value("data").toObject();
		qint64 receivedUserId = data.value("user_id").toVariant().toLongLong();
		qDebug() << "Login info received, user_id:" << receivedUserId;

		Celestian::userId = static_cast<qint64>(receivedUserId);

		emit loginInfoReceived();  // 发送信号，通知登录信息已接收

		reply->deleteLater();
		localManager->deleteLater();  // 确保局部 manager 被删除
		});

	connect(reply, &QNetworkReply::errorOccurred, this, [this, reply, localManager](QNetworkReply::NetworkError code) {
		qWarning() << "Network error occurred:" << code << reply->errorString();
		reply->deleteLater();
		localManager->deleteLater();  // 确保局部 manager 被删除
		});
}

void Celestian::setUserId(qint64 id)
{
	userId = id;
}

void Celestian::handleApiResponse(QNetworkReply* reply)
{
	if (reply->error() != QNetworkReply::NoError) {
		QMessageBox::warning(this, "API Error", reply->errorString());
		reply->deleteLater();
		return;
	}

	QByteArray responseData = reply->readAll();
	qDebug() << "Raw response data in handleApiResponse:" << responseData;

	// 检测并移除 UTF-8 BOM（如果存在）
	if (responseData.startsWith("\xEF\xBB\xBF")) {
		responseData.remove(0, 3);
		qDebug() << "Removed BOM from response data";
	}

	QJsonParseError parseError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData, &parseError);
	if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
		QMessageBox::warning(this, "Parse Error", "Invalid JSON response: " + parseError.errorString());
		qWarning() << "Parse error:" << parseError.errorString();
		reply->deleteLater();
		return;
	}

	QJsonObject jsonResponse = jsonDoc.object();
	if (jsonResponse.value("status").toString() != "ok" || jsonResponse.value("retcode").toInt() != 0) {
		QMessageBox::warning(this, "API Error", "Unexpected API response");
		reply->deleteLater();
		return;
	}

	QJsonArray groupList = jsonResponse.value("data").toArray();
	populateTable(groupList);

	reply->deleteLater();
}

void Celestian::populateTable(const QJsonArray& data)
{
	groupData = data;  // 存储完整的 group 列表
	ui->tableWidget->clearContents();
	ui->tableWidget->setRowCount(data.size());
	ui->tableWidget->setColumnCount(1);
	ui->tableWidget->setHorizontalHeaderLabels({ "Group Name" });

	ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
	ui->tableWidget->horizontalHeader()->setSectionsMovable(false);
	ui->tableWidget->setDragDropMode(QAbstractItemView::NoDragDrop);
	ui->tableWidget->setDragEnabled(false);
	ui->tableWidget->setDropIndicatorShown(false);

	for (int i = 0; i < data.size(); ++i) {
		QJsonObject item = data[i].toObject();
		QString groupName = item.value("group_name").toString();
		ui->tableWidget->setItem(i, 0, new QTableWidgetItem(groupName));
	}
}

bool Celestian::updateStatusIndicator(bool isOnline)
{
	if (isOnline) {
		ui->statusIndicator->setStyleSheet("background-color: green; border-radius: 10px;");
		return true;
	}
	else {
		ui->statusIndicator->setStyleSheet("background-color: red; border-radius: 10px;");
		return false;
	}
}

void Celestian::onTableItemDoubleClicked(QTableWidgetItem* item)
{
	int row = item->row();  // 获取双击的行号

	// 从 groupData 中提取 group_id
	QJsonObject itemData = groupData[row].toObject();
	int temp = itemData.value("group_id").toInt();
	setCurrentGroupId(temp);

	qDebug() << "Group ID selected:" << currentGroupId;
	qDebug() << "Group ID selected in onTableItemDoubleClicked:" << currentGroupId;
}

void Celestian::onPracticeButtonClicked()
{
	if (currentGroupId == -1) {
		QMessageBox::warning(this, "Error", "请先选择一个有效的群组！");
		return;
	}

	Practice* practiceWindow = new Practice(this);
	practiceWindow->setAttribute(Qt::WA_DeleteOnClose);  // 窗口关闭时自动删除
	practiceWindow->show();
}

void Celestian::onActButtonClicked()
{
	if (!isRunning) {
		if (currentGroupId == -1) {
			QMessageBox::warning(this, "提示", "请先双击选择一个群组！");
			return;
		}

		QMessageBox::information(this, "提示", "已发送采药请求，请等待时间结束...");

		// 初始化计数器和最大次数
		currentCount = 1;
		maxCount = 10;

		// 显示并初始化进度条
		ui->progressBar->setVisible(true);
		ui->progressBar->setValue(currentCount);
		ui->progressBar->setMaximum(maxCount);
		ui->progressBar->setFormat(QString("%1/%2").arg(currentCount).arg(maxCount));
		ui->progressBar->setTextVisible(true);  // 显示文本

		// 立即发送第一次“采药”请求
		hasUpdatedTime = false;
		JsonRequestHandler::sendJsonRequest(SENDTOGROUP, "采药");

		// 创建定时器
		timer = new QTimer(this);

		connect(timer, &QTimer::timeout, this, [this]() {
			hasUpdatedTime = false;
			JsonRequestHandler::sendJsonRequest(SENDTOGROUP, "采药");
			currentCount++;

			// 更新进度条
			ui->progressBar->setValue(currentCount);
			ui->progressBar->setFormat(QString("%1/%2").arg(currentCount).arg(maxCount));

			if (currentCount >= maxCount) {
				timer->stop();
				timer->deleteLater();
				timer = nullptr;
				isRunning = false;
				ui->act->setText("启动采药");

				ui->progressBar->setVisible(false);

				QMessageBox::information(this, "提示", "采药任务已完成！");
			}
			});
		QTimer::singleShot(3000, [this]() {
			if (objHarvestTime)
			{
				timer->start(*objHarvestTime + 65000);
				isRunning = true;
				ui->act->setText("停止采药");
				delete objHarvestTime;
			}
			else
			{
				QMessageBox::warning(this, "警告", "未获取到时间信息");
				ui->progressBar->setVisible(false);
			}
			});
	}
	else {
		if (timer && timer->isActive()) {
			timer->stop();
			timer->deleteLater();
			timer = nullptr;
			QMessageBox::information(this, "提示", "采药任务已手动停止！");
		}

		isRunning = false;
		ui->act->setText("启动采药");

		ui->progressBar->setVisible(false);
	}
}

void Celestian::onHarvestDataReceived(const QString& data)
{
	int harvestTimes = 0;
	// 检查是否已更新过时间，若已更新则直接返回
	if (hasUpdatedTime) return;

	// 1. 使用工具类清洗数据
	QString cleanedData = LogProcessor::processLogMessage(data, currentGroupId);
	qDebug() << "清洗后的数据：" << cleanedData;
	hasUpdatedTime = true;  // 标记已更新

	// 2. 使用正则表达式匹配时间信息
	QRegularExpression regex(R"(开始采药(\d+)分钟后采药归来)");
	QRegularExpressionMatch match = regex.match(cleanedData);
	if (match.hasMatch()) {
		if (harvestTimes <= 10)
		{
			harvestTime = match.captured(1).toInt() * 60000;  // 将时间转换为毫秒
			objHarvestTime = new int(harvestTime);
			qDebug() << "提取的时间信息（毫秒）：" << harvestTime;

			// 设置定时器，在 harvestTime 毫秒后发送“采药归来”请求
			QTimer::singleShot(harvestTime, this, []() {
				JsonRequestHandler::sendJsonRequest(SENDTOGROUP, "采药归来");
				JsonRequestHandler::sendJsonRequest(SENDTOGROUP, "外出游历");
				});

			QMessageBox::information(this, "提示", QString("采药已完成，%1分钟后将发送‘采药归来’请求").arg(harvestTime / 60000));
			harvestTimes++;
		}
		else
		{
			QMessageBox::warning(this, "提示", "采药次数已达到10次");
		}
	}
}

void Celestian::onSignButtonClicked()
{
	JsonRequestHandler::sendJsonRequest(SENDTOGROUP, "签到");
	QTimer::singleShot(500, []() {
		JsonRequestHandler::sendJsonRequest(SENDTOGROUP, "鱼饵兑换");
		});

	QTimer::singleShot(500, []() {
		JsonRequestHandler::sendJsonRequest(SENDTOGROUP, "宗门丹药领取");
		});

	QTimer::singleShot(500, []() {
		JsonRequestHandler::sendJsonRequest(SENDTOGROUP, "灵田收取");
		});
}

void Celestian::onBankButtonClicked()
{
	JsonRequestHandler::sendJsonRequest(SENDTOPRIVATE, "灵庄信息");
}

void Celestian::onPackButtonClicked()
{
	if (currentGroupId == -1) {
		QMessageBox::warning(this, "提示", "请先双击选择一个群组！");
		return;
	}

	backpackMan* backpackWindow = new backpackMan(this);
	backpackWindow->setGroupId(currentGroupId);  // 传递当前群组 ID

	// 连接主界面的信号与子窗口的槽函数
	connect(this, &Celestian::dataReceived, backpackWindow, &backpackMan::onDataReceived);

	backpackWindow->setAttribute(Qt::WA_DeleteOnClose);
	backpackWindow->exec();
}
void backpackMan::setGroupId(int groupId)
{
	currentGroupId = groupId;
}