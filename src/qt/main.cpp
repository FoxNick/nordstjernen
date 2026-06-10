/* Nordstjernen — Qt frontend application entry point.
 * Copyright 2026 Andreas Røsdal
 * SPDX-License-Identifier: LicenseRef-NSL-1.0
 */

#include "procwindow.h"

#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Nordstjernen"));
    QApplication::setApplicationDisplayName(
        QStringLiteral("Nordstjernen (Qt)"));

    const QStringList args = QApplication::arguments();
    const QString url =
        args.size() > 1 ? args.at(1) : QStringLiteral("about:start");

    ProcWindow window;
    window.show();
    window.addTab(url);
    return app.exec();
}
