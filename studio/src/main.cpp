//
// This file is part of the Aulos toolkit.
// Copyright (C) 2020 Sergei Blagodarin.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "studio.hpp"

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
	QCoreApplication::setOrganizationDomain("blagodarin.me");
	QCoreApplication::setOrganizationName("blagodarin.me");
	QApplication::setWindowIcon(QIcon{ ":/aulos.png" });
	Studio studio;
	studio.show();
	return app.exec();
}
