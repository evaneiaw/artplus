#pragma once

#include <QDialog>

class QDoubleSpinBox;
class QSpinBox;
class QLabel;
class QPushButton;
class QCheckBox;
class QComboBox;

namespace artplus::ui {

// Trapping dialog - ArtPro+ v24 style
class TrappingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TrappingDialog(QWidget* parent = nullptr);

    struct Params {
        double trapWidth = 0.25; // mm
        double maxTrapWidth = 1.0; // mm
        bool blackLimit = true;
        double blackLimitValue = 30.0; // %
        bool richBlackLimit = true;
        double richBlackLimitValue = 50.0; // %
        bool slidingTrap = false;
        double slidingTrapValue = 50.0; // %
        bool overprintBlack = true;
        bool trapToRichBlack = false;
        bool joinStyle = true; // true = round, false = miter
        double miterLimit = 4.0;
    };

    void setParams(const Params& params);
    Params params() const;

private:
    void buildUi();
    void onTrapClicked();
    void onCancelClicked();

    QDoubleSpinBox* m_trapWidth = nullptr;
    QDoubleSpinBox* m_maxTrapWidth = nullptr;
    QCheckBox* m_blackLimit = nullptr;
    QDoubleSpinBox* m_blackLimitValue = nullptr;
    QCheckBox* m_richBlackLimit = nullptr;
    QDoubleSpinBox* m_richBlackLimitValue = nullptr;
    QCheckBox* m_slidingTrap = nullptr;
    QDoubleSpinBox* m_slidingTrapValue = nullptr;
    QCheckBox* m_overprintBlack = nullptr;
    QCheckBox* m_trapToRichBlack = nullptr;
    QCheckBox* m_joinStyleRound = nullptr; // round vs miter
    QDoubleSpinBox* m_miterLimit = nullptr;
    QPushButton* m_trapButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
};

} // namespace artplus::ui