#pragma once

#include <QDialog>
#include <QNetworkAccessManager>
#include <QTableWidgetItem>
#include "ui_market.h"

class market : public QDialog
{
	Q_OBJECT

public:
	explicit market(QWidget* parent = nullptr);
	~market();

	void setGroupId(int groupId);

public slots:
	void onMarketDataReceived(const QString& data);

private slots:
	void onRefreshButtonClicked();
	void onResellButtonClicked();

private:
	Ui::Form ui;
	QNetworkAccessManager* networkManager;
	int currentGroupId = -1;
	bool isUpdated = true;
	QString selectedItemName; // 选中的道具名称
	qint64 selectedPrice = -1; // 选中的道具价格
	void onTableItemDoubleClicked(QTableWidgetItem* item);
	void onBuyButtonClicked();
};