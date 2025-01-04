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

private slots:
	void onRefreshPageClicked();  // 槽函数，处理“获取页面”按钮点击

private:
	Ui::backpackMan ui;
	QNetworkAccessManager* networkManager;
	int currentGroupId;  // 存储当前群组 ID
};
