#pragma once

#include "artplus/edit/Transform.h"

#include <QDialog>

class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QSpinBox;

namespace artplus::ui {

// Faithful reproduction of the ArtPro+ v24 Object > Transform… dialog.
//
// The layout uses absolute dialog-relative geometry measured from the live
// v24 dialog (700x248 client area). Defaults, Alt semantics and the control
// wording follow the same capture.
class TransformDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TransformDialog(QWidget* parent = nullptr);

    // Values shown when the dialog opens (last accepted values of the
    // document) and anchor selection.
    void setInitial(const artplus::edit::TransformParams& params,
                    artplus::edit::TransformAnchor anchor);

    artplus::edit::TransformParams params() const;
    artplus::edit::TransformAnchor anchor() const;
    bool copyRequested() const { return m_copyRequested; }

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void buildUi();
    void resetToDefaults();
    void onTransformClicked();
    void onCancelClicked();
    void onLinkToggled(bool checked);
    void syncLinkedScale(QDoubleSpinBox* changed);

    QDoubleSpinBox* m_moveX = nullptr;
    QDoubleSpinBox* m_moveY = nullptr;
    QDoubleSpinBox* m_scaleX = nullptr;
    QDoubleSpinBox* m_scaleY = nullptr;
    QDoubleSpinBox* m_rotate = nullptr;
    QDoubleSpinBox* m_shear = nullptr;
    QSpinBox* m_repetitions = nullptr;
    QWidget* m_anchor = nullptr;
    QWidget* m_flipHorizontal = nullptr;
    QWidget* m_flipVertical = nullptr;
    QWidget* m_link = nullptr;
    QWidget* m_gradients = nullptr;
    QWidget* m_strokes = nullptr;
    QWidget* m_masks = nullptr;
    QWidget* m_transformEach = nullptr;
    QLabel* m_includeLabel = nullptr;
    QLabel* m_transformLabel = nullptr;
    QLabel* m_copiesLabel = nullptr;
    QLabel* m_hintText = nullptr;
    QWidget* m_hintIcon = nullptr;
    QPushButton* m_transformButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
    bool m_copyRequested = false;
    bool m_syncingScale = false;
};

} // namespace artplus::ui
