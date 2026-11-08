#pragma once

#include <QDialog>

class QDoubleSpinBox;
class QSpinBox;
class QLabel;
class QPushButton;
class QCheckBox;
class QComboBox;

namespace artplus::ui {

// Step & Repeat dialog - ArtPro+ v24 style
class StepAndRepeatDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StepAndRepeatDialog(QWidget* parent = nullptr);

    struct Params {
        int copiesX = 1;
        int copiesY = 1;
        double offsetX = 0.0;
        double offsetY = 0.0;
        double gapX = 0.0;
        double gapY = 0.0;
        bool stagger = false;
        double staggerOffset = 0.0;
        bool continuous = false;
        bool copyObjects = true;
    };

    void setParams(const Params& params);
    Params params() const;

private:
    void buildUi();
    void onStepRepeatClicked();
    void onCancelClicked();

    QSpinBox* m_copiesX = nullptr;
    QSpinBox* m_copiesY = nullptr;
    QDoubleSpinBox* m_offsetX = nullptr;
    QDoubleSpinBox* m_offsetY = nullptr;
    QDoubleSpinBox* m_gapX = nullptr;
    QDoubleSpinBox* m_gapY = nullptr;
    QCheckBox* m_stagger = nullptr;
    QDoubleSpinBox* m_staggerOffset = nullptr;
    QCheckBox* m_continuous = nullptr;
    QCheckBox* m_copyObjects = nullptr;
    QPushButton* m_stepRepeatButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
};

} // namespace artplus::ui