#include "backpackMan.h"
#include "LogProcessor.h"
#include "Celestian.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QMessageBox>
#include <QDebug>

backpackMan::backpackMan(QWidget* parent)
	: QDialog(parent), networkManager(new QNetworkAccessManager(this)), currentGroupId(-1)
{
	ui.setupUi(this);
	ui.tableWidget->setColumnCount(2);
	// 可选：设置表头
	QStringList headers;
	headers << "药材名称" << "数量";
	ui.tableWidget->setHorizontalHeaderLabels(headers);

	// 连接“获取页面”按钮与槽函数
	connect(ui.refreshPage, &QPushButton::clicked, this, &backpackMan::onRefreshPageClicked);
}

void backpackMan::onDataReceived(const QString& data)
{
	// 1. 使用工具类清洗数据
	QString cleanedData = LogProcessor::processLogMessage(data, currentGroupId);
	qDebug() << "清洗后的数据：" << cleanedData;

	// 2. 进一步清理噪音（移除常见无关部分）
	cleanedData.remove(QRegularExpression(R"(&enter=false&reply=false)"));  // 移除无关参数
	cleanedData.remove(QRegularExpression(R"(查看|炼金|上一页|背包帮助|跳转【页数】|下一页)"));  // 移除无关内容
	cleanedData.replace(QRegularExpression(R"(\s{2,})"), " ");  // 替换多余空格为单个空格
	qDebug() << "进一步清理后的数据：" << cleanedData;

	// 3. 正则表达式提取药材名称和数量
	QRegularExpression regex(R"((?:一品药材|二品药材|三品药材|四品药材|五品药材|六品药材|七品药材)\s(\S+)\s数量:(\d+))");
	QRegularExpressionMatchIterator i = regex.globalMatch(cleanedData);

	// 4. 清空表格
	ui.tableWidget->setRowCount(0);

	// 5. 遍历匹配结果并填入表格
	while (i.hasNext()) {
		QRegularExpressionMatch match = i.next();
		QString herbName = match.captured(1);  // 提取药材名称
		QString quantity = match.captured(2);  // 提取数量

		// 在表格中插入新行
		int rowCount = ui.tableWidget->rowCount();
		ui.tableWidget->insertRow(rowCount);

		// 添加药材名称到第一列
		QTableWidgetItem* herbItem = new QTableWidgetItem(herbName);
		ui.tableWidget->setItem(rowCount, 0, herbItem);

		// 添加数量到第二列
		QTableWidgetItem* quantityItem = new QTableWidgetItem(quantity);
		ui.tableWidget->setItem(rowCount, 1, quantityItem);
	}

	qDebug() << "提取并显示了药材数据：" << cleanedData;
}

void backpackMan::onRefreshPageClicked()
{
	int currentGroupId = Celestian::getCurrentGroupId();
	if (currentGroupId == -1) {
		QMessageBox::warning(this, "错误", "群组 ID 无效，请重试！");
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
	textMessage["data"] = QJsonObject{ {"text", "药材背包"} };  // 固定文本
	messageArray.append(textMessage);

	jsonData["message"] = messageArray;

	// 发送 POST 请求
	QNetworkReply* reply = networkManager->post(request, QJsonDocument(jsonData).toJson());

	// 处理响应
	connect(reply, &QNetworkReply::finished, this, [this, reply]() {
		QByteArray responseData = reply->readAll();
		qDebug() << "Server response:" << responseData;

		if (reply->error() != QNetworkReply::NoError) {
			QMessageBox::warning(this, "错误", "发送请求失败：" + reply->errorString());
		}
		else {
			QMessageBox::information(this, "成功", "请求已发送！");
		}

		reply->deleteLater();
		});
}