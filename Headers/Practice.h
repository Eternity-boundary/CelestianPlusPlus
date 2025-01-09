// Created by Eternity_boundary on Jan 4, 2025
#pragma once
#include <QDialog>
#include <QTextEdit>
#include <QNetworkAccessManager>
#include <QTimer>
#define PRACTICE_TIMER_INTERVAL 65000

class Practice : public QDialog
{
	Q_OBJECT

public:
	explicit Practice(QWidget* parent = nullptr);

	// 禁用复制构造函数和赋值运算符
	Practice(const Practice&) = delete;
	Practice& operator=(const Practice&) = delete;

public slots:
	void appendLogMessage(const QString& message);  // 用于主窗口发送日志数据到日志窗口

private slots:
	void sendGroupMessage();

private:
	QTextEdit* logOutput;
	QNetworkAccessManager* networkManager;
	QTimer* timer;  // 定时器，用于循环发送消息
};
