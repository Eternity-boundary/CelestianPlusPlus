// Created by Eternity_boundary on Jan 5,2025
#include "Headers\backpackMan.h"
#include "Headers\Celestian.h"
#include "Headers\JsonRequestHandler.h"
#include "Headers\LogProcessor.h"
#pragma warning(push)
#pragma warning(disable: _CELESTIAN_DISABLED_WARNING)
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#pragma warning(pop)

qint64 selectedPrice;//价格
backpackMan::backpackMan(QWidget* parent)
	: QDialog(parent), networkManager(new QNetworkAccessManager(this)), currentGroupId(-1)
{
	ui.setupUi(this);
	ui.tableWidget->setColumnCount(2);
	setWindowTitle("背包管理");

	// 设置表头
	QStringList headers;
	headers << "药材名称" << "数量";
	ui.tableWidget->setHorizontalHeaderLabels(headers);

	// 禁用表格的编辑与拖动
	ui.tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);  // 禁用编辑
	ui.tableWidget->setDragDropMode(QAbstractItemView::NoDragDrop);      // 禁用拖放
	ui.tableWidget->setDragEnabled(false);
	ui.tableWidget->setDropIndicatorShown(false);

	connect(ui.refreshPage, &QPushButton::clicked, this, &backpackMan::onRefreshPageClicked);
	connect(ui.tableWidget, &QTableWidget::itemDoubleClicked, this, &backpackMan::onTableItemDoubleClicked);
	connect(ui.previousPage, &QPushButton::clicked, this, &backpackMan::onPreviousPageClicked);
	connect(ui.nextPage, &QPushButton::clicked, this, &backpackMan::onNextPageClicked);
	connect(ui.sell, &QPushButton::clicked, this, &backpackMan::onSellButtonClicked);
}

void backpackMan::onDataReceived(const QString& data)
{
	// 1. 使用工具类清洗数据
	QString cleanedData = LogProcessor::processLogMessage(data, currentGroupId);
	qDebug() << "清洗后的数据：" << cleanedData;

	// 额外处理：获取价格
	QString selectedPriceStr;  // 选中的药材价格字符串
	QRegularExpression priceRegex(R"(价格:(\d{1,4}(?:\.\d{1,2})?[万亿]))");
	QRegularExpressionMatch priceMatch = priceRegex.match(cleanedData);
	if (priceMatch.hasMatch()) {
		selectedPriceStr = priceMatch.captured(1);           // 提取价格字符串
		selectedPrice = convertPrice(selectedPriceStr);      // 转换价格为整数

		// 修改显示为万单位，保留两位小数
		qDebug() << "提取的价格：" << selectedPriceStr;
		qDebug() << "转换后的价格：" << QString::number(selectedPrice / 1e4, 'f', 2) << "万";
	}

	// 2. 进一步清理噪音（移除常见无关部分）
	cleanedData.remove(QRegularExpression(R"(&enter=false&reply=false)"));  // 移除无关参数
	cleanedData.remove(QRegularExpression(R"(查看|炼金|上一页|背包帮助|跳转【页数】|下一页)"));  // 移除无关内容
	cleanedData.replace(QRegularExpression(R"(\s{2,})"), " ");  // 替换多余空格为单个空格
	qDebug() << "进一步清理后的数据：" << cleanedData;

	// 3. 正则表达式提取药材名称和数量
	QRegularExpression regex(R"((?:一品药材|二品药材|三品药材|四品药材|五品药材|六品药材|七品药材|八品药材|九品药材)\s(\S+)\s数量:(\d+))");
	QRegularExpressionMatchIterator i = regex.globalMatch(cleanedData);

	// 4. 遍历匹配结果并填入表格（避免重复插入）
	while (i.hasNext()) {
		QRegularExpressionMatch match = i.next();
		QString herbName = match.captured(1);  // 提取药材名称
		QString quantity = match.captured(2);  // 提取数量

		// 检查表格中是否已存在相同的药材名称
		bool isDuplicate = false;
		for (int row = 0; row < ui.tableWidget->rowCount(); ++row) {
			if (ui.tableWidget->item(row, 0)->text() == herbName) {
				isDuplicate = true;
				break;
			}
		}

		if (isDuplicate) {
			continue;  // 跳过已存在的药材
		}

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
	QTableWidget* table = ui.tableWidget;
	table->clearContents();
	table->setRowCount(0);
	JsonRequestHandler::sendJsonRequest(SENDTOPRIVATE, "药材背包");
}

qint16 pageCount = 1;

void backpackMan::onPreviousPageClicked()
{
	if (pageCount == 1) {
		QMessageBox::warning(this, "提示", "已经是第一页了！");
		return;
	}
	pageCount--;
	QTableWidget* table = ui.tableWidget;
	table->clearContents();
	table->setRowCount(0);
	JsonRequestHandler::sendJsonRequest(SENDTOPRIVATE, "药材背包 " + QString::number(pageCount));
}

void backpackMan::onNextPageClicked()
{
	QTableWidget* table = ui.tableWidget;
	table->clearContents();
	table->setRowCount(0);
	pageCount++;
	JsonRequestHandler::sendJsonRequest(SENDTOPRIVATE, "药材背包 " + QString::number(pageCount));
}

void backpackMan::onSellButtonClicked()
{
	if (selectedHerbName.isEmpty()) {
		QMessageBox::warning(this, "提示", "请先选择一个药材！");
		return;
	}

	if (selectedPrice == -1) {
		QMessageBox::warning(this, "提示", "价格无效，请检查数据！");
		return;
	}

	JsonRequestHandler::sendJsonRequest(SENDTOPRIVATE, "坊市刷新" + selectedHerbName);
	// TODO: 添加检测到售出上限溢出提示
	QTimer::singleShot(2000, this, [this]() {
		QMessageBox msgBox;
		msgBox.setWindowTitle("确认上架");
		msgBox.setText("选择的药材为：" + selectedHerbName + "\n价格：" + QString::number(selectedPrice / 1e4, 'f', 2) + "万\n是否确认上架？");
		msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		msgBox.setDefaultButton(QMessageBox::No);
		int ret = msgBox.exec();

		if (ret == QMessageBox::Yes) {
			// 发送“坊市上架”请求，价格使用整数形式
			JsonRequestHandler::sendJsonRequest(SENDTOGROUP, "坊市上架 " + selectedHerbName + " " + QString::number(selectedPrice));
			qDebug() << "已发送坊市上架请求：" << selectedHerbName << " " << selectedPrice;
		}
		});
}

void backpackMan::onTableItemDoubleClicked(QTableWidgetItem* item)
{
	if (item == nullptr) return;  // 检查 item 是否为空

	int row = item->row();  // 获取双击的行号
	QTableWidgetItem* herbItem = ui.tableWidget->item(row, 0);  // 获取第一列的药材名称

	if (herbItem) {
		selectedHerbName = herbItem->text();  // 存储药材名称
		qDebug() << "Selected herb name:" << selectedHerbName;
	}
}

qint64 convertPrice(const QString& priceStr)
{
	QRegularExpression regex(R"((\d+(?:\.\d+)?)([万亿]))");
	QRegularExpressionMatch match = regex.match(priceStr);

	if (!match.hasMatch()) {
		return -1;  // 返回 -1 表示匹配失败
	}

	double number = match.captured(1).toDouble();  // 提取数字部分并转换为浮点数
	QString unit = match.captured(2);              // 提取单位部分

	if (unit == "万") {
		return static_cast<qint64>(number * 1e4);  // 转换为整数，乘以 10^4
	}
	else if (unit == "亿") {
		return static_cast<qint64>(number * 1e8);  // 转换为整数，乘以 10^8
	}

	return -1;
}