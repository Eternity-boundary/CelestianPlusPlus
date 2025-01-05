// Created by Eternity_boundary on Jan 5,2025
#include "Headers\Celestian.h"
#include "Headers\JsonRequestHandler.h"
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

void JsonRequestHandler::sendJsonRequest(const QString& textContent)
{
	int currentGroupId = Celestian::getCurrentGroupId();
	if (currentGroupId == -1) {
		QMessageBox::warning(nullptr, "错误", "群组 ID 无效，请重试！");
		return;
	}

	QUrl apiUrl("http://localhost:3000/send_group_msg");
	QNetworkRequest request(apiUrl);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	// 构建 JSON 数据
	QJsonObject jsonData;
	jsonData["group_id"] = currentGroupId;

	QJsonArray messageArray;

	QJsonObject atMessage;
	atMessage["type"] = "at";
	atMessage["data"] = QJsonObject{ {"qq", "3889015870"} };  // 固定 QQ 号
	messageArray.append(atMessage);

	QJsonObject textMessage;
	textMessage["type"] = "text";
	textMessage["data"] = QJsonObject{ {"text", textContent} };  // 传入自定义文本内容
	messageArray.append(textMessage);

	jsonData["message"] = messageArray;

	// 创建局部 QNetworkAccessManager
	QNetworkAccessManager* networkManager = new QNetworkAccessManager();
	QNetworkReply* reply = networkManager->post(request, QJsonDocument(jsonData).toJson());

	// 处理响应
	QObject::connect(reply, &QNetworkReply::finished, [reply, networkManager]() {
		QByteArray responseData = reply->readAll();
		qDebug() << "Server response:" << responseData;

		if (reply->error() != QNetworkReply::NoError) {
			QMessageBox::warning(nullptr, "错误", "发送请求失败：" + reply->errorString());
		}

		reply->deleteLater();
		networkManager->deleteLater();
		});
}