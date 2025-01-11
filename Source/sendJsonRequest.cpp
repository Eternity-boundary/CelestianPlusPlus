// Created by Eternity_boundary on Jan 5, 2025
#include "Headers\Celestian.h"
#include "Headers\JsonRequestHandler.h"
#pragma warning(push)
#pragma warning(disable: _CELESTIAN_DISABLED_WARNING)
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#pragma warning(pop)

void JsonRequestHandler::sendJsonRequest(const bool sendToWhere, const QString& textContent)
{
	QUrl apiUrl = (sendToWhere == SENDTOPRIVATE)
		? QUrl("http://localhost:3000/send_private_msg")
		: QUrl("http://localhost:3000/send_group_msg");

	QJsonObject jsonData;
	QJsonArray messageArray;

	if (sendToWhere == SENDTOPRIVATE) {
		jsonData["user_id"] = 3889015870;
	}
	else {
		int currentGroupId = Celestian::getCurrentGroupId();
		if (currentGroupId == -1) {
			QMessageBox::warning(nullptr, "错误", "群组 ID 无效，请重试！");
			return; // Added return to prevent further execution
		}
		jsonData["group_id"] = currentGroupId;

		QJsonObject atMessage;
		atMessage["type"] = "at";
		atMessage["data"] = QJsonObject{ {"qq", "3889015870"} };
		messageArray.append(atMessage);
	}

	QJsonObject textMessage;
	textMessage["type"] = "text";
	textMessage["data"] = QJsonObject{ {"text", textContent} };
	messageArray.append(textMessage);

	jsonData["message"] = messageArray;

	QNetworkRequest request(apiUrl);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	QNetworkReply* reply = networkManager->post(request, QJsonDocument(jsonData).toJson());

	QObject::connect(reply, &QNetworkReply::finished, [reply]() {
		QByteArray responseData = reply->readAll();
		qDebug() << "Server response:" << responseData;

		if (reply->error() != QNetworkReply::NoError) {
			QMessageBox::warning(nullptr, "错误", "发送请求失败：" + reply->errorString());
			return; // Added return to handle errors properly
		}

		reply->deleteLater();
		});
}