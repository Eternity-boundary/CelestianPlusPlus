//Created by Eternity_boundary on Jan.12 2025
#include "Headers\market.h"
#include "Headers\JsonRequestHandler.h"
#include "Headers\Celestian.h"
#pragma warning(push)
#pragma warning(disable:_CELESTIAN_DISABLED_WARNING)
#include <QDebug>
#include <QMessageBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTableWidgetItem>
#include <Qtimer.h>
#include <QLineEdit>
#include <QInputDialog>
#pragma warning(pop)

market::market(QWidget* parent)
	: QDialog(parent), networkManager(new QNetworkAccessManager(this))
{
	ui.setupUi(this);
	ui.tableWidget->setColumnCount(3);
	setWindowTitle("坊市管理");

	// 设置表头
	QStringList headers;
	headers << "道具名称" << "价格" << "道具 ID";
	ui.tableWidget->setHorizontalHeaderLabels(headers);

	// 禁用表格的编辑与拖动
	ui.tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
	ui.tableWidget->setDragDropMode(QAbstractItemView::NoDragDrop);
	ui.tableWidget->setDragEnabled(false);
	ui.tableWidget->setDropIndicatorShown(false);

	// 连接按钮点击信号到槽函数
	connect(ui.refreshPage, &QPushButton::clicked, this, &market::onRefreshButtonClicked);
	connect(ui.reSell, &QPushButton::clicked, this, &market::onResellButtonClicked);
	connect(ui.tableWidget, &QTableWidget::itemDoubleClicked, this, &market::onTableItemDoubleClicked);
	connect(ui.buy, &QPushButton::clicked, this, &market::onBuyButtonClicked);

	// 连接到 Celestian 的 dataReceived 信号
	Celestian* mainWindow = qobject_cast<Celestian*>(parent);
	if (mainWindow) {
		connect(mainWindow, &Celestian::dataReceived, this, &market::onMarketDataReceived);
	}
}

market::~market()
{
	delete networkManager;
}

void market::setGroupId(int groupId)
{
	currentGroupId = groupId;
}

void market::onMarketDataReceived(const QString& data)
{
	if (isUpdated) return;
	// 清理表格内容
	ui.tableWidget->clearContents();
	ui.tableWidget->setRowCount(0);

	// 使用正则表达式提取坊市数据
	QRegularExpression regex(R"((\S+)\s价格:\s([\d.]+)万\s道具ID\s(\d+))");
	QRegularExpressionMatchIterator i = regex.globalMatch(data);

	while (i.hasNext()) {
		isUpdated = true;
		QRegularExpressionMatch match = i.next();
		QString itemName = match.captured(1);  // 道具名称
		QString price = match.captured(2) + "万";  // 价格（带单位）
		QString itemId = match.captured(3);    // 道具 ID

		int rowCount = ui.tableWidget->rowCount();
		ui.tableWidget->insertRow(rowCount);

		// 将道具名称、价格和道具 ID 分别添加到表格中
		QTableWidgetItem* itemNameItem = new QTableWidgetItem(itemName);
		QTableWidgetItem* priceItem = new QTableWidgetItem(price);
		QTableWidgetItem* itemIdItem = new QTableWidgetItem(itemId);

		ui.tableWidget->setItem(rowCount, 0, itemNameItem);
		ui.tableWidget->setItem(rowCount, 1, priceItem);
		ui.tableWidget->setItem(rowCount, 2, itemIdItem);
	}

	qDebug() << "坊市数据已加载：" << data;
}

void market::onRefreshButtonClicked()
{
	QTableWidget* table = ui.tableWidget;
	table->clearContents();
	table->setRowCount(0);
	isUpdated = false;
	// 发送“我的坊市”请求
	JsonRequestHandler::sendJsonRequest(SENDTOPRIVATE, "我的坊市");
	qDebug() << "已发送‘我的坊市’请求";
}

void market::onResellButtonClicked()
{
	if (selectedItemName.isEmpty()) {
		QMessageBox::warning(this, "提示", "请先双击选择一个道具！");
		return;
	}

	// 发送“坊市刷新”请求
	QString refreshRequest = "坊市刷新 " + selectedItemName;
	JsonRequestHandler::sendJsonRequest(SENDTOPRIVATE, refreshRequest);
	qDebug() << "已发送坊市刷新请求：" << refreshRequest;

	// 连接到 Celestian 的 dataReceived 信号，等待返回数据
	Celestian* mainWindow = qobject_cast<Celestian*>(parent());
	if (mainWindow) {
		connect(mainWindow, &Celestian::dataReceived, this, [this, mainWindow](const QString& data) {
			QRegularExpression priceRegex(R"(价格:(\d{1,4}(?:\.\d{1,2})?[万亿]))");
			QRegularExpressionMatch match = priceRegex.match(data);

			if (match.hasMatch()) {
				QString priceStr = match.captured(1);
				selectedPrice = convertPrice(priceStr); // 使用工具函数转换价格

				if (selectedPrice != -1) {
					// 弹出确认窗口，显示新价格并确认是否上架
					QMessageBox::StandardButton resellReply = QMessageBox::question(
						this,
						"确认上架",
						QString("道具：%1\n刷新后的价格：%2\n是否确认上架？")
						.arg(selectedItemName)
						.arg(priceStr),
						QMessageBox::Yes | QMessageBox::No);

					if (resellReply == QMessageBox::Yes) {
						// 发送“坊市上架”请求
						QString removed = "坊市下架 " + selectedItemName;
						JsonRequestHandler::sendJsonRequest(SENDTOGROUP, removed);
						QTimer::singleShot(1500, [=]() {
							QString resellRequest = "坊市上架 " + selectedItemName + " " + QString::number(selectedPrice);
							JsonRequestHandler::sendJsonRequest(SENDTOGROUP, resellRequest);
							qDebug() << "已发送坊市上架请求：" << resellRequest;
							});
					}
					else {
						qDebug() << "用户取消了上架操作";
					}
				}
				else {
					QMessageBox::warning(this, "提示", "未提取到有效价格！");
				}
			}
			else {
				QMessageBox::warning(this, "提示", "服务器返回数据中未找到价格信息！");
			}

			// 断开信号连接，避免重复处理
			disconnect(mainWindow, &Celestian::dataReceived, nullptr, nullptr);
			});
	}
}

void market::onTableItemDoubleClicked(QTableWidgetItem* item)
{
	if (!item) return;

	int row = item->row();
	QTableWidgetItem* itemNameCell = ui.tableWidget->item(row, 0);
	QTableWidgetItem* priceCell = ui.tableWidget->item(row, 1);

	if (itemNameCell && priceCell) {
		selectedItemName = itemNameCell->text();
		selectedPrice = priceCell->text().remove("万").toDouble() * 1e4; // 转换为整数价格
		qDebug() << "已选择道具：" << selectedItemName << "，价格：" << selectedPrice;
	}
}

void market::onBuyButtonClicked()
{
	bool ok;
	QString itemInput = QInputDialog::getText(
		this,
		"购买物品",
		"请输入要购买的物品名称和数量（格式：名称 数量）：",
		QLineEdit::Normal,
		"",
		&ok);

	if (!ok || itemInput.isEmpty()) {
		QMessageBox::warning(this, "提示", "输入不能为空！");
		return;
	}

	// 使用正则表达式提取物品名称和数量
	QRegularExpression regex(R"(^(\S+)\s+(\d+)$)");
	QRegularExpressionMatch match = regex.match(itemInput);
	if (!match.hasMatch()) {
		QMessageBox::warning(this, "提示", "输入格式错误，请输入‘名称 数量’！");
		return;
	}

	QString itemName = match.captured(1);   // 提取物品名称
	int itemQuantity = match.captured(2).toInt(); // 提取物品数量

	// 发送“坊市刷新”请求，获取物品价格
	QString refreshRequest = "坊市刷新 " + itemName;
	JsonRequestHandler::sendJsonRequest(SENDTOPRIVATE, refreshRequest);
	qDebug() << "已发送坊市刷新请求：" << refreshRequest;

	// 使用定时器处理 2 秒超时逻辑
	QTimer* timeoutTimer = new QTimer(this);
	timeoutTimer->setSingleShot(true);
	timeoutTimer->start(2000);

	// 连接到 Celestian 的 dataReceived 信号，处理返回数据
	Celestian* mainWindow = qobject_cast<Celestian*>(parent());
	if (mainWindow) {
		connect(mainWindow, &Celestian::dataReceived, this, [this, mainWindow, itemName, itemQuantity, timeoutTimer](const QString& data) {
			QRegularExpression priceRegex(R"(价格:(\d{1,4}(?:\.\d{1,2})?[万亿]))");
			QRegularExpressionMatch priceMatch = priceRegex.match(data);

			if (priceMatch.hasMatch()) {
				QString priceStr = priceMatch.captured(1);
				qint64 itemPrice = convertPrice(priceStr);

				timeoutTimer->stop(); // 停止超时定时器

				// 弹出确认购买窗口
				QMessageBox::StandardButton reply = QMessageBox::question(
					this,
					"确认购买",
					QString("物品：%1\n数量：%2\n价格：%3万\n是否确认购买？")
					.arg(itemName)
					.arg(itemQuantity)
					.arg(QString::number(itemPrice / 1e4, 'f', 2)),
					QMessageBox::Yes | QMessageBox::No);

				if (reply == QMessageBox::Yes) {
					// 发送“坊市购买”请求
					QString buyRequest = "坊市购买 " + itemName + " " + QString::number(itemQuantity);
					JsonRequestHandler::sendJsonRequest(SENDTOGROUP, buyRequest);
					qDebug() << "已发送坊市购买请求：" << buyRequest;
				}

				// 断开信号连接，防止重复触发
				disconnect(mainWindow, &Celestian::dataReceived, nullptr, nullptr);
				timeoutTimer->deleteLater();
			}
			});

		// 处理超时逻辑
		connect(timeoutTimer, &QTimer::timeout, this, [this, mainWindow, timeoutTimer]() {
			QMessageBox::warning(this, "提示", "未匹配到有效的物品价格，请重新输入！");
			disconnect(mainWindow, &Celestian::dataReceived, nullptr, nullptr);
			timeoutTimer->deleteLater();
			});
	}
}