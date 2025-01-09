// Created by Eternity_boundary on Jan 4,2025
#pragma once
#define _CELESTIAN_DISABLED_WARNING 26813 26495 4820 4458 5045
#pragma warning(push)
#pragma warning(disable:_CELESTIAN_DISABLED_WARNING)
#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QJsonArray>
#include <QTcpServer>
#include <QNetworkReply>
#include <QTableWidgetItem>
#pragma warning(pop)

constexpr auto SENDTOGROUP = false;
constexpr auto SENDTOPRIVATE = true;

QT_BEGIN_NAMESPACE
namespace Ui { class CelestianClass; }
QT_END_NAMESPACE

class Celestian : public QMainWindow
{
	Q_OBJECT

public:
	explicit Celestian(QWidget* parent = nullptr);
	~Celestian();

	Celestian(const Celestian&) = delete;
	Celestian& operator=(const Celestian&) = delete;

	static qint64 getUserId();
	static int getCurrentGroupId();
	static void setCurrentGroupId(int id);

signals:
	void loginInfoReceived();
	void newLogDataReceived(const QString& message);
	void dataReceived(const QString& data);

private slots:
	void on_pushButton_clicked();
	void handleApiResponse(QNetworkReply* reply);
	void onTableItemDoubleClicked(QTableWidgetItem* item);
	void onPracticeButtonClicked();
	void onPackButtonClicked();
	void onactButtonClicked();
	void onHarvestDataReceived(const QString& data);
	void onSignButtonClicked();

private:
	void startHttpServer();
	void getLoginInfo();
	void setUserId(qint64 id);
	void populateTable(const QJsonArray& data);
	bool updateStatusIndicator(bool isOnline);
	int harvestTime = -1;  // 存储提取到的时间（以分钟为单位）
	bool hasUpdatedTime = false;
	bool online_status_flag = false;
	QTimer* heartBeatTimer;

	QString processServerReport(const QByteArray& requestData);  // 封装的解析函数

	Ui::CelestianClass* ui;
	QNetworkAccessManager* networkManager;

	static qint64 userId;
	static int currentGroupId;

	QJsonArray groupData;
};
