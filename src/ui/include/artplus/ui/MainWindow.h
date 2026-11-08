#pragma once

#include "artplus/edit/Transform.h"

#include <QMainWindow>
#include <QSizeF>
#include <QString>
#include <QVector>

#include <memory>

class QAction;
class QComboBox;
class QDoubleSpinBox;
class QDockWidget;
class QLabel;
class QListWidget;
class QPushButton;
class QSpinBox;
class QTimer;

namespace artplus::document { class ArtDocument; }
class QTreeWidget;

namespace artplus::pdf { class PdfDocument; struct PdfSnapshot; class PdfEditor; }
namespace artplus::edit { class CommandStack; class Command; }

namespace artplus::ui {

class PageView;
class CollapsiblePanel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    bool openDocument(const QString& path);
    bool saveCurrent();

    // Read-only state accessors (used by the GUI integration test).
    bool hasDocument() const;
    int pageCount() const;
    int currentPageIndex() const;
    double zoom() const;

    // View rotation is purely visual: `userRotation` is the extra rotation
    // applied on top of the page's own /Rotate, and `displayedPageSizeOf`
    // returns the page box as shown (width/height swapped for 90/270).
    // Neither of them changes the PDF itself.
    int userRotation() const { return m_userRotation; }
    QSizeF displayedPageSizeOf(int pageIndex) const { return displayedPageSize(pageIndex); }

    // Coordinate mapping between the PDF page space and the displayed (rotated)
    // page space. Object geometry is always stored unrotated; clicks and
    // highlights are expressed in the displayed space, so every consumer must
    // go through these helpers.
    int totalViewRotation() const;
    QRectF pdfRectToDisplayed(const QRectF& rect) const;
    QPointF displayedPointToPdf(const QPointF& point) const;

signals:
    void documentChanged();

private slots:
    void onNoDocument();
    void previousPage();
    void nextPage();
    void goToPage(int index);
    void fitPage();
    void fitWidth();
    void actualSize();
    void zoomIn();
    void zoomOut();
    void rotateViewClockwise();
    void closeDocument();
    void onRenderTimer();
    void onPageListChanged(int row);
    void onTransformAgain();

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    void buildMenus();
    void buildToolStrip();
    void buildLeftDock();
    void buildRightDock();
    void saveAsCopy();
    void saveEditedAs();
    void undoEdit();
    void redoEdit();
    bool pushCommand(std::unique_ptr<artplus::edit::Command> command);
    void refreshProperties();
    void applyPropertyEdit();
    void selectObjectAt(const QPointF& pagePoint);
    void selectNextObject();
    void selectPreviousObject();
    // Applies one selection set to the object list, the viewport overlay and
    // the dependent panels in a single pass.
    void selectObjectRows(const QVector<int>& rows);
    void selectAllObjects();
    void selectInverseObjects();
    void selectSameFillColor();
    void selectSameStrokeColor();
    void onRunPreflight();
    void showTransformDialog();
    void showScreeningDialog();
    void showTrappingDialog();
    void showStepAndRepeatDialog();
    void zoomToSelection();
    void buildObjectPanel();
    void rebuildEditor();
    void discardEditor();
    void refreshObjectList();
    void onObjectSelectionChanged();
    void nudgeSelection(double dx, double dy);
    void updateSelectionOverlay();
    void updateUi();
    void updateStatus();

    void exportPdf();
    void exportPng();
    void exportJpg();

    void deleteSelectedObjects();
    void copySelectedObjects();
    void pasteObjects();
    void groupSelected();
    void ungroupSelected();
    void lockSelectedObjects(bool lock);
    void showSelectedObjects(bool show);
    void toggleOutlineMode();
    void toggleColorManagedPreview();
    void cycleInspector(int direction);
    void goToPageDialog();
    void updatePanels();
    void renderCurrentPage();
    void scheduleRender();
    void showError(const QString& title, const QString& message);
    void resetDocumentState();
    QSizeF displayedPageSize(int pageIndex) const;
    void applyPageGeometry();
    void populatePageList();
    void updateDocumentInfo();
    void updateWindowTitle();
    void onCursorMoved(const QPointF& pagePoint);

    void showRecolorDialog();
    void showShaperDialog();
    void showWrapDialog();

    // Layer operations
    void addLayer();
    void removeLayer();
    void reorderLayer(bool up);
    void renameLayer(const QString& name);
    void toggleLayerVisibility();
    void toggleLayerLock();
    void toggleLayerPrint();
    void refreshLayersPanel();

    // Separation operations
    void addSeparation();
    void removeSeparation();

    // Units: 0 = mm, 1 = pt, 2 = inch.
    double toDisplayUnits(double points) const;
    QString unitSuffix() const;

    std::unique_ptr<artplus::pdf::PdfDocument> m_doc;
    std::unique_ptr<artplus::document::ArtDocument> m_artDoc;
    std::unique_ptr<artplus::pdf::PdfSnapshot> m_snapshot;
    std::unique_ptr<artplus::pdf::PdfEditor> m_editor;
    std::unique_ptr<artplus::edit::CommandStack> m_history;
    PageView* m_view = nullptr;

    QDockWidget* m_messagesDock = nullptr;
    QDockWidget* m_panelsDock = nullptr;

    CollapsiblePanel* m_messagesPanel = nullptr;
    QTreeWidget* m_messageTree = nullptr;

    CollapsiblePanel* m_documentPanel = nullptr;
    CollapsiblePanel* m_layersPanel = nullptr;
    CollapsiblePanel* m_separationsPanel = nullptr;
    CollapsiblePanel* m_qualityPanel = nullptr;

    QLabel* m_layerName = nullptr;
    QLabel* m_layerInfo = nullptr;
    QListWidget* m_objectList = nullptr;
    CollapsiblePanel* m_objectPanel = nullptr;
    QLabel* m_propType = nullptr;
    QLabel* m_propId = nullptr;
    QLabel* m_propBounds = nullptr;
    QLabel* m_propState = nullptr;
    QLabel* m_propText = nullptr;
    QLabel* m_propFont = nullptr;
    QLabel* m_propImage = nullptr;
    QDoubleSpinBox* m_propX = nullptr;
    QDoubleSpinBox* m_propY = nullptr;
    QDoubleSpinBox* m_propW = nullptr;
    QDoubleSpinBox* m_propH = nullptr;
    QDoubleSpinBox* m_propRotation = nullptr;
    QAction* m_undoAction = nullptr;
    QAction* m_redoAction = nullptr;
    QAction* m_nextObjectAction = nullptr;
    QAction* m_prevObjectAction = nullptr;
    QAction* m_transformAction = nullptr;
    QAction* m_transformAgainAction = nullptr;
    QAction* m_zoomSelectionAction = nullptr;
    QAction* m_selectAllAction = nullptr;
    QAction* m_selectInverseAction = nullptr;
    QAction* m_selectSameFillAction = nullptr;
    QAction* m_selectSameStrokeAction = nullptr;
    // Quality Control panel widgets.
    QPushButton* m_runPreflightButton = nullptr;
    QListWidget* m_preflightResults = nullptr;
    bool m_updatingProperties = false;
    QAction* m_saveEditedAction = nullptr;
    QAction* m_cutAction = nullptr;
    QAction* m_copyAction = nullptr;
    QAction* m_pasteAction = nullptr;
    QAction* m_deleteAction = nullptr;
    QAction* m_groupAction = nullptr;
    QAction* m_ungroupAction = nullptr;
    QAction* m_lockAction = nullptr;
    QAction* m_unlockAction = nullptr;
    QAction* m_hideAction = nullptr;
    QAction* m_showAction = nullptr;
    QAction* m_screeningAction = nullptr;
    QAction* m_trappingAction = nullptr;
    QAction* m_stepRepeatAction = nullptr;
    QAction* m_preflightAction = nullptr;
    QAction* m_updateTrapLayerAction = nullptr;
    QAction* m_recolorAction = nullptr;
    QAction* m_shaperAction = nullptr;
    QAction* m_wrapAction = nullptr;
    QAction* m_moveLeft = nullptr;
    QAction* m_moveRight = nullptr;
    QAction* m_moveUp = nullptr;
    QAction* m_moveDown = nullptr;

    // Layer panel tool buttons
    QToolButton* m_addLayerButton = nullptr;
    QToolButton* m_removeLayerButton = nullptr;
    QToolButton* m_layerOrderButton = nullptr;
    QToolButton* m_layerOptionsButton = nullptr;

    // Separation panel tool buttons
    QToolButton* m_addSeparationButton = nullptr;
    QToolButton* m_removeSeparationButton = nullptr;
    QToolButton* m_separationOptionsButton = nullptr;

    int m_selectedObject = -1;
    QVector<int> m_selectedObjects;
    artplus::edit::TransformParams m_transformDefaults;
    artplus::edit::TransformAnchor m_transformAnchor =
        artplus::edit::TransformAnchor::TopLeft;
    QLabel* m_profileValue = nullptr;
    QLabel* m_pageSizeValue = nullptr;
    QSpinBox* m_tacLimit = nullptr;

    // Clipboard for copy/paste objects
    QVector<artplus::pdf::EditableObject> m_clipboardObjects;

    // Document panel metadata labels
    QLabel* m_pageCountLabel = nullptr;
    QLabel* m_pageSizeLabel = nullptr;
    QLabel* m_orientationLabel = nullptr;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_authorLabel = nullptr;
    QLabel* m_creatorLabel = nullptr;
    QLabel* m_producerLabel = nullptr;
    QLabel* m_creationDateLabel = nullptr;
    QLabel* m_modificationDateLabel = nullptr;
    QLabel* m_outputIntentLabel = nullptr;

    QLabel* m_docLabel = nullptr;
    QLabel* m_cursorLabel = nullptr;
    QLabel* m_pageInfoLabel = nullptr;
    QLabel* m_zoomLabel = nullptr;
    QComboBox* m_unitsCombo = nullptr;
    QSpinBox* m_pageSpin = nullptr;
    QTimer* m_renderTimer = nullptr;

    QAction* m_showBoxesAction = nullptr;
    QAction* m_crosshairAction = nullptr;
    QAction* m_outlineAction = nullptr;
    QAction* m_colorManagedPreviewAction = nullptr;
    QAction* m_nextInspectorAction = nullptr;
    QAction* m_prevInspectorAction = nullptr;

    // Drag state for object dragging and marquee selection
    QPointF m_dragStartPagePos;
    bool m_isDraggingObject = false;

    int m_page = 0;
    int m_userRotation = 0;
    int m_units = 0;
    bool m_suppressSpinSignal = false;
};

} // namespace artplus::ui
