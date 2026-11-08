#pragma once

#include <QDialog>

class QDoubleSpinBox;
class QSpinBox;
class QLabel;
class QPushButton;
class QCheckBox;
class QComboBox;

namespace artplus::ui {

// Screening dialog - ArtPro+ v24 style
class ScreeningDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ScreeningDialog(QWidget* parent = nullptr);

    struct Params {
        QString screenSet = "Default";
        double frequency = 150.0; // lpi
        double angle = 45.0; // degrees
        double dotShape = 0.0; // 0=round, 1=ellipse, 2=square, etc.
        bool autoFrequency = true;
        bool useAngleOffset = false;
        double angleOffset = 0.0;
        bool accurateScreens = false;
    };

    void setParams(const Params& params);
    Params params() const;

private:
    void buildUi();
    void onScreenClicked();
    void onCancelClicked();

    QComboBox* m_screenSet = nullptr;
    QCheckBox* m_autoFrequency = nullptr;
    QDoubleSpinBox* m_frequency = nullptr;
    QDoubleSpinBox* m_angle = nullptr;
    QDoubleSpinBox* m_dotShape = nullptr;
    QCheckBox* m_useAngleOffset = nullptr;
    QDoubleSpinBox* m_angleOffset = nullptr;
    QCheckBox* m_accurateScreens = nullptr;
    QPushButton* m_screenButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
};

} // namespace artplus::ui