/* Nordstjernen — Qt tabbed browser window with one renderer process per tab.
 * Copyright 2026 Andreas Røsdal
 * SPDX-License-Identifier: LicenseRef-NSL-1.0
 */

#ifndef NORDSTJERNEN_QT_PROCWINDOW_H
#define NORDSTJERNEN_QT_PROCWINDOW_H

#include <QMainWindow>

class ProcView;
class QLineEdit;
class QTabWidget;
class QAction;
class QDialog;
class QTableWidget;

class ProcWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit ProcWindow(QWidget *parent = nullptr);

    ProcView *addTab(const QString &url, bool foreground = true);

private slots:
    void onAddressEntered();
    void onBack();
    void onForward();
    void onReload();
    void onHome();
    void onNewTab();
    void onCloseTab(int index);
    void onCurrentChanged(int index);
    void onAbout();
    void onTaskManager();

private:
    ProcView *currentView() const;
    ProcView *viewAt(int index) const;
    void connectView(ProcView *view);
    void updateChrome();
    void updateNavActions();
    void refreshTaskManager();
    QString normalizeUrl(const QString &input) const;

    QTabWidget *m_tabs = nullptr;
    QLineEdit *m_address = nullptr;
    QAction *m_backAction = nullptr;
    QAction *m_forwardAction = nullptr;
    QAction *m_reloadAction = nullptr;
    QAction *m_homeAction = nullptr;
    QDialog *m_taskMgr = nullptr;
    QTableWidget *m_taskTable = nullptr;
};

#endif
