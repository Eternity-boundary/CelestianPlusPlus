// Created by Eternity_boundary on Jan 5, 2025
#pragma once

#include <QString>

namespace {
	QNetworkAccessManager* getNetworkManager() {
		static QNetworkAccessManager manager;
		return &manager;
	}
}

class JsonRequestHandler
{
public:
	static void sendJsonRequest(const bool sendToWhere, const QString& textContent);
};
