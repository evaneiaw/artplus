#pragma once

#include <QDialog>

class QDoubleSpinBox;
class QSpinBox;
class QLabel;
class QPushButton;
class QCheckBox;
class QComboBox;

namespace artplus::ui {

// Shaper dialog - ArtPro+ v24 style (Envelope/Envelope Distort)
class ShaperDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ShaperDialog(QWidget* parent = nullptr);

    struct Params {
        QString shapeType = "Arc"; // Arc, Flag, Wave, Fish, Rise, Fish Eye, Inflate, Squeeze, Twist
        double bend = 0.0; // %
        double horizontalDistortion = 0.0; // %
        double verticalDistortion = 0.0; // %
        bool preview = true;
    };

    void setParams(const Params& params);
    Params params() const;

private:
    void buildUi();
    void onShapeClicked();
    void onCancelClicked();

    QComboBox* m_shapeType = nullptr;
    QDoubleSpinBox* m_bend = nullptr;
    QDoubleSpinBox* m_horizontalDistortion = nullptr;
    QDoubleSpinBox* m_verticalDistortion = nullptr;
    QCheckBox* m_preview = nullptr;
    QPushButton* m_shapeButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
};

} // namespace artplus::ui