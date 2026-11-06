#pragma once
#include <QLabel>
namespace artplus::ui { class StatusBarPanel : public QLabel { Q_OBJECT public: explicit StatusBarPanel(QWidget* p=nullptr) : QLabel(p) { setText("No document"); } }; }
