#include "Celestian.h"
#include "ui_Celestian.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>

Celestian::Celestian(QWidget* parent)
	: QMainWindow(parent)
	, ui(new Ui::CelestianClass)
	, networkManager(new QNetworkAccessManager(this))
{
	ui->setupUi(this);
	connect(ui->pushButton, &QPushButton::clicked, this, &Celestian::on_pushButton_clicked);
	connect(networkManager, &QNetworkAccessManager::finished, this, &Celestian::handleApiResponse);
}

Celestian::~Celestian()
{
	delete ui;
}

void Celestian::on_pushButton_clicked()
{
	QUrl apiUrl("http://localhost:3000/get_group_list");
	QNetworkRequest request(apiUrl);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	QJsonObject requestBody;
	requestBody["no_cache"] = true;

	networkManager->post(request, QJsonDocument(requestBody).toJson());
}

void Celestian::handleApiResponse(QNetworkReply* reply)
{
	if (reply->error() != QNetworkReply::NoError) {
		QMessageBox::warning(this, "API Error", reply->errorString());
		reply->deleteLater();
		return;
	}

	QByteArray responseData = reply->readAll();
	QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
	if (!jsonDoc.isObject()) {
		QMessageBox::warning(this, "Parse Error", "Invalid JSON response");
		reply->deleteLater();
		return;
	}

	QJsonObject jsonResponse = jsonDoc.object();
	if (jsonResponse.value("status").toString() != "ok" || jsonResponse.value("retcode").toInt() != 0) {
		QMessageBox::warning(this, "API Error", "Unexpected API response");
		reply->deleteLater();
		return;
	}

	QJsonArray groupList = jsonResponse.value("data").toArray();
	populateTable(groupList);

	reply->deleteLater();
}

void Celestian::populateTable(const QJsonArray& data)
{
	ui->tableWidget->clearContents();
	ui->tableWidget->setRowCount(data.size());
	ui->tableWidget->setColumnCount(1); // 仅显示 group_name 列
	ui->tableWidget->setHorizontalHeaderLabels({ "Group Name" });

	for (int i = 0; i < data.size(); ++i) {
		QJsonObject item = data[i].toObject();
		QString groupName = item.value("group_name").toString();
		ui->tableWidget->setItem(i, 0, new QTableWidgetItem(groupName));
	}
}