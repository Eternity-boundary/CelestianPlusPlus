// Created by Eternity_boundary on Jan 4,2025
#pragma once
#include <QString>
#include <QRegularExpression>
#include <QUrl>
#include <QTextDocumentFragment>

class LogProcessor
{
public:
	// 处理日志消息的静态方法
	static QString processLogMessage(const QString& rawMessage, qint64 currentUserId)
	{
		QString message = rawMessage;

		// 1. URL 解码
		message = QUrl::fromPercentEncoding(message.toUtf8());

		// 2. 替换十六进制转义字符
		message = decodeHexEscapes(message);

		// 3. HTML 实体解码
		message = QTextDocumentFragment::fromHtml(message).toPlainText();

		// 4. 移除所有 CQ 码（包括 CQ:markdown、CQ:at 等）
		message.replace(QRegularExpression(R"(\[CQ:[^\]]*\])"), "");

		// 5. 移除 mqqapi:// 链接
		message.replace(QRegularExpression(R"(mqqapi://[^\s\]]+)"), "");

		// 6. 移除类似 ]@xxxxxxxx 的冗余标记
		message.replace(QRegularExpression(R"(\]@[\w\\_]+)"), "");

		// 7. 移除类似 {"version":2} 的 JSON 格式干扰
		message.replace(QRegularExpression(R"(\{.*?\})"), "");

		// 8. 移除多余的括号与标点符号
		message.replace(QRegularExpression(R"([\(\)\[\]])"), "");

		// 9. 移除类似 @xxxxxxxx 这样的标记
		message.replace(QRegularExpression(R"(@[\w\\_]+)"), "");

		// 10. 移除与当前用户无关的 @ 信息
		QString atPattern = QString(R"(\[CQ:at,qq=%1[^\]]*\])").arg(currentUserId);
		message.remove(QRegularExpression(atPattern));

		// 11. 移除多余空格和换行符
		message = message.simplified();

		return message.trimmed();
	}

private:
	// 解码十六进制转义字符的静态方法
	static QString decodeHexEscapes(const QString& input)
	{
		QString result = input;
		QRegularExpression hexPattern(R"(\\x([0-9A-Fa-f]{2}))");  // 匹配 \xHH 格式的转义字符
		QRegularExpressionMatch match;

		int startIndex = 0;
		while ((startIndex = result.indexOf(hexPattern, startIndex, &match)) != -1) {
			bool ok;
			int hexValue = match.captured(1).toInt(&ok, 16);  // 将捕获的十六进制字符串转换为整数
			if (ok) {
				result.replace(match.capturedStart(0), match.capturedLength(0), QChar(hexValue));  // 替换为对应字符
				startIndex += 1;  // 移动索引避免死循环
			}
			else {
				break;  // 如果转换失败，直接跳出循环
			}
		}

		return result;
	}
};


