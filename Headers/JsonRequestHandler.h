// Created by Eternity_boundary on Jan 5, 2025
#pragma once

#include <QString>

namespace {
	std::unique_ptr<QNetworkAccessManager> networkManager = std::make_unique<QNetworkAccessManager>();
}

class JsonRequestHandler
{
public:
	static void sendJsonRequest(const bool sendToWhere, const QString& textContent);
};
