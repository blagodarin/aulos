// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "studio.hpp"

#include <aulos_config.h>

#include <cassert>

#include <QApplication>

namespace
{
	QtMessageHandler messageHandler;
}

int main(int argc, char** argv)
{
	::messageHandler = qInstallMessageHandler([](QtMsgType type, const QMessageLogContext& context, const QString& message) {
		assert(type == QtDebugMsg || type == QtInfoMsg
			|| message.startsWith("QWindowsWindow::setGeometry: Unable to set geometry ") // A window was too small to contain all its widgets, so its size was increased.
		);
		::messageHandler(type, context, message);
	});
	QApplication app{ argc, argv };
	QCoreApplication::setApplicationName("Aulos Studio");
	QCoreApplication::setApplicationVersion(AULOS_VERSION);
	QCoreApplication::setOrganizationDomain("blagodarin.me");
	QCoreApplication::setOrganizationName("blagodarin.me");
	QApplication::setWindowIcon(QIcon{ ":/aulos.png" });
	Studio studio;
	studio.show();
	return app.exec();
}
