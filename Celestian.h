#pragma once
#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
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

private slots:
	void on_pushButton_clicked();
	void handleApiResponse(QNetworkReply* reply);

private:
	Ui::CelestianClass* ui;  // 使用 Ui::CelestianClass 指针
	QNetworkAccessManager* networkManager;

	void populateTable(const QJsonArray& data);
};
