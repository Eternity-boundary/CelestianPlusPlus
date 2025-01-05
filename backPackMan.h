#pragma once
#include <QDialog>
#include <QNetworkAccessManager>
#include "ui_backpack.h"

class backpackMan : public QDialog
{
	Q_OBJECT

public:
	explicit backpackMan(QWidget* parent = nullptr);
	void setGroupId(int groupId);  // 设置群组 ID

public slots:
	void onDataReceived(const QString& data);  // 槽函数，处理数据接收
	void onTableItemDoubleClicked(QTableWidgetItem* item);  // 处理表格双击事件

private slots:
	void onRefreshPageClicked();  // 槽函数，处理“获取页面”按钮点击
	void onPreviousPageClicked();  // 槽函数，处理“上一页”按钮点击
	void onNextPageClicked();  // 槽函数，处理“下一页”按钮点击
	void onSellButtonClicked();  // 槽函数，处理“卖出”按钮点击
private:
	Ui::backpackMan ui;
	QNetworkAccessManager* networkManager;
	int currentGroupId;  // 存储当前群组 ID
	QString selectedHerbName;  // 存储选中的药材名称
};

qint64 convertPrice(const QString& priceStr);
