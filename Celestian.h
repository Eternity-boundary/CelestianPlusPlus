// Created by Eternity_boundary on Jan 4,2025
#pragma once
#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QJsonArray>

QT_BEGIN_NAMESPACE
namespace Ui { class CelestianClass; }
QT_END_NAMESPACE

class Celestian : public QMainWindow
{
	Q_OBJECT

public:
	explicit Celestian(QWidget* parent = nullptr);
	~Celestian();

	static int getUserId(); // 提供一个只读接口

signals:
	void loginInfoReceived();

private slots:
	void on_pushButton_clicked();
	void handleApiResponse(QNetworkReply* reply);

private:
	void startHttpServer();
	void getLoginInfo();
	void populateTable(const QJsonArray& data);
	void updateStatusIndicator(bool isOnline);

	Ui::CelestianClass* ui;
	QNetworkAccessManager* networkManager;
	static int userId;
	static void setUserId(int id);
};