#pragma once

#include <QDialog>

class QDoubleSpinBox;
class QSpinBox;
class QLabel;
class QPushButton;
class QCheckBox;
class QComboBox;

namespace artplus::ui {

// Wrap dialog - ArtPro+ v24 style (Text Wrap / Object Wrap)
class WrapDialog : public QDialog
{
    Q_OBJECT

public:
    explicit WrapDialog(QWidget* parent = nullptr);

    struct Params {
        QString wrapType = "Bounding Box"; // Bounding Box, Object Shape, Jump Object, Jump to Next Column
        double offset = 6.0; // pt
        bool invert = false;
        bool wrapInside = false;
        bool applyToAll = false;
    };

    void setParams(const Params& params);
    Params params() const;

private:
    void buildUi();
    void onWrapClicked();
    void onCancelClicked();

    QComboBox* m_wrapType = nullptr;
    QDoubleSpinBox* m_offset = nullptr;
    QCheckBox* m_invert = nullptr;
    QCheckBox* m_wrapInside = nullptr;
    QCheckBox* m_applyToAll = nullptr;
    QPushButton* m_wrapButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
};

} // namespace artplus::ui