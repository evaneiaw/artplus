#pragma once

#include <QDialog>

class QDoubleSpinBox;
class QSpinBox;
class QLabel;
class QPushButton;
class QCheckBox;
class QComboBox;

namespace artplus::ui {

// Recolor dialog - ArtPro+ v24 style
class RecolorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RecolorDialog(QWidget* parent = nullptr);

    struct Params {
        QString colorSpace = "CMYK";
        QString targetColor = "Cyan";
        double tolerance = 10.0; // %
        bool preserveBlack = true;
        bool preserveWhite = true;
        bool useICC = false;
    };

    void setParams(const Params& params);
    Params params() const;

private:
    void buildUi();
    void onRecolorClicked();
    void onCancelClicked();

    QComboBox* m_colorSpace = nullptr;
    QComboBox* m_targetColor = nullptr;
    QDoubleSpinBox* m_tolerance = nullptr;
    QCheckBox* m_preserveBlack = nullptr;
    QCheckBox* m_preserveWhite = nullptr;
    QCheckBox* m_useICC = nullptr;
    QPushButton* m_recolorButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
};

} // namespace artplus::ui