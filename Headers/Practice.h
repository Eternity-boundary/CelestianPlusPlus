#pragma once
#include <QDialog>
#include <QTextEdit>
#include <QNetworkAccessManager>
#define PRACTICE_TIMER_INTERVAL 65000

class Practice : public QDialog
{
	Q_OBJECT

public:
	explicit Practice(QWidget* parent = nullptr);

public slots:
	void appendLogMessage(const QString& message);  // 用于主窗口发送日志数据到日志窗口

private slots:
	void sendGroupMessage();

private:
	QTextEdit* logOutput;
	QNetworkAccessManager* networkManager;

	QString decodeHexEscapes(const QString& input);

	QString formatRawMessage(const QString& rawMessage);  // 格式化 raw_message，移除 CQ 码

	QTimer* timer;  // 定时器，用于循环发送消息
};
