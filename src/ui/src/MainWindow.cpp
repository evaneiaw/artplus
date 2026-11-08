#include "artplus/ui/MainWindow.h"

#include "artplus/core/Logger.h"
#include "artplus/pdf/PdfDocument.h"
#include "artplus/pdf/PdfEditor.h"
#include "artplus/edit/Command.h"
#include "artplus/edit/Properties.h"
#include "artplus/pdf/PdfSnapshot.h"
#include "artplus/preflight/Preflight.h"
#include "artplus/ui/Panel.h"
#include "artplus/ui/PageView.h"
#include "artplus/ui/Theme.h"
#include "artplus/ui/TransformDialog.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QDockWidget>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QCryptographicHash>
#include <QFileDialog>
#include <QGridLayout>
#include <QFileInfo>
#include <QFontMetrics>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QScrollArea>
#include <QPushButton>
#include <QPainter>
#include <QScreen>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <cmath>
#include <vector>

namespace artplus::ui {

namespace {

constexpr int kMaxRenderPixels = 8000;

int normalizeRotation(int degrees)
{
    int r = degrees % 360;
    if (r < 0)
        r += 360;
    return ((r + 45) / 90 * 90) % 360;
}

QLabel* makeLabel(const QString& text, bool bold = false, int pointSize = 8,
                  const QColor& color = theme::kText)
{
    auto* label = new QLabel(text);
    label->setFont(theme::uiFont(pointSize, bold));
    QPalette pal = label->palette();
    pal.setColor(QPalette::WindowText, color);
    label->setPalette(pal);
    return label;
}

QWidget* makeIconLabel(theme::IconKind kind, int size = 14, const QColor& color = theme::kText)
{
    auto* label = new QLabel();
    label->setPixmap(theme::icon(kind, color, size).pixmap(size, size));
    label->setFixedSize(size, size);
    return label;
}

QToolButton* makeFlatButton(theme::IconKind kind, const QString& tooltip, int size = 14)
{
    auto* button = new QToolButton();
    button->setAutoRaise(true);
    button->setIcon(theme::icon(kind, theme::kText, size));
    button->setIconSize(QSize(size, size));
    button->setToolTip(tooltip);
    button->setFont(theme::captionFont());
    return button;
}

QWidget* makeRowSeparator()
{
    auto* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet(QStringLiteral("color: #E0E0E0; background: #E0E0E0;"));
    line->setFixedHeight(1);
    return line;
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("ArtPlus"));
    if (QScreen* screen = QGuiApplication::primaryScreen()) {
        const QRect avail = screen->availableGeometry();
        const int w = std::min(1500, avail.width() - 40);
        const int h = std::min(940, avail.height() - 80);
        setGeometry(avail.x() + 20, avail.y() + 20, w, h);
    } else {
        setGeometry(60, 40, 1400, 900);
    }
    setAcceptDrops(true);

    m_view = new PageView(this);
    setCentralWidget(m_view);

    // Docks first: the Window menu needs their toggle actions; the toolbar
    // toggles refer to them through null-checked lambdas.
    buildToolStrip();
    buildLeftDock();
    buildRightDock();
    buildMenus();

    // Status bar -------------------------------------------------------------
    auto* status = new QStatusBar(this);
    status->setSizeGripEnabled(false);
    setStatusBar(status);

    auto* info = new QLabel(this);
    info->setPixmap(theme::icon(theme::IconKind::Info, theme::kText, 14).pixmap(14, 14));
    info->setFixedWidth(18);
    status->addWidget(info);

    m_cursorLabel = makeLabel(QStringLiteral("X: —   Y: —"));
    m_cursorLabel->setMinimumWidth(150);
    status->addWidget(m_cursorLabel);

    auto* spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    status->addWidget(spacer, 1);

    m_pageSpin = new QSpinBox(this);
    m_pageSpin->setRange(1, 1);
    m_pageSpin->setFixedWidth(56);
    m_pageSpin->setFont(theme::uiFont(8));
    status->addPermanentWidget(makeLabel(QStringLiteral("Page")));
    status->addPermanentWidget(m_pageSpin);
    m_pageInfoLabel = makeLabel(QStringLiteral("0 of 0"));
    status->addPermanentWidget(m_pageInfoLabel);

    m_unitsCombo = new QComboBox(this);
    m_unitsCombo->addItems({QStringLiteral("mm"), QStringLiteral("pt"), QStringLiteral("in")});
    m_unitsCombo->setFont(theme::uiFont(8));
    m_unitsCombo->setFixedWidth(56);
    status->addPermanentWidget(m_unitsCombo);

    m_zoomLabel = makeLabel(QStringLiteral("100,0%"));
    m_zoomLabel->setMinimumWidth(64);
    m_zoomLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    status->addPermanentWidget(m_zoomLabel);

    connect(m_pageSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &MainWindow::goToPage);
    connect(m_unitsCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index) {
                m_units = index;
                updateStatus();
            });

    m_view->setStatusCallback([this]() { updateStatus(); });
    m_view->setRenderRequestCallback([this]() { scheduleRender(); });
    m_view->setCursorCallback([this](const QPointF& p) { onCursorMoved(p); });
    m_view->setClickCallback([this](const QPointF& p) { selectObjectAt(p); });
    m_view->setHitTestCallback([this](const QPointF& pagePos) -> objects::ObjectId {
        if (!m_editor)
            return 0;
        for (int i = m_editor->objectCount() - 1; i >= 0; --i) {
            const auto* object = m_editor->object(i);
            if (object && object->bounds.contains(pagePos))
                return static_cast<objects::ObjectId>(object->index);
        }
        return 0;
    });

    // Object drag callback: translate selected objects
    m_view->setObjectDragCallback([this](const QPointF& currentPagePos) {
        if (!m_editor || m_selectedObjects.isEmpty())
            return;
        if (!m_isDraggingObject) {
            // First call - store the start position
            m_dragStartPagePos = currentPagePos;
            m_isDraggingObject = true;
            return;
        }
        // Calculate delta from start position
        double dx = currentPagePos.x() - m_dragStartPagePos.x();
        double dy = currentPagePos.y() - m_dragStartPagePos.y();
        
        // Translate all selected objects
        for (int index : m_selectedObjects) {
            if (index >= 0 && index < m_editor->objectCount()) {
                m_editor->translateObject(index, dx, dy);
            }
        }
        
        // Update drag start position for next incremental move
        m_dragStartPagePos = currentPagePos;
        
        // Update UI and schedule render
        updateSelectionOverlay();
        scheduleRender();
    });

    // Marquee select callback: select objects within rectangle
    m_view->setMarqueeSelectCallback([this](const QRectF& marqueeRect) {
        if (!m_editor || !m_objectList)
            return;
        
        const Qt::KeyboardModifiers mods = QApplication::keyboardModifiers();
        bool shiftDown = mods.testFlag(Qt::ShiftModifier);
        bool ctrlDown = mods.testFlag(Qt::ControlModifier);
        
        QVector<int> newSelection;
        for (int i = 0; i < m_editor->objectCount(); ++i) {
            const auto* object = m_editor->object(i);
            if (object && marqueeRect.intersects(object->bounds)) {
                newSelection.append(i);
            }
        }
        
        if (ctrlDown) {
            // Toggle selection
            for (int idx : newSelection) {
                if (m_selectedObjects.contains(idx))
                    m_selectedObjects.erase(std::find(m_selectedObjects.begin(), m_selectedObjects.end(), idx));
                else
                    m_selectedObjects.append(idx);
            }
            std::sort(m_selectedObjects.begin(), m_selectedObjects.end());
            m_selectedObjects.erase(std::unique(m_selectedObjects.begin(), m_selectedObjects.end()), m_selectedObjects.end());
        } else if (shiftDown && !m_selectedObjects.isEmpty()) {
            // Extend selection range
            int first = qMin(newSelection.first(), m_selectedObjects.first());
            int last = qMax(newSelection.last(), m_selectedObjects.last());
            m_selectedObjects.clear();
            for (int i = first; i <= last; ++i)
                m_selectedObjects.append(i);
        } else {
            // Replace selection
            m_selectedObjects = newSelection;
            std::sort(m_selectedObjects.begin(), m_selectedObjects.end());
        }
        
        m_selectedObject = m_selectedObjects.isEmpty() ? -1 : m_selectedObjects.first();
        m_objectList->setCurrentRow(m_selectedObject);
        updateSelectionOverlay();
        onObjectSelectionChanged();
    });

    // Object drag end callback: reset drag state
    m_view->setObjectDragEndCallback([this]() {
        m_isDraggingObject = false;
    });

    m_renderTimer = new QTimer(this);
    m_renderTimer->setSingleShot(true);
    m_renderTimer->setInterval(120);
    connect(m_renderTimer, &QTimer::timeout, this, &MainWindow::onRenderTimer);

    updateUi();
    updateWindowTitle();
    logInfo(QStringLiteral("MainWindow constructed"));
}

MainWindow::~MainWindow() = default;

// ---------------------------------------------------------------------------
// Menus
// ---------------------------------------------------------------------------

void MainWindow::buildMenus()
{
    auto addDisabled = [](QMenu* menu, const QString& text,
                          const QKeySequence& shortcut = QKeySequence()) {
        QAction* action = menu->addAction(text);
        action->setEnabled(false);
        if (!shortcut.isEmpty())
            action->setShortcut(shortcut);
        return action;
    };
    auto addTodoSubmenu = [](QMenu* menu, const QString& title) {
        QMenu* submenu = menu->addMenu(title);
        QAction* action = submenu->addAction(QStringLiteral("Not implemented in ArtPlus yet"));
        action->setEnabled(false);
        return submenu;
    };

    // File ------------------------------------------------------------------
    auto* fileMenu = menuBar()->addMenu(QStringLiteral("File"));
    addDisabled(fileMenu, QStringLiteral("New"), QKeySequence::New);
    addDisabled(fileMenu, QStringLiteral("New Report Template..."));
    auto* open = fileMenu->addAction(QStringLiteral("Open..."));
    open->setShortcut(QKeySequence::Open);
    connect(open, &QAction::triggered, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(
            this, QStringLiteral("Open PDF"), QString(),
            QStringLiteral("PDF Files (*.pdf);;All Files (*.*)"));
        if (!path.isEmpty())
            openDocument(path);
    });
    addTodoSubmenu(fileMenu, QStringLiteral("Open Recent"));
    fileMenu->addSeparator();
    auto* close = fileMenu->addAction(QStringLiteral("Close"));
    close->setShortcut(QKeySequence(QStringLiteral("Ctrl+W")));
    connect(close, &QAction::triggered, this, &MainWindow::closeDocument);
    addDisabled(fileMenu, QStringLiteral("Close All"),
                QKeySequence(QStringLiteral("Ctrl+Alt+W")));
    auto* save = fileMenu->addAction(QStringLiteral("Save"));
    save->setShortcut(QKeySequence::Save);
    connect(save, &QAction::triggered, this, [this]() { saveCurrent(); });
    m_saveEditedAction = fileMenu->addAction(QStringLiteral("Save As..."));
    m_saveEditedAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+S")));
    m_saveEditedAction->setEnabled(false);
    connect(m_saveEditedAction, &QAction::triggered, this, &MainWindow::saveEditedAs);
    fileMenu->addSeparator();
    addTodoSubmenu(fileMenu, QStringLiteral("Export To"));
    fileMenu->addSeparator();
    addDisabled(fileMenu, QStringLiteral("Convert To PDF+"));
    addDisabled(fileMenu, QStringLiteral("Convert To Report Template..."));
    addDisabled(fileMenu, QStringLiteral("Convert To Variable Data Template..."));
    fileMenu->addSeparator();
    addTodoSubmenu(fileMenu, QStringLiteral("Action Lists"));
    addDisabled(fileMenu, QStringLiteral("Launch Workflow..."),
                QKeySequence(QStringLiteral("Ctrl+L")));
    fileMenu->addSeparator();
    addDisabled(fileMenu, QStringLiteral("Insert Content From PDF..."));
    addDisabled(fileMenu, QStringLiteral("Place File..."),
                QKeySequence(QStringLiteral("Ctrl+Shift+P")));
    addDisabled(fileMenu, QStringLiteral("Place Image Inside..."),
                QKeySequence(QStringLiteral("Ctrl+Alt+Shift+P")));
    fileMenu->addSeparator();
    addTodoSubmenu(fileMenu, QStringLiteral("Pages"));
    fileMenu->addSeparator();
    addDisabled(fileMenu, QStringLiteral("Page Boxes Setup..."),
                QKeySequence(QStringLiteral("Ctrl+Alt+P")));
    addDisabled(fileMenu, QStringLiteral("Document Setup..."));
    fileMenu->addSeparator();
    addDisabled(fileMenu, QStringLiteral("Print..."), QKeySequence::Print);
    fileMenu->addSeparator();
    auto* quit = fileMenu->addAction(QStringLiteral("Exit"));
    quit->setShortcut(QKeySequence(QStringLiteral("Ctrl+Q")));
    connect(quit, &QAction::triggered, this, &QWidget::close);

    // Edit ------------------------------------------------------------------
    auto* editMenu = menuBar()->addMenu(QStringLiteral("Edit"));
    m_undoAction = editMenu->addAction(QStringLiteral("Undo"));
    m_undoAction->setShortcut(QKeySequence::Undo);
    m_undoAction->setEnabled(false);
    connect(m_undoAction, &QAction::triggered, this, &MainWindow::undoEdit);
    m_redoAction = editMenu->addAction(QStringLiteral("Redo"));
    m_redoAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+Z")));
    m_redoAction->setEnabled(false);
    connect(m_redoAction, &QAction::triggered, this, &MainWindow::redoEdit);
    editMenu->addSeparator();
    addDisabled(editMenu, QStringLiteral("Cut"), QKeySequence::Cut);
    addDisabled(editMenu, QStringLiteral("Copy"), QKeySequence::Copy);
    addDisabled(editMenu, QStringLiteral("Paste"), QKeySequence::Paste);
    editMenu->addSeparator();
    addDisabled(editMenu, QStringLiteral("Paste in Front"),
                QKeySequence(QStringLiteral("Ctrl+F")));
    addDisabled(editMenu, QStringLiteral("Paste in Back"),
                QKeySequence(QStringLiteral("Ctrl+Shift+F")));
    addDisabled(editMenu, QStringLiteral("Paste with Layer Structure"),
                QKeySequence(QStringLiteral("Ctrl+Shift+V")));
    addDisabled(editMenu, QStringLiteral("Paste and Match Style"),
                QKeySequence(QStringLiteral("Ctrl+Alt+Shift+V")));
    editMenu->addSeparator();
    addDisabled(editMenu, QStringLiteral("Delete"), QKeySequence(Qt::Key_Backspace));
    editMenu->addSeparator();
    addDisabled(editMenu, QStringLiteral("Options..."),
                QKeySequence(QStringLiteral("Ctrl+K")));

    // Object ----------------------------------------------------------------
    auto* objectMenu = menuBar()->addMenu(QStringLiteral("Object"));
    addTodoSubmenu(objectMenu, QStringLiteral("Arrange"));
    m_transformAction = objectMenu->addAction(QStringLiteral("Transform..."));
    m_transformAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+M")));
    m_transformAction->setEnabled(false);
    connect(m_transformAction, &QAction::triggered, this, &MainWindow::showTransformDialog);

    // Transform Again — repeats the last successful Transform operation on the current selection.
    m_transformAgainAction = objectMenu->addAction(QStringLiteral("Transform Again"));
    m_transformAgainAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+D")));
    m_transformAgainAction->setEnabled(false);
    connect(m_transformAgainAction, &QAction::triggered, this, &MainWindow::onTransformAgain);

    objectMenu->addSeparator();
    // Group/Ungroup/Lock/Hide are ArtPro+ document-model features. The current
    // PDFium-based object model has no group object, no lock flag and no hidden
    // state to mutate, so these stay disabled placeholders instead of faking an
    // action in the UI.
    addDisabled(objectMenu, QStringLiteral("Group"), QKeySequence(QStringLiteral("Ctrl+G")));
    addDisabled(objectMenu, QStringLiteral("Ungroup"),
                QKeySequence(QStringLiteral("Ctrl+Shift+G")));
    addDisabled(objectMenu, QStringLiteral("Lock"), QKeySequence(QStringLiteral("Ctrl+2")));
    addDisabled(objectMenu, QStringLiteral("Unlock All"),
                QKeySequence(QStringLiteral("Ctrl+Alt+2")));
    addDisabled(objectMenu, QStringLiteral("Hide"), QKeySequence(QStringLiteral("Ctrl+3")));
    addDisabled(objectMenu, QStringLiteral("Show All"),
                QKeySequence(QStringLiteral("Ctrl+Alt+3")));
    objectMenu->addSeparator();
    addTodoSubmenu(objectMenu, QStringLiteral("Path"));
    addTodoSubmenu(objectMenu, QStringLiteral("Compound"));
    addTodoSubmenu(objectMenu, QStringLiteral("Clipping Mask"));
    addTodoSubmenu(objectMenu, QStringLiteral("Opacity Mask"));
    addTodoSubmenu(objectMenu, QStringLiteral("Image"));
    addTodoSubmenu(objectMenu, QStringLiteral("Text"));
    addTodoSubmenu(objectMenu, QStringLiteral("Placed Art"));
    objectMenu->addSeparator();
    addTodoSubmenu(objectMenu, QStringLiteral("Remove Color Profile"));
    addDisabled(objectMenu, QStringLiteral("Recolor..."));
    objectMenu->addSeparator();
    addDisabled(objectMenu, QStringLiteral("Shaper..."), QKeySequence(QStringLiteral("Ctrl+E")));
    addDisabled(objectMenu, QStringLiteral("Spread/Choke..."),
                QKeySequence(QStringLiteral("Ctrl+Shift+T")));
    objectMenu->addSeparator();
    addDisabled(objectMenu, QStringLiteral("Expand"));
    addDisabled(objectMenu, QStringLiteral("Rasterize..."));
    addTodoSubmenu(objectMenu, QStringLiteral("Clean"));
    addTodoSubmenu(objectMenu, QStringLiteral("Clip"));
    addTodoSubmenu(objectMenu, QStringLiteral("Fit"));
    objectMenu->addSeparator();
    addDisabled(objectMenu, QStringLiteral("Convert to Placeholder..."));



    // Select ----------------------------------------------------------------
    auto* selectMenu = menuBar()->addMenu(QStringLiteral("Select"));
    m_selectAllAction = selectMenu->addAction(QStringLiteral("Select All"));
    m_selectAllAction->setShortcut(QKeySequence::SelectAll);
    m_selectAllAction->setEnabled(false);
    connect(m_selectAllAction, &QAction::triggered, this, &MainWindow::selectAllObjects);
    auto* deselect = selectMenu->addAction(QStringLiteral("Deselect All"));
    deselect->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+A")));
    connect(deselect, &QAction::triggered, this, [this]() { selectObjectRows({}); });
    m_selectInverseAction = selectMenu->addAction(QStringLiteral("Select Inverse"));
    m_selectInverseAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+Alt+A")));
    m_selectInverseAction->setEnabled(false);
    connect(m_selectInverseAction, &QAction::triggered, this, &MainWindow::selectInverseObjects);
    m_nextObjectAction = selectMenu->addAction(QStringLiteral("Select Next Object"));
    m_nextObjectAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+Tab")));
    m_nextObjectAction->setEnabled(false);
    connect(m_nextObjectAction, &QAction::triggered, this, &MainWindow::selectNextObject);
    m_prevObjectAction = selectMenu->addAction(QStringLiteral("Select Previous Object"));
    m_prevObjectAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+Tab")));
    m_prevObjectAction->setEnabled(false);
    connect(m_prevObjectAction, &QAction::triggered, this, &MainWindow::selectPreviousObject);
    selectMenu->addSeparator();
    m_selectSameFillAction = selectMenu->addAction(QStringLiteral("Select Same Fill Color"));
    m_selectSameFillAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+Alt+I")));
    m_selectSameFillAction->setEnabled(false);
    connect(m_selectSameFillAction, &QAction::triggered, this, &MainWindow::selectSameFillColor);
    m_selectSameStrokeAction = selectMenu->addAction(QStringLiteral("Select Same Stroke Color"));
    m_selectSameStrokeAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+Alt+Shift+I")));
    m_selectSameStrokeAction->setEnabled(false);
    connect(m_selectSameStrokeAction, &QAction::triggered, this, &MainWindow::selectSameStrokeColor);
    selectMenu->addSeparator();
    QAction* outline = addDisabled(selectMenu, QStringLiteral("Select by Paths (Outline Mode)"));
    outline->setCheckable(true);

    // Prepress --------------------------------------------------------------
    auto* prepress = menuBar()->addMenu(QStringLiteral("Prepress"));
    addDisabled(prepress, QStringLiteral("Add Barcode"), QKeySequence(QStringLiteral("Ctrl+Shift+B")));
    addDisabled(prepress, QStringLiteral("Convert to Barcode"),
                QKeySequence(QStringLiteral("Ctrl+Alt+Shift+B")));
    addDisabled(prepress, QStringLiteral("Add Dimensions"),
                QKeySequence(QStringLiteral("Ctrl+Shift+L")));
    prepress->addSeparator();
    addDisabled(prepress, QStringLiteral("Add Bleed..."));
    addDisabled(prepress, QStringLiteral("White Underprint..."),
                QKeySequence(QStringLiteral("Ctrl+Shift+W")));
    addDisabled(prepress, QStringLiteral("Rich Black..."));
    prepress->addSeparator();
    addTodoSubmenu(prepress, QStringLiteral("Ink Eater Areas"));
    prepress->addSeparator();
    addDisabled(prepress, QStringLiteral("Screening..."),
                QKeySequence(QStringLiteral("Ctrl+Alt+S")));
    addDisabled(prepress, QStringLiteral("Ink Coverage..."));
    addTodoSubmenu(prepress, QStringLiteral("Convert Separations With"));
    prepress->addSeparator();
    addTodoSubmenu(prepress, QStringLiteral("Thin Parts"));
    prepress->addSeparator();
    addDisabled(prepress, QStringLiteral("Update Trap Layer"),
                QKeySequence(QStringLiteral("Ctrl+T")));
    addDisabled(prepress, QStringLiteral("Trapping..."),
                QKeySequence(QStringLiteral("Ctrl+Alt+T")));
    prepress->addSeparator();
    addTodoSubmenu(prepress, QStringLiteral("Warp"));
    prepress->addSeparator();
    addDisabled(prepress, QStringLiteral("Preflight..."));
    addDisabled(prepress, QStringLiteral("Manage Preflight Profiles..."));
    prepress->addSeparator();
    addTodoSubmenu(prepress, QStringLiteral("Variable Data"));
    prepress->addSeparator();
    addTodoSubmenu(prepress, QStringLiteral("Step & Repeat"));
    addDisabled(prepress, QStringLiteral("Create Seamless Repetition..."));
    prepress->addSeparator();
    addTodoSubmenu(prepress, QStringLiteral("Flexo Tools"));

    // View ------------------------------------------------------------------
    auto* viewMenu = menuBar()->addMenu(QStringLiteral("View"));
    auto* zoomMenu = viewMenu->addMenu(QStringLiteral("Zoom"));
    auto* fitInWindow = zoomMenu->addAction(QStringLiteral("Fit in Window / Previous"));
    fitInWindow->setShortcut(QKeySequence(QStringLiteral("Ctrl+0")));
    connect(fitInWindow, &QAction::triggered, this, &MainWindow::fitPage);
    auto* actualSize = zoomMenu->addAction(QStringLiteral("Actual Size / Previous"));
    actualSize->setShortcut(QKeySequence(QStringLiteral("Ctrl+1")));
    connect(actualSize, &QAction::triggered, this, &MainWindow::actualSize);
    auto* zoomInAction = zoomMenu->addAction(QStringLiteral("Zoom In"));
    zoomInAction->setShortcut(QKeySequence::ZoomIn);
    connect(zoomInAction, &QAction::triggered, this, &MainWindow::zoomIn);
    auto* zoomOutAction = zoomMenu->addAction(QStringLiteral("Zoom Out"));
    zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    connect(zoomOutAction, &QAction::triggered, this, &MainWindow::zoomOut);
    zoomMenu->addSeparator();
    m_zoomSelectionAction = zoomMenu->addAction(QStringLiteral("Zoom to Selection / Previous"));
    m_zoomSelectionAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+Alt+0")));
    m_zoomSelectionAction->setEnabled(false);
    connect(m_zoomSelectionAction, &QAction::triggered, this, &MainWindow::zoomToSelection);

    // Rotate View submenu – substitutes the former Todo placeholder.
    // Each action rotates the displayed page by 90° steps;
    // the image is pre-rotated by PDFium, so no QPainter::rotate() is applied.
    auto* r90 = viewMenu->addAction(QStringLiteral("Rotate 90"));
    r90->setShortcut(QKeySequence(QStringLiteral("Ctrl+Alt+R")));
    r90->setEnabled(true);
    connect(r90, &QAction::triggered, this, &MainWindow::rotateViewClockwise);

    auto* r270 = viewMenu->addAction(QStringLiteral("Rotate 270"));
    r270->setShortcut(QKeySequence(QStringLiteral("Ctrl+Alt+L")));
    r270->setEnabled(true);
    connect(r270, &QAction::triggered, this, [this]() {
        m_userRotation = normalizeRotation(m_userRotation - 90);
        applyPageGeometry();
        m_view->setPageSize(displayedPageSize(m_page));
        renderCurrentPage();
        updateUi();
    });

    viewMenu->addSeparator();

    auto* r180 = viewMenu->addAction(QStringLiteral("Rotate 180"));
    r180->setShortcut(QKeySequence(QStringLiteral("Ctrl+Alt+U")));
    r180->setEnabled(true);
    connect(r180, &QAction::triggered, this, [this]() {
        m_userRotation = normalizeRotation(m_userRotation + 180);
        applyPageGeometry();
        m_view->setPageSize(displayedPageSize(m_page));
        renderCurrentPage();
        updateUi();
    });

    viewMenu->addSeparator();
    m_showBoxesAction = viewMenu->addAction(QStringLiteral("Hide Page Boxes"));
    m_showBoxesAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+B")));
    m_showBoxesAction->setCheckable(true);
    m_showBoxesAction->setChecked(true);
    connect(m_showBoxesAction, &QAction::toggled, this, [this](bool on) {
        m_view->setPageBoxesVisible(!on);
    });

    m_crosshairAction = viewMenu->addAction(QStringLiteral("Show Crosshair"));
    m_crosshairAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+R")));
    m_crosshairAction->setCheckable(true);
    connect(m_crosshairAction, &QAction::toggled, this, [this](bool on) {
        m_view->setCrosshairEnabled(on);
    });
    addDisabled(viewMenu, QStringLiteral("Switch to Outline"),
                QKeySequence(QStringLiteral("Ctrl+Y")));
    viewMenu->addSeparator();
    addDisabled(viewMenu, QStringLiteral("Color Managed Preview"),
                QKeySequence(QStringLiteral("Ctrl+Alt+Y")));
    viewMenu->addSeparator();
    addDisabled(viewMenu, QStringLiteral("Open Next Inspector"),
                QKeySequence(QStringLiteral("Ctrl+I")));
    addDisabled(viewMenu, QStringLiteral("Open Previous Inspector"),
                QKeySequence(QStringLiteral("Ctrl+Shift+I")));
    viewMenu->addSeparator();
    addTodoSubmenu(viewMenu, QStringLiteral("Go To"));

    // Window / Help ---------------------------------------------------------
    auto* windowMenu = menuBar()->addMenu(QStringLiteral("Window"));
    if (m_messagesDock)
        windowMenu->addAction(m_messagesDock->toggleViewAction());
    if (m_panelsDock)
        windowMenu->addAction(m_panelsDock->toggleViewAction());


    auto* helpMenu = menuBar()->addMenu(QStringLiteral("Help"));
    auto* about = helpMenu->addAction(QStringLiteral("About ArtPlus..."));
    connect(about, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, QStringLiteral("About ArtPlus"),
            QStringLiteral("ArtPlus - independent prepress / PDF editor.\n"
                           "Menus mirror the ArtPro+ v24 structure observed on the "
                           "installed application; unverified or unimplemented "
                           "entries are disabled."));
    });
}


// ---------------------------------------------------------------------------
// Tool strip
// ---------------------------------------------------------------------------

void MainWindow::buildToolStrip()
{
    auto* bar = new QToolBar(QStringLiteral("Main"), this);
    bar->setMovable(false);
    bar->setFloatable(false);
    bar->setIconSize(QSize(26, 26));
    bar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    addToolBar(Qt::TopToolBarArea, bar);

    auto addBig = [this, bar](theme::IconKind kind, const QString& text, bool checkable = false,
                              bool enabled = true) {
        auto* button = new QToolButton(bar);
        button->setAutoRaise(true);
        button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        button->setIcon(theme::icon(kind, enabled ? theme::kText : theme::kTextDisabled, 26));
        button->setIconSize(QSize(26, 26));
        button->setText(text);
        button->setFont(theme::captionFont());
        button->setCheckable(checkable);
        button->setEnabled(enabled);
        button->setMinimumWidth(66);
        bar->addWidget(button);
        return button;
    };

    addBig(theme::IconKind::ObjectBrowser, QStringLiteral("Object Browser"));
    addBig(theme::IconKind::Colors, QStringLiteral("Colors"));
    addBig(theme::IconKind::Images, QStringLiteral("Images"));
    addBig(theme::IconKind::PlacedArt, QStringLiteral("Placed Art"));

    auto* messagesButton = addBig(theme::IconKind::Messages, QStringLiteral("Messages"), true);
    messagesButton->setChecked(true);

    addBig(theme::IconKind::ActionLists, QStringLiteral("Action Lists"));
    addBig(theme::IconKind::ShareApprove, QStringLiteral("Share & Approve"));

    bar->addSeparator();

    auto addSmall = [bar](theme::IconKind kind, const QString& text, bool enabled) {
        auto* button = new QToolButton(bar);
        button->setAutoRaise(true);
        button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        button->setIcon(theme::icon(kind, enabled ? theme::kText : theme::kTextDisabled, 18));
        button->setIconSize(QSize(18, 18));
        button->setText(text);
        button->setFont(theme::captionFont());
        button->setEnabled(enabled);
        button->setMinimumWidth(52);
        bar->addWidget(button);
        return button;
    };

    addSmall(theme::IconKind::Clean, QStringLiteral("Clean"), true);
    auto* selectBtn = addSmall(theme::IconKind::Select, QStringLiteral("Select"), true);
    selectBtn->setCheckable(true);
    selectBtn->setChecked(true);
    auto* handBtn = addSmall(theme::IconKind::Hand, QStringLiteral("Hand"), true);
    handBtn->setCheckable(true);
    // Make Select and Hand mutually exclusive
    connect(selectBtn, &QToolButton::toggled, this, [this, handBtn](bool checked) {
        if (checked) {
            handBtn->setChecked(false);
            m_view->setInteractionMode(PageView::InteractionMode::Selection);
        }
    });
    connect(handBtn, &QToolButton::toggled, this, [this, selectBtn](bool checked) {
        if (checked) {
            selectBtn->setChecked(false);
            m_view->setInteractionMode(PageView::InteractionMode::Pan);
        }
    });
    addSmall(theme::IconKind::Recolor, QStringLiteral("Recolor"), false);
    addSmall(theme::IconKind::Shaper, QStringLiteral("Shaper"), false);
    addSmall(theme::IconKind::Barcode, QStringLiteral("Barcode"), true);
    addSmall(theme::IconKind::Screening, QStringLiteral("Screening"), true);
    addSmall(theme::IconKind::Trapping, QStringLiteral("Trapping"), true);
    addSmall(theme::IconKind::Wrap, QStringLiteral("Wrap"), false);

    auto* spacer = new QWidget(bar);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    bar->addWidget(spacer);

    addSmall(theme::IconKind::ThreeD, QStringLiteral("3D"), true);
    addSmall(theme::IconKind::Preview, QStringLiteral("Preview"), true);

    connect(messagesButton, &QToolButton::toggled, this, [this](bool on) {
        if (m_messagesDock)
            m_messagesDock->setVisible(on);
    });
}

// ---------------------------------------------------------------------------
// Docks
// ---------------------------------------------------------------------------

void MainWindow::buildLeftDock()
{
    m_messagesPanel = new CollapsiblePanel(QStringLiteral("Messages"), this);

    m_messageTree = new QTreeWidget(m_messagesPanel->body());
    m_messageTree->setHeaderHidden(true);
    m_messageTree->setColumnCount(2);
    m_messageTree->setRootIsDecorated(true);
    m_messageTree->setIndentation(14);
    m_messageTree->setFont(theme::uiFont(8));
    m_messageTree->setFrameShape(QFrame::NoFrame);
    m_messageTree->header()->setStretchLastSection(false);
    m_messageTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_messageTree->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_messageTree->setColumnWidth(1, 24);

    m_messagesPanel->bodyLayout()->addWidget(m_messageTree, 1);
    m_messagesPanel->body()->setMinimumHeight(300);
    m_messagesPanel->body()->setStyleSheet(QStringLiteral("background: #F0F0F0;"));
    m_messageTree->setStyleSheet(
        QStringLiteral("QTreeWidget { background: #F0F0F0; }"
                       "QTreeWidget::item { background: #F0F0F0; }"));

    m_messagesDock = new QDockWidget(QStringLiteral("Messages"), this);
    m_messagesDock->setObjectName(QStringLiteral("messagesDock"));
    m_messagesDock->setTitleBarWidget(new QWidget(m_messagesDock));
    m_messagesDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    m_messagesDock->setMaximumWidth(340);
    m_messagesDock->setWidget(m_messagesPanel);
    addDockWidget(Qt::LeftDockWidgetArea, m_messagesDock);
}

void MainWindow::buildRightDock()
{
    m_panelsDock = new QDockWidget(QStringLiteral("Panels"), this);
    m_panelsDock->setObjectName(QStringLiteral("panelsDock"));
    m_panelsDock->setTitleBarWidget(new QWidget(m_panelsDock));
    m_panelsDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

    auto* scroll = new QScrollArea(m_panelsDock);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(QStringLiteral("background: #F0F0F0; border-left: 1px solid #D0D0D0;"));

    auto* container = new QWidget(scroll);
    container->setStyleSheet(QStringLiteral("background: #F0F0F0;"));
    auto* stack = new QVBoxLayout(container);
    stack->setContentsMargins(0, 0, 0, 0);
    stack->setSpacing(0);

    // --- Document -----------------------------------------------------------
    m_documentPanel = new CollapsiblePanel(QStringLiteral("Document"), container);
    {
        auto* crosshair = new QToolButton();
        crosshair->setCheckable(true);
        crosshair->setAutoRaise(true);
        crosshair->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        crosshair->setIcon(theme::icon(theme::IconKind::Minus, theme::kText, 12));
        crosshair->setText(QStringLiteral("Crosshair"));
        crosshair->setFont(theme::uiFont(8));
        crosshair->setChecked(false);
        crosshair->setStyleSheet(QStringLiteral("QToolButton:checked { background: transparent; border: 0px; }"));
        auto* crossRow = new QWidget();
        auto* crossLayout = new QHBoxLayout(crossRow);
        crossLayout->setContentsMargins(6, 1, 6, 1);
        crossLayout->addWidget(crosshair);
        crossLayout->addStretch();
        m_documentPanel->bodyLayout()->addWidget(crossRow);
        connect(crosshair, &QToolButton::toggled, this, [this](bool on) {
            m_view->setCrosshairEnabled(on);
            if (m_crosshairAction)
                m_crosshairAction->setChecked(on);
        });

        auto* boxes = new QToolButton();
        boxes->setCheckable(true);
        boxes->setAutoRaise(true);
        boxes->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        boxes->setIcon(theme::icon(theme::IconKind::Eye, theme::kText, 12));
        boxes->setText(QStringLiteral("Page Boxes"));
        boxes->setFont(theme::uiFont(8));
        boxes->setChecked(true);
        boxes->setStyleSheet(QStringLiteral("QToolButton:checked { background: transparent; border: 0px; }"));
        auto* boxesRow = new QWidget();
        auto* boxesLayout = new QHBoxLayout(boxesRow);
        boxesLayout->setContentsMargins(6, 1, 6, 1);
        boxesLayout->addWidget(boxes);
        boxesLayout->addStretch();
        m_documentPanel->bodyLayout()->addWidget(boxesRow);
        connect(boxes, &QToolButton::toggled, this, [this, boxes](bool on) {
            boxes->setIcon(theme::icon(on ? theme::IconKind::Eye : theme::IconKind::EyeClosed,
                                       theme::kText, 12));
            m_view->setPageBoxesVisible(on);
            if (m_showBoxesAction)
                m_showBoxesAction->setChecked(on);
        });

        // --- Document metadata labels ---
        m_pageCountLabel = new QLabel();
        m_pageCountLabel->setFont(theme::uiFont(8));
        m_pageCountLabel->setText(QStringLiteral("Pages: –"));
        m_documentPanel->bodyLayout()->addWidget(m_pageCountLabel);

        m_pageSizeLabel = new QLabel();
        m_pageSizeLabel->setFont(theme::uiFont(8));
        m_pageSizeLabel->setText(QStringLiteral("Size: –"));
        m_documentPanel->bodyLayout()->addWidget(m_pageSizeLabel);

        m_orientationLabel = new QLabel();
        m_orientationLabel->setFont(theme::uiFont(8));
        m_orientationLabel->setText(QStringLiteral("Orientation: –"));
        m_documentPanel->bodyLayout()->addWidget(m_orientationLabel);

        m_titleLabel = new QLabel();
        m_titleLabel->setFont(theme::uiFont(8));
        m_titleLabel->setText(QStringLiteral("Title: –"));
        m_documentPanel->bodyLayout()->addWidget(m_titleLabel);

        m_authorLabel = new QLabel();
        m_authorLabel->setFont(theme::uiFont(8));
        m_authorLabel->setText(QStringLiteral("Author: –"));
        m_documentPanel->bodyLayout()->addWidget(m_authorLabel);

        m_creatorLabel = new QLabel();
        m_creatorLabel->setFont(theme::uiFont(8));
        m_creatorLabel->setText(QStringLiteral("Creator: –"));
        m_documentPanel->bodyLayout()->addWidget(m_creatorLabel);

        m_producerLabel = new QLabel();
        m_producerLabel->setFont(theme::uiFont(8));
        m_producerLabel->setText(QStringLiteral("Producer: –"));
        m_documentPanel->bodyLayout()->addWidget(m_producerLabel);

        m_creationDateLabel = new QLabel();
        m_creationDateLabel->setFont(theme::uiFont(8));
        m_creationDateLabel->setText(QStringLiteral("Created: –"));
        m_documentPanel->bodyLayout()->addWidget(m_creationDateLabel);

        m_modificationDateLabel = new QLabel();
        m_modificationDateLabel->setFont(theme::uiFont(8));
        m_modificationDateLabel->setText(QStringLiteral("Modified: –"));
        m_documentPanel->bodyLayout()->addWidget(m_modificationDateLabel);

        m_outputIntentLabel = new QLabel();
        m_outputIntentLabel->setFont(theme::uiFont(8));
        m_outputIntentLabel->setText(QStringLiteral("Output Intent: –"));
        m_documentPanel->bodyLayout()->addWidget(m_outputIntentLabel);
    }
    stack->addWidget(m_documentPanel);

    // --- Layers -------------------------------------------------------------
    m_layersPanel = new CollapsiblePanel(QStringLiteral("Layers"), container);
    {
        auto* header = new QWidget();
        header->setStyleSheet(QStringLiteral("background: #ECECEC;"));
        auto* headerLayout = new QHBoxLayout(header);
        headerLayout->setContentsMargins(6, 2, 10, 2);
        headerLayout->setSpacing(6);
        headerLayout->addWidget(makeIconLabel(theme::IconKind::Eye, 14));
        headerLayout->addWidget(makeIconLabel(theme::IconKind::Lock, 12, theme::kTextDisabled));
        headerLayout->addWidget(makeIconLabel(theme::IconKind::SquareOutline, 12, theme::kTextDisabled));
        headerLayout->addWidget(makeLabel(QStringLiteral("Name"), true, 8), 1);
        headerLayout->addWidget(makeIconLabel(theme::IconKind::Printer, 14));
        m_layersPanel->bodyLayout()->addWidget(header);

        auto* row = new QWidget();
        auto* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(6, 2, 10, 2);
        rowLayout->setSpacing(6);
        rowLayout->addWidget(makeIconLabel(theme::IconKind::Eye, 14));
        rowLayout->addWidget(makeIconLabel(theme::IconKind::LockOpen, 12, theme::kTextDisabled));
        rowLayout->addWidget(makeIconLabel(theme::IconKind::SquareOutline, 12, theme::kTextDisabled));
        m_layerName = makeLabel(QStringLiteral("Layer 1"), true, 8);
        rowLayout->addWidget(m_layerName, 1);
        rowLayout->addWidget(makeIconLabel(theme::IconKind::Check, 14));
        m_layersPanel->bodyLayout()->addWidget(row);

        auto* infoRow = new QWidget();
        auto* infoLayout = new QHBoxLayout(infoRow);
        infoLayout->setContentsMargins(30, 0, 10, 3);
        m_layerInfo = makeLabel(QStringLiteral("no document"), false, 8, theme::kTextDisabled);
        infoLayout->addWidget(m_layerInfo, 1);
        m_layersPanel->bodyLayout()->addWidget(infoRow);

        m_objectList = new QListWidget();
        m_objectList->setFont(theme::uiFont(8));
        m_objectList->setFrameShape(QFrame::NoFrame);
        m_objectList->setSelectionMode(QAbstractItemView::ExtendedSelection);
        m_objectList->setMinimumHeight(140);
        connect(m_objectList, &QListWidget::itemSelectionChanged, this,
                &MainWindow::onObjectSelectionChanged);
        m_layersPanel->bodyLayout()->addWidget(m_objectList, 1);

        auto* tools = new QWidget();
        auto* toolsLayout = new QHBoxLayout(tools);
        toolsLayout->setContentsMargins(6, 1, 10, 1);
        toolsLayout->setSpacing(1);
        toolsLayout->addStretch();
        toolsLayout->addWidget(makeFlatButton(theme::IconKind::Plus, QStringLiteral("Add layer")));
        toolsLayout->addWidget(makeFlatButton(theme::IconKind::Minus, QStringLiteral("Remove layer")));
        toolsLayout->addWidget(makeFlatButton(theme::IconKind::LayersOrder, QStringLiteral("Layer order")));
        toolsLayout->addWidget(makeFlatButton(theme::IconKind::Gear, QStringLiteral("Layer options")));
        m_layersPanel->bodyLayout()->addWidget(tools);
    }
    stack->addWidget(m_layersPanel);

    // --- Separations --------------------------------------------------------
    m_separationsPanel = new CollapsiblePanel(QStringLiteral("Separations"), container);
    {
        auto* profileRow = new QWidget();
        auto* profileLayout = new QHBoxLayout(profileRow);
        profileLayout->setContentsMargins(6, 3, 6, 3);
        profileLayout->setSpacing(4);
        profileLayout->addWidget(makeLabel(QStringLiteral("Document Profile:")));
        profileLayout->addWidget(makeIconLabel(theme::IconKind::Warning, 13));
        m_profileValue = makeLabel(QStringLiteral("(not specified)"), false, 8,
                                   QColor(0x2F, 0x6F, 0xD0));
        profileLayout->addWidget(m_profileValue, 1);
        m_separationsPanel->bodyLayout()->addWidget(profileRow);

        auto* header = new QWidget();
        header->setStyleSheet(QStringLiteral("background: #ECECEC;"));
        auto* headerLayout = new QHBoxLayout(header);
        headerLayout->setContentsMargins(6, 2, 10, 2);
        headerLayout->setSpacing(6);
        headerLayout->addWidget(makeIconLabel(theme::IconKind::Eye, 13));
        auto* nameHeader = makeLabel(QStringLiteral("Name"), true, 8);
        nameHeader->setMinimumWidth(70);
        headerLayout->addWidget(nameHeader, 1);
        auto* hashHeader = makeLabel(QStringLiteral("#"), true, 8);
        hashHeader->setFixedWidth(14);
        headerLayout->addWidget(hashHeader);
        auto* setupHeader = makeLabel(QStringLiteral("Setup"), true, 8);
        setupHeader->setFixedWidth(36);
        headerLayout->addWidget(setupHeader);
        m_separationsPanel->bodyLayout()->addWidget(header);

        struct Ink {
            QString name;
            QColor color;
            int number;
            bool real; // whether color is extracted from PDF or profile-based
        };
        // Profile-based CMYK inks (always displayed as reference)
        const Ink inks[4] = {{QStringLiteral("Ciano"), QColor(0x00, 0xAE, 0xEF), 1, false},
                             {QStringLiteral("Magenta"), QColor(0xEC, 0x00, 0x8C), 2, false},
                             {QStringLiteral("Amarelo"), QColor(0xFF, 0xF2, 0x00), 3, false},
                             {QStringLiteral("Preto"), QColor(0x23, 0x1F, 0x20), 4, false}};

        // Try to get actual ink names from document if possible
        QStringList actualInkNames;
        if (m_doc && m_doc->isValid()) {
            // PDFium 158 nÃ£o fornece FPDFDoc_GetInkNames/neste projeto;
            // exibir aviso informativo
            actualInkNames = QStringList() << QStringLiteral("(API nÃ£o disponÚvel em PDFium 158 deste projeto)")
                                             << QStringLiteral("Use Ghidra/ArtPro+ v24 para extraÁÃ£o real");
        }

        for (const Ink& ink : inks) {
            auto* row = new QWidget();
            auto* rowLayout = new QHBoxLayout(row);
            rowLayout->setContentsMargins(6, 1, 10, 1);
            rowLayout->setSpacing(6);
            rowLayout->addWidget(makeIconLabel(theme::IconKind::Eye, 13));

            auto* swatch = new QLabel();
            swatch->setFixedSize(14, 14);
            swatch->setStyleSheet(QStringLiteral("background: %1; border: 1px solid #808080;")
                                      .arg(ink.color.name()));
            rowLayout->addWidget(swatch);

            auto* name = makeLabel(ink.name + (ink.real ? QStringLiteral(" *") : QString()));
            name->setMinimumWidth(66);
            rowLayout->addWidget(name, 1);

            auto* number = makeLabel(QString::number(ink.number));
            number->setFixedWidth(14);
            rowLayout->addWidget(number);

            // Show status indicator: (profile-based) or nothing
            auto* statusLabel = new QLabel(ink.real ? QStringLiteral("(perfil)") : QStringLiteral("(profile-based)"));
            statusLabel->setFont(theme::uiFont(6));
            statusLabel->setStyleSheet(QStringLiteral("color: #808080;"));
            rowLayout->addWidget(statusLabel, 0, Qt::AlignRight);

            auto* check = new QCheckBox();
            check->setFixedWidth(16);
            rowLayout->addWidget(check, 0, Qt::AlignRight);
            m_separationsPanel->bodyLayout()->addWidget(row);
        }

        // Show information about ink extraction availability
        if (!actualInkNames.isEmpty()) {
            auto* infoRow = new QWidget();
            auto* infoLayout = new QHBoxLayout(infoRow);
            infoLayout->setContentsMargins(6, 2, 10, 2);
            auto* infoLabel = makeLabel(actualInkNames.first());
            infoLayout->addWidget(infoLabel, 1);
            m_separationsPanel->bodyLayout()->addWidget(infoRow);
        }

        auto* tools = new QWidget();
        auto* toolsLayout = new QHBoxLayout(tools);
        toolsLayout->setContentsMargins(6, 1, 10, 1);
        toolsLayout->setSpacing(1);
        toolsLayout->addWidget(makeFlatButton(theme::IconKind::Printer, QStringLiteral("Print setup")));
        toolsLayout->addStretch();
        toolsLayout->addWidget(makeFlatButton(theme::IconKind::Plus, QStringLiteral("Add separation")));
        toolsLayout->addWidget(makeFlatButton(theme::IconKind::Minus, QStringLiteral("Remove separation")));
        toolsLayout->addWidget(makeFlatButton(theme::IconKind::Gear, QStringLiteral("Separation options")));
        m_separationsPanel->bodyLayout()->addWidget(tools);
    }
    stack->addWidget(m_separationsPanel);

    // --- Quality Control ----------------------------------------------------
    m_qualityPanel = new CollapsiblePanel(QStringLiteral("Quality Control"), container);
    {
        auto* combo = new QComboBox();
        combo->addItems({QStringLiteral("Total Area Coverage"), QStringLiteral("Page Boxes"),
                          QStringLiteral("Separations"), QStringLiteral("Fonts"),
                          QStringLiteral("Images"), QStringLiteral("Resolution")});
        combo->setFont(theme::uiFont(8));
        auto* comboRow = new QWidget();
        auto* comboLayout = new QHBoxLayout(comboRow);
        comboLayout->setContentsMargins(6, 3, 6, 3);
        comboLayout->addWidget(combo, 1);
        m_qualityPanel->bodyLayout()->addWidget(comboRow);

        // Run button
        m_runPreflightButton = new QPushButton(QStringLiteral("Run"));
        m_runPreflightButton->setFont(theme::uiFont(8));
        m_runPreflightButton->setFixedHeight(28);
        m_runPreflightButton->setDefault(true);
        auto* buttonRow = new QWidget();
        auto* buttonLayout = new QHBoxLayout(buttonRow);
        buttonLayout->setContentsMargins(6, 3, 6, 3);
        buttonLayout->addStretch();
        buttonLayout->addWidget(m_runPreflightButton);
        m_qualityPanel->bodyLayout()->addWidget(buttonRow);

        // Results area
        m_preflightResults = new QListWidget();
        m_preflightResults->setFont(theme::uiFont(7));
        m_preflightResults->setMinimumHeight(80);
        m_preflightResults->setFrameStyle(QFrame::NoFrame);
        m_qualityPanel->bodyLayout()->addWidget(m_preflightResults);

        auto* limitRow = new QWidget();
        auto* limitLayout = new QHBoxLayout(limitRow);
        limitLayout->setContentsMargins(6, 2, 6, 4);
        limitLayout->addWidget(makeLabel(QStringLiteral("TAC Limit:")));
        m_tacLimit = new QSpinBox();
        m_tacLimit->setRange(0, 400);
        m_tacLimit->setValue(300);
        m_tacLimit->setSuffix(QStringLiteral(" %"));
        m_tacLimit->setFont(theme::uiFont(8));
        m_tacLimit->setFixedWidth(72);
        limitLayout->addWidget(m_tacLimit);
        limitLayout->addStretch();
        m_qualityPanel->bodyLayout()->addWidget(limitRow);

        // Connect run button
        connect(m_runPreflightButton, &QPushButton::clicked, this, &MainWindow::onRunPreflight);
    }
    stack->addWidget(m_qualityPanel);

    stack->addStretch(1);
    scroll->setWidget(container);
    m_panelsDock->setWidget(scroll);
    m_panelsDock->setMinimumWidth(236);
    m_panelsDock->setMaximumWidth(300);
    addDockWidget(Qt::RightDockWidgetArea, m_panelsDock);

    resizeDocks({m_messagesDock, m_panelsDock}, {235, 255}, Qt::Horizontal);
}

// ---------------------------------------------------------------------------
// Document handling
// ---------------------------------------------------------------------------

bool MainWindow::openDocument(const QString& path)
{
    const QFileInfo info(path);
    if (!info.exists() || !info.isFile()) {
        showError(QStringLiteral("Open PDF"),
                  QStringLiteral("File not found:\n%1").arg(path));
        return false;
    }

    QString error;
    auto document = artplus::pdf::PdfDocument::open(path, &error);
    if (!document || !document->isValid()) {
        showError(QStringLiteral("Open PDF"),
                  QStringLiteral("Could not open the PDF file.\n\n%1")
                      .arg(error.isEmpty() ? QStringLiteral("The file may be encrypted or corrupted.")
                                           : error));
        return false;
    }

    if (document->pageCount() <= 0) {
        showError(QStringLiteral("Open PDF"), QStringLiteral("The document contains no pages."));
        return false;
    }

    m_doc = std::move(document);
    m_snapshot = std::make_unique<artplus::pdf::PdfSnapshot>(
        artplus::pdf::PdfSnapshot::capture(*m_doc));
    m_page = 0;
    m_userRotation = 0;
    m_history = std::make_unique<artplus::edit::CommandStack>();

    applyPageGeometry();
    populatePageList();
    updateWindowTitle();
    m_view->fitPage();
    renderCurrentPage();
    updateUi();
    updatePanels();
    updateDocumentInfo();  // <-- added
    rebuildEditor();
    emit documentChanged();
    return true;
}

bool MainWindow::saveCurrent()
{
    if (!hasDocument()) {
        statusBar()->showMessage(QStringLiteral("Nothing to save"), 3000);
        return false;
    }
    statusBar()->showMessage(
        QStringLiteral("In-place saving is not available. Use File > Save As (edited)… or Save As Copy…"),
        6000);
    return false;
}

void MainWindow::saveAsCopy()
{
    if (!hasDocument()) {
        statusBar()->showMessage(QStringLiteral("No document to save"), 3000);
        return;
    }

    const QFileInfo source(m_doc->filePath());
    const QString suggested = source.absolutePath() + QLatin1Char('/') +
                              source.completeBaseName() + QStringLiteral("-copy.pdf");
    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Save a preserved copy"), suggested,
        QStringLiteral("PDF Files (*.pdf)"));
    if (path.isEmpty())
        return;

    QString error;
    if (!m_doc->saveCopyPreserving(path, &error)) {
        showError(QStringLiteral("Save As Copy"), error);
        return;
    }

    QByteArray writtenHash;
    QFile written(path);
    if (written.open(QIODevice::ReadOnly)) {
        QCryptographicHash hash(QCryptographicHash::Sha256);
        hash.addData(&written);
        writtenHash = hash.result().toHex();
    }
    const bool identical = !writtenHash.isEmpty() &&
                           writtenHash == m_doc->sourceSha256();
    logInfo(QStringLiteral("Preserved copy written: %1 (%2)")
                .arg(path, identical ? QStringLiteral("byte-identical")
                                    : QStringLiteral("content differs")));
    statusBar()->showMessage(
        QStringLiteral("Copy written: %1 — %2")
            .arg(QFileInfo(path).fileName(),
                 identical ? QStringLiteral("byte-identical to the source")
                           : QStringLiteral("content differs")),
        6000);
}

bool MainWindow::hasDocument() const { return m_doc && m_doc->isValid(); }
int MainWindow::pageCount() const { return hasDocument() ? m_doc->pageCount() : 0; }
int MainWindow::currentPageIndex() const { return m_page; }
double MainWindow::zoom() const { return m_view ? m_view->zoom() : 0.0; }

void MainWindow::closeDocument()
{
    const bool dirty = (m_history && m_history->isModified()) ||
                       (!m_history && m_editor && m_editor->isModified());
    if (dirty) {
        const auto answer = QMessageBox::question(
            this, QStringLiteral("Unsaved edits"),
            QStringLiteral("This page has unsaved edits. Discard them?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes)
            return;
    }
    discardEditor();
    resetDocumentState();
    updateUi();
    updatePanels();
    updateDocumentInfo();  // <-- added
    updateWindowTitle();
}

void MainWindow::resetDocumentState()
{
    m_doc.reset();
    m_snapshot.reset();
    m_page = 0;
    m_userRotation = 0;
    m_selectedObject = -1;
    m_selectedObjects.clear();
    m_transformDefaults = artplus::edit::TransformParams{};
    m_transformAnchor = artplus::edit::TransformAnchor::TopLeft;
    m_view->clearPage();
    m_view->setPageSize(QSizeF(595.275590551, 841.889763780));
    if (m_messageTree)
        m_messageTree->clear();
    if (m_layerName)
        m_layerName->setText(QStringLiteral("Layer 1"));
    if (m_profileValue)
        m_profileValue->setText(QStringLiteral("(not specified)"));
}

QSizeF MainWindow::displayedPageSize(int pageIndex) const
{
    if (!hasDocument() || !m_doc->hasPage(pageIndex))
        return {595.275590551, 841.889763780};
    const QSizeF raw = m_doc->pageSize(pageIndex);
    const int total = normalizeRotation(m_doc->pageRotation(pageIndex) + m_userRotation);
    if (total == 90 || total == 270)
        return QSizeF(raw.height(), raw.width());
    return raw;
}

void MainWindow::applyPageGeometry()
{
    if (!hasDocument())
        return;
    m_view->setPageSize(displayedPageSize(m_page));
}

int MainWindow::totalViewRotation() const
{
    if (!hasDocument() || !m_doc->hasPage(m_page))
        return normalizeRotation(m_userRotation);
    return normalizeRotation(m_doc->pageRotation(m_page) + m_userRotation);
}

// Mapping between the PDF's own page space and the space of the page as drawn.
// The rendered image is rotated clockwise by `totalViewRotation()`, so a PDF
// point (x, y) on a w-by-h page lands at:
//   90 deg  -> (h - y, x)
//   180 deg -> (w - x, h - y)
//   270 deg -> (y, w - x)
QRectF MainWindow::pdfRectToDisplayed(const QRectF& rect) const
{
    const int rotation = totalViewRotation();
    if (rotation == 0)
        return rect;

    const QSizeF size = m_doc->pageSize(m_page);
    const double w = size.width();
    const double h = size.height();
    const double x1 = rect.left();
    const double y1 = rect.top();
    const double x2 = rect.right();
    const double y2 = rect.bottom();

    switch (rotation) {
    case 90:
        return QRectF(h - y2, x1, y2 - y1, x2 - x1);
    case 180:
        return QRectF(w - x2, h - y2, x2 - x1, y2 - y1);
    case 270:
        return QRectF(y1, w - x2, y2 - y1, x2 - x1);
    default:
        break;
    }
    return rect;
}

QPointF MainWindow::displayedPointToPdf(const QPointF& point) const
{
    const int rotation = totalViewRotation();
    if (rotation == 0)
        return point;

    const QSizeF size = m_doc->pageSize(m_page);
    const double w = size.width();
    const double h = size.height();

    switch (rotation) {
    case 90:
        return QPointF(point.y(), h - point.x());
    case 180:
        return QPointF(w - point.x(), h - point.y());
    case 270:
        return QPointF(w - point.y(), point.x());
    default:
        break;
    }
    return point;
}

void MainWindow::renderCurrentPage()
{
    if (!hasDocument() || !m_doc->hasPage(m_page)) {
        m_view->clearPage();
        return;
    }

    const QSizeF pageSize = m_doc->pageSize(m_page);
    const double longestPt = std::max(pageSize.width(), pageSize.height());

    double renderScale = m_view->zoom();
    if (longestPt > 1e-6) {
        const double maxScale = kMaxRenderPixels / longestPt;
        renderScale = std::min(renderScale, maxScale);
    }
    renderScale = std::max(renderScale, 1.0 / 64.0);

    QImage image;
    if (m_editor && m_editor->isValid() && m_editor->pageIndex() == m_page)
        image = m_editor->render(renderScale, m_userRotation);
    else
        image = m_doc->renderPage(m_page, renderScale, m_userRotation, true).image;

    if (!image.isNull())
        m_view->setPageImage(image);
    else
        m_view->clearPage();

    updateStatus();
}

void MainWindow::scheduleRender()
{
    if (m_renderTimer)
        m_renderTimer->start();
}

void MainWindow::onRenderTimer() { renderCurrentPage(); }

void MainWindow::previousPage()
{
    if (m_page > 0)
        goToPage(m_page);
}

void MainWindow::nextPage()
{
    if (hasDocument() && m_page + 1 < m_doc->pageCount())
        goToPage(m_page + 2);
}

void MainWindow::goToPage(int oneBasedIndex)
{
    if (m_suppressSpinSignal)
        return;
    if (!hasDocument())
        return;
    const int target = oneBasedIndex - 1;
    if (target < 0 || target >= m_doc->pageCount() || target == m_page)
        return;

    const bool dirty = (m_history && m_history->isModified()) ||
                       (!m_history && m_editor && m_editor->isModified());
    if (dirty) {
        const auto answer = QMessageBox::question(
            this, QStringLiteral("Unsaved edits"),
            QStringLiteral("This page has unsaved edits. Discard them and change page?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes) {
            updateUi();
            return;
        }
    }

    discardEditor();
    m_page = target;
    applyPageGeometry();
    renderCurrentPage();
    updateUi();
    rebuildEditor();
    updateDocumentInfo();  // <-- added
}

void MainWindow::fitPage()
{
    if (!hasDocument())
        return;
    applyPageGeometry();
    m_view->fitPage();
    renderCurrentPage();
}

void MainWindow::fitWidth()
{
    if (!hasDocument())
        return;
    applyPageGeometry();
    m_view->fitWidth();
    renderCurrentPage();
}

void MainWindow::actualSize() { m_view->actualSize(); }
void MainWindow::zoomIn() { m_view->zoomIn(); }
void MainWindow::zoomOut() { m_view->zoomOut(); }

void MainWindow::rotateViewClockwise()
{
    m_userRotation = normalizeRotation(m_userRotation + 90);
    applyPageGeometry();
    m_view->setPageSize(displayedPageSize(m_page));
    renderCurrentPage();
    updateUi();
}

void MainWindow::populatePageList()
{
    // Pages are navigated from the status bar; no page list panel in this layout.
}

void MainWindow::updateDocumentInfo()
{
    if (!m_doc || !m_doc->isValid()) {
        m_pageCountLabel->setText(QStringLiteral("Pages: –"));
        m_pageSizeLabel->setText(QStringLiteral("Size: –"));
        m_orientationLabel->setText(QStringLiteral("Orientation: –"));
        m_titleLabel->setText(QStringLiteral("Title: –"));
        m_authorLabel->setText(QStringLiteral("Author: –"));
        m_creatorLabel->setText(QStringLiteral("Creator: –"));
        m_producerLabel->setText(QStringLiteral("Producer: –"));
        m_creationDateLabel->setText(QStringLiteral("Created: –"));
        m_modificationDateLabel->setText(QStringLiteral("Modified: –"));
        m_outputIntentLabel->setText(QStringLiteral("Output Intent: –"));
        return;
    }

    // Page count
    int count = m_doc->pageCount();
    m_pageCountLabel->setText(QStringLiteral("Pages: %1").arg(count));

    // Page size and orientation (use first page)
    if (count > 0) {
        QSizeF size = m_doc->pageSize(0);
        m_pageSizeLabel->setText(QStringLiteral("Size: %1 × %2 pt").arg(size.width(), 0, 'f', 1).arg(size.height(), 0, 'f', 1));

        int rotation = m_doc->pageRotation(0);
        QString orient;
        switch (rotation) {
        case 0: orient = QStringLiteral("0° (portrait)"); break;
        case 90: orient = QStringLiteral("90° (landscape)"); break;
        case 180: orient = QStringLiteral("180° (upside-down)"); break;
        case 270: orient = QStringLiteral("270° (right portrait)"); break;
        default: orient = QStringLiteral("%1°").arg(rotation);
        }
        m_orientationLabel->setText(QStringLiteral("Orientation: %1").arg(orient));
    } else {
        m_pageSizeLabel->setText(QStringLiteral("Size: –"));
        m_orientationLabel->setText(QStringLiteral("Orientation: –"));
    }

    // Metadata
    m_titleLabel->setText(QStringLiteral("Title: %1").arg(m_doc->title().isEmpty() ? QStringLiteral("(not specified)") : m_doc->title()));
    m_authorLabel->setText(QStringLiteral("Author: %1").arg(m_doc->author().isEmpty() ? QStringLiteral("(not specified)") : m_doc->author()));

    m_creatorLabel->setText(QStringLiteral("Creator: %1").arg(m_doc->creator().isEmpty() ? QStringLiteral("(not specified)") : m_doc->creator()));

    m_producerLabel->setText(QStringLiteral("Producer: %1").arg(m_doc->producer().isEmpty() ? QStringLiteral("(not specified)") : m_doc->producer()));

    m_creationDateLabel->setText(QStringLiteral("Created: %1").arg(m_doc->creationDate().isEmpty() ? QStringLiteral("(not specified)") : m_doc->creationDate()));
    m_modificationDateLabel->setText(QStringLiteral("Modified: %1").arg(m_doc->modificationDate().isEmpty() ? QStringLiteral("(not specified)") : m_doc->modificationDate()));

    // Output Intent
    QString outputIntent = m_doc->outputIntentName();
    m_outputIntentLabel->setText(QStringLiteral("Output Intent: %1").arg(outputIntent.isEmpty() ? QStringLiteral("(not specified)") : outputIntent));
}

void MainWindow::rebuildEditor()
{
    discardEditor();
    if (!hasDocument())
        return;

    m_editor = std::make_unique<artplus::pdf::PdfEditor>(*m_doc, m_page);
    if (!m_editor->isValid()) {
        logWarning(QStringLiteral("Page editing unavailable for page %1").arg(m_page + 1));
        m_editor.reset();
        updatePanels();
        return;
    }
    for (const QString& warning : m_editor->warnings())
        logWarning(warning);
    refreshObjectList();
    updatePanels();
}

void MainWindow::discardEditor()
{
    if (m_editor && m_editor->isModified())
        logWarning(QStringLiteral("Discarding unsaved edits on page %1").arg(m_page + 1));
    m_editor.reset();
    if (m_history)
        m_history->clear();
    m_selectedObject = -1;
    if (m_view)
        m_view->clearSelection();
    refreshProperties();
    if (m_objectList) {
        QSignalBlocker blocker(m_objectList);
        m_objectList->clear();
    }
    updatePanels();
}

namespace {

QString objectRowText(int index, const artplus::pdf::EditableObject& object)
{
    const QString kind = artplus::pdf::pageObjectKindName(object.kind);
    const QRectF& b = object.bounds;
    const QString bounds = QStringLiteral("%1×%2 @ %3,%4")
                               .arg(b.width(), 0, 'f', 1)
                               .arg(b.height(), 0, 'f', 1)
                               .arg(b.left(), 0, 'f', 1)
                               .arg(b.top(), 0, 'f', 1);
    QString detail;
    if (object.kind == artplus::pdf::PageObjectKind::Text)
        detail = QStringLiteral(" \"%1\"").arg(object.text.left(24));
    else if (object.kind == artplus::pdf::PageObjectKind::Image)
        detail = QStringLiteral(" %1×%2 px").arg(object.imageWidth).arg(object.imageHeight);
    else if (object.kind == artplus::pdf::PageObjectKind::Form)
        detail = QStringLiteral(" %1 children").arg(object.formChildCount);
    
    // Informação sobre presença de canais de color (sem valores CMYK reais)
    // Usa flags hasFillColor/hasStrokeColor do EditableObject
    QString colorInfo;
    if (object.hasFillColor)
        colorInfo = QStringLiteral(" Fill:Sim");
    if (object.hasStrokeColor)
        colorInfo += QStringLiteral(" Stroke:Sim");
    
    return QStringLiteral("%1  %2  %3%4%5")
        .arg(index).arg(kind, bounds, detail, colorInfo.isEmpty() ? QString() : QStringLiteral(" [") + colorInfo + QStringLiteral("]"));
}

} // namespace

void MainWindow::refreshObjectList()
{
    if (!m_objectList)
        return;

    {
        QSignalBlocker blocker(m_objectList);
        m_objectList->clear();
        if (m_editor) {
            for (int i = 0; i < m_editor->objectCount(); ++i) {
                if (const auto* object = m_editor->object(i))
                    m_objectList->addItem(objectRowText(i, *object));
            }
        }

        if (m_objectList->count() > 0) {
            QVector<int> kept;
            for (int row : m_selectedObjects) {
                if (row >= 0 && row < m_objectList->count())
                    kept.append(row);
            }
            if (kept.isEmpty())
                kept.append(std::clamp(m_selectedObject, 0, m_objectList->count() - 1));
            m_objectList->setCurrentRow(kept.front());
            for (int i = 1; i < kept.size(); ++i) {
                if (QListWidgetItem* item = m_objectList->item(kept.at(i)))
                    item->setSelected(true);
            }
        }
    }

    if (m_objectList->count() == 0) {
        m_selectedObject = -1;
        m_selectedObjects.clear();
        if (m_view)
            m_view->clearSelection();
        refreshProperties();
        updatePanels();
        return;
    }
    onObjectSelectionChanged();
}

void MainWindow::onObjectSelectionChanged()
{
    m_selectedObjects.clear();
    m_selectedObject = -1;

    if (m_editor && m_objectList) {
        const QList<QListWidgetItem*> items = m_objectList->selectedItems();
        for (QListWidgetItem* item : items) {
            const int row = m_objectList->row(item);
            if (row >= 0 && row < m_editor->objectCount())
                m_selectedObjects.append(row);
        }
        std::sort(m_selectedObjects.begin(), m_selectedObjects.end());
        const auto last = std::unique(m_selectedObjects.begin(), m_selectedObjects.end());
        m_selectedObjects.erase(last, m_selectedObjects.end());
        if (!m_selectedObjects.isEmpty())
            m_selectedObject = m_selectedObjects.first();
    }

    updateSelectionOverlay();
    refreshProperties();

    if (m_editor && m_selectedObject >= 0) {
        if (const auto* object = m_editor->object(m_selectedObject)) {
            if (m_selectedObjects.size() > 1) {
                statusBar()->showMessage(
                    QStringLiteral("%1 objects selected — Object > Transform… applies one "
                                   "transform to the selection")
                        .arg(m_selectedObjects.size()),
                    6000);
            } else {
                statusBar()->showMessage(
                    QStringLiteral("%1 selected — %2. Use Object > Move … (Ctrl+arrows) then "
                                   "File > Save As (edited)…")
                        .arg(artplus::pdf::pageObjectKindName(object->kind),
                             objectRowText(m_selectedObject, *object)),
                    6000);
            }
        }
    }
    updatePanels();
}

void MainWindow::nudgeSelection(double dx, double dy)
{
    if (!m_editor || m_selectedObjects.isEmpty())
        return;

    bool moved = false;
    if (m_selectedObjects.size() == 1) {
        moved = pushCommand(std::make_unique<artplus::edit::MoveObjectCommand>(
            *m_editor, m_selectedObjects.first(), dx, dy));
    } else {
        std::vector<int> indices(m_selectedObjects.begin(), m_selectedObjects.end());
        artplus::edit::TransformParams params;
        params.moveX = dx;
        params.moveY = dy;
        auto command = std::make_unique<artplus::edit::TransformObjectsCommand>(
            *m_editor, indices, params, artplus::edit::TransformAnchor::TopLeft, false);
        moved = command->hasEffect() && pushCommand(std::move(command));
    }

    if (!moved) {
        statusBar()->showMessage(
            QStringLiteral("The selected object cannot be edited"), 4000);
        return;
    }

    statusBar()->showMessage(
        QStringLiteral("Moved by (%1, %2) pt — unsaved. Use File > Save As (edited)…")
            .arg(dx)
            .arg(dy),
        5000);
    updatePanels();
}

void MainWindow::updateSelectionOverlay()
{
    if (!m_view)
        return;
    if (m_editor && !m_selectedObjects.isEmpty()) {
        QVector<QRectF> rects;
        rects.reserve(m_selectedObjects.size());
        for (int index : m_selectedObjects) {
            if (const auto* object = m_editor->object(index))
                rects.append(pdfRectToDisplayed(object->bounds));
        }
        if (!rects.isEmpty()) {
            m_view->setSelectionRects(rects);
            return;
        }
    }
    m_view->clearSelection();
}

void MainWindow::saveEditedAs()
{
    if (!m_editor || !hasDocument()) {
        statusBar()->showMessage(QStringLiteral("No editable page"), 3000);
        return;
    }

    const QFileInfo source(m_doc->filePath());
    const QString suggested = source.absolutePath() + QLatin1Char('/') +
                              source.completeBaseName() + QStringLiteral("-edited.pdf");
    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Save edited PDF"), suggested,
        QStringLiteral("PDF Files (*.pdf)"));
    if (path.isEmpty())
        return;

    QString error;
    if (!m_editor->saveAs(path, &error)) {
        showError(QStringLiteral("Save edited PDF"), error);
        return;
    }

    logInfo(QStringLiteral("Edited PDF saved and verified: %1").arg(path));
    if (m_history)
        m_history->markClean();
    statusBar()->showMessage(
        QStringLiteral("Saved and verified: %1").arg(QFileInfo(path).fileName()), 8000);
    updatePanels();
    updateWindowTitle();
}

void MainWindow::zoomToSelection()
{
    if (!m_editor || !m_view || m_selectedObjects.isEmpty())
        return;

    bool hasBounds = false;
    QRectF bounds;
    for (int index : m_selectedObjects) {
        const auto* object = m_editor->object(index);
        if (!object || object->bounds.isEmpty())
            continue;
        // zoomToSelection works in displayed-page space, like the view itself.
        const QRectF displayed = pdfRectToDisplayed(object->bounds);
        bounds = hasBounds ? bounds.united(displayed) : displayed;
        hasBounds = true;
    }
    if (hasBounds)
        m_view->zoomToSelection(bounds);
}

void MainWindow::showTransformDialog()
{
    if (!m_editor || m_selectedObjects.isEmpty())
        return;

    std::vector<int> indices;
    indices.reserve(static_cast<size_t>(m_selectedObjects.size()));
    for (int index : m_selectedObjects) {
        if (index >= 0 && index < m_editor->objectCount())
            indices.push_back(index);
    }
    if (indices.empty())
        return;

    TransformDialog dialog(this);
    dialog.setInitial(m_transformDefaults, m_transformAnchor);
    if (dialog.exec() != QDialog::Accepted)
        return;

    const artplus::edit::TransformParams params = dialog.params();
    m_transformDefaults = params;
    m_transformAnchor = dialog.anchor();

    auto command = std::make_unique<artplus::edit::TransformObjectsCommand>(
        *m_editor, indices, params, dialog.anchor(), dialog.copyRequested());
    if (!command->hasEffect()) {
        statusBar()->showMessage(QStringLiteral("Transform produced no change"), 3000);
        return;
    }

    if (pushCommand(std::move(command))) {
        statusBar()->showMessage(dialog.copyRequested()
                                     ? QStringLiteral("Transform copies created — unsaved")
                                     : QStringLiteral("Transform applied — unsaved"),
                                 5000);
    } else {
        statusBar()->showMessage(QStringLiteral("Transform could not be applied"), 4000);
    }
}

void MainWindow::onTransformAgain()
{
    if (!m_editor || m_selectedObjects.isEmpty() || !m_history)
        return;

    // Retrieve the cached transform parameters from the history.
    const auto last = m_history->lastTransform();
    if (!last.has_value()) {
        statusBar()->showMessage(QStringLiteral("No previous transform to repeat"), 3000);
        return;
    }

    const auto& [params, anchor, isCopy] = last.value();

    std::vector<int> indices;
    indices.reserve(static_cast<size_t>(m_selectedObjects.size()));
    for (int index : m_selectedObjects) {
        if (index >= 0 && index < m_editor->objectCount())
            indices.push_back(index);
    }
    if (indices.empty())
        return;

    auto command = std::make_unique<artplus::edit::TransformObjectsCommand>(
        *m_editor, indices, params, anchor, isCopy);
    if (!command->hasEffect()) {
        statusBar()->showMessage(QStringLiteral("Repeated transform produces no change"), 3000);
        return;
    }

    if (pushCommand(std::move(command))) {
        statusBar()->showMessage(QStringLiteral("Transform repeated — unsaved"), 5000);
    }
}

bool MainWindow::pushCommand(std::unique_ptr<artplus::edit::Command> command)
{
    if (!m_editor || !m_history || !command)
        return false;
    if (!m_history->execute(std::move(command)))
        return false;

    m_editor->applyEdits();
    renderCurrentPage();
    refreshObjectList();
    refreshProperties();
    updatePanels();
    return true;
}

void MainWindow::undoEdit()
{
    if (!m_editor || !m_history || !m_history->undo()) {
        updatePanels();
        return;
    }
    m_editor->applyEdits();
    renderCurrentPage();
    refreshObjectList();
    refreshProperties();
    statusBar()->showMessage(QStringLiteral("Undo"), 2500);
    updatePanels();
}

void MainWindow::redoEdit()
{
    if (!m_editor || !m_history || !m_history->redo()) {
        updatePanels();
        return;
    }
    m_editor->applyEdits();
    renderCurrentPage();
    refreshObjectList();
    refreshProperties();
    statusBar()->showMessage(QStringLiteral("Redo"), 2500);
    updatePanels();
}

void MainWindow::buildObjectPanel()
{
    m_objectPanel = new CollapsiblePanel(QStringLiteral("Object"));
    m_objectPanel->setEnabled(false);

    auto* form = new QWidget();
    auto* grid = new QGridLayout(form);
    grid->setContentsMargins(6, 4, 10, 4);
    grid->setHorizontalSpacing(6);
    grid->setVerticalSpacing(3);

    m_propType = makeLabel(QStringLiteral("-"));
    m_propId = makeLabel(QStringLiteral("-"));
    grid->addWidget(makeLabel(QStringLiteral("Type:")), 0, 0);
    grid->addWidget(m_propType, 0, 1, 1, 3);
    grid->addWidget(makeLabel(QStringLiteral("ID:")), 1, 0);
    grid->addWidget(m_propId, 1, 1, 1, 3);

    auto makeSpin = [this](double lo, double hi, int decimals, const QString& suffix) {
        auto* spin = new QDoubleSpinBox();
        spin->setRange(lo, hi);
        spin->setDecimals(decimals);
        spin->setSuffix(suffix);
        spin->setFont(theme::uiFont(8));
        spin->setKeyboardTracking(false);
        spin->setFixedWidth(120);
        spin->setEnabled(false);
        connect(spin, &QDoubleSpinBox::editingFinished, this, &MainWindow::applyPropertyEdit);
        return spin;
    };

    m_propX = makeSpin(-100000.0, 100000.0, 2, QStringLiteral(" pt"));
    m_propY = makeSpin(-100000.0, 100000.0, 2, QStringLiteral(" pt"));
    m_propW = makeSpin(0.01, 100000.0, 2, QStringLiteral(" pt"));
    m_propH = makeSpin(0.01, 100000.0, 2, QStringLiteral(" pt"));
    m_propRotation = makeSpin(-180.0, 180.0, 1, QStringLiteral(" deg"));

    int row = 2;
    auto addRow = [&](const QString& caption, QWidget* field) {
        grid->addWidget(makeLabel(caption), row, 0);
        grid->addWidget(field, row, 1);
        ++row;
    };
    addRow(QStringLiteral("X:"), m_propX);
    addRow(QStringLiteral("Y:"), m_propY);
    addRow(QStringLiteral("W:"), m_propW);
    addRow(QStringLiteral("H:"), m_propH);
    addRow(QStringLiteral("Rot:"), m_propRotation);

    m_propBounds = makeLabel(QStringLiteral("-"));
    m_propState = makeLabel(QStringLiteral("-"));
    addRow(QStringLiteral("Bounds:"), m_propBounds);
    addRow(QStringLiteral("State:"), m_propState);

    m_propText = makeLabel(QString());
    m_propText->setWordWrap(true);
    m_propFont = makeLabel(QString());
    m_propImage = makeLabel(QString());
    m_propText->setVisible(false);
    m_propFont->setVisible(false);
    m_propImage->setVisible(false);
    grid->addWidget(m_propText, row++, 0, 1, 2);
    grid->addWidget(m_propFont, row++, 0, 1, 2);
    grid->addWidget(m_propImage, row++, 0, 1, 2);

    m_objectPanel->bodyLayout()->addWidget(form);
}

void MainWindow::refreshProperties()
{
    if (!m_objectPanel || !m_propType)
        return;

    m_updatingProperties = true;

    const bool hasObject =
        m_editor && m_editor->isValid() && m_selectedObject >= 0 &&
        m_selectedObject < m_editor->objectCount() && m_editor->object(m_selectedObject);

    if (!hasObject) {
        m_objectPanel->setEnabled(false);
        m_propType->setText(QStringLiteral("-"));
        m_propId->setText(QStringLiteral("-"));
        m_propBounds->setText(QStringLiteral("-"));
        m_propState->setText(QStringLiteral("-"));
        m_propX->setEnabled(false);
        m_propY->setEnabled(false);
        m_propW->setEnabled(false);
        m_propH->setEnabled(false);
        m_propRotation->setEnabled(false);
        m_propText->setVisible(false);
        m_propFont->setVisible(false);
        m_propImage->setVisible(false);
        m_updatingProperties = false;
        return;
    }

    const auto p = artplus::edit::describe(*m_editor->object(m_selectedObject));
    m_objectPanel->setEnabled(true);
    m_propType->setText(p.kind);
    m_propId->setText(QString::number(p.index));
    m_propX->setValue(p.x);
    m_propY->setValue(p.y);
    m_propW->setValue(p.width);
    m_propH->setValue(p.height);
    m_propRotation->setValue(p.rotationDegrees);
    m_propX->setEnabled(true);
    m_propY->setEnabled(true);
    m_propW->setEnabled(p.width > 1e-6);
    m_propH->setEnabled(p.height > 1e-6);
    m_propRotation->setEnabled(true);
    m_propBounds->setText(QStringLiteral("%1, %2  %3 x %4 pt")
                              .arg(p.bounds.left(), 0, 'f', 2)
                              .arg(p.bounds.top(), 0, 'f', 2)
                              .arg(p.bounds.width(), 0, 'f', 2)
                              .arg(p.bounds.height(), 0, 'f', 2));
    const bool dirtyState = m_history ? m_history->isModified() : p.modified;
    m_propState->setText(dirtyState ? QStringLiteral("modified") : QStringLiteral("clean"));

    m_propText->setVisible(!p.text.isEmpty());
    m_propFont->setVisible(!p.fontName.isEmpty());
    m_propImage->setVisible(p.pixelWidth > 0);
    if (!p.text.isEmpty())
        m_propText->setText(QStringLiteral("Content: \"%1\"").arg(p.text));
    if (!p.fontName.isEmpty())
        m_propFont->setText(QStringLiteral("Font: %1 @ %2 pt")
                                .arg(p.fontName)
                                .arg(p.fontSize, 0, 'f', 1));
    if (p.pixelWidth > 0)
        m_propImage->setText(QStringLiteral("Image: %1 x %2 px  (%3 dpi x %4 dpi)")
                                 .arg(p.pixelWidth)
                                 .arg(p.pixelHeight)
                                 .arg(p.dpiX, 0, 'f', 1)
                                 .arg(p.dpiY, 0, 'f', 1));

    m_updatingProperties = false;
}

void MainWindow::applyPropertyEdit()
{
    if (m_updatingProperties || !m_editor || m_selectedObject < 0)
        return;
    const auto* object = m_editor->object(m_selectedObject);
    if (!object)
        return;

    auto* spin = qobject_cast<QDoubleSpinBox*>(sender());
    if (!spin)
        return;

    const auto p = artplus::edit::describe(*object);
    bool applied = false;

    if (spin == m_propX) {
        const double dx = spin->value() - p.x;
        if (std::abs(dx) > 1e-9)
            applied = pushCommand(std::make_unique<artplus::edit::MoveObjectCommand>(
                *m_editor, m_selectedObject, dx, 0.0));
    } else if (spin == m_propY) {
        const double dy = spin->value() - p.y;
        if (std::abs(dy) > 1e-9)
            applied = pushCommand(std::make_unique<artplus::edit::MoveObjectCommand>(
                *m_editor, m_selectedObject, 0.0, dy));
    } else if (spin == m_propW) {
        if (p.width > 1e-6 && spin->value() > 0.0) {
            const double sx = spin->value() / p.width;
            if (std::abs(sx - 1.0) > 1e-9)
                applied = pushCommand(std::make_unique<artplus::edit::ScaleObjectCommand>(
                    *m_editor, m_selectedObject, p.x, p.y, sx, 1.0));
        }
    } else if (spin == m_propH) {
        if (p.height > 1e-6 && spin->value() > 0.0) {
            const double sy = spin->value() / p.height;
            if (std::abs(sy - 1.0) > 1e-9)
                applied = pushCommand(std::make_unique<artplus::edit::ScaleObjectCommand>(
                    *m_editor, m_selectedObject, p.x, p.y, 1.0, sy));
        }
    } else if (spin == m_propRotation) {
        const double delta = spin->value() - p.rotationDegrees;
        if (std::abs(delta) > 1e-9 &&
            pushCommand(std::make_unique<artplus::edit::RotateObjectCommand>(
                *m_editor, m_selectedObject, p.bounds.center().x(), p.bounds.center().y(),
                delta))) {
            applied = true;
        }
    }

    if (!applied)
        refreshProperties();
}

void MainWindow::selectObjectAt(const QPointF& pagePoint)
{
    if (!m_editor || !m_objectList)
        return;
    // PageView reports the click in displayed-page space; object bounds are
    // stored in PDF page space, so the point has to be mapped back.
    const QPointF pdfPoint = displayedPointToPdf(pagePoint);
    // Determine modifier keys: Shift adds to selection, Ctrl toggles selection.
    // Use Qt's keyboard modifiers which are available through the global app object.
    const Qt::KeyboardModifiers mods = QApplication::keyboardModifiers();
    bool shiftDown = mods.testFlag(Qt::ShiftModifier);
    bool ctrlDown = mods.testFlag(Qt::ControlModifier);

    int foundRow = -1;
    for (int i = m_editor->objectCount() - 1; i >= 0; --i) {
        const auto* object = m_editor->object(i);
        if (object && object->bounds.contains(pdfPoint)) {
            foundRow = i;
            break;
        }
    }

    if (foundRow < 0)
        return; // nothing clicked

    // --- Multi-selection logic ---
    if (ctrlDown) {
        // Ctrl+click: toggles the individual object in the selection vector
        if (m_selectedObjects.contains(foundRow))
            m_selectedObjects.erase(std::find(m_selectedObjects.begin(), m_selectedObjects.end(), foundRow));
        else
            m_selectedObjects.append(foundRow);
        std::sort(m_selectedObjects.begin(), m_selectedObjects.end());
        const auto last = std::unique(m_selectedObjects.begin(), m_selectedObjects.end());
        m_selectedObjects.erase(last, m_selectedObjects.end());
    }
    else if (shiftDown && !m_selectedObjects.isEmpty()) {
        // Shift+click: select range from last clicked to the previously selected row
        int first = qMin(foundRow, m_selectedObject);
        int last = qMax(foundRow, m_selectedObject);
        m_selectedObjects.clear();
        for (int i = first; i <= last; ++i)
            m_selectedObjects.append(i);
        std::sort(m_selectedObjects.begin(), m_selectedObjects.end());
        const auto lastUniq = std::unique(m_selectedObjects.begin(), m_selectedObjects.end());
        m_selectedObjects.erase(lastUniq, m_selectedObjects.end());
    }
    else {
        // Simple click (no modifiers): replace selection with just this object
        m_selectedObjects.clear();
        m_selectedObjects.append(foundRow);
    }

    // Update the single-select index and the list widget
    m_selectedObject = m_selectedObjects.isEmpty() ? -1 : m_selectedObjects.first();
    m_objectList->setCurrentRow(m_selectedObject);
    updateSelectionOverlay();

    // Notify dependents
    onObjectSelectionChanged();
}

void MainWindow::selectNextObject()
{
    if (!m_objectList || m_objectList->count() == 0)
        return;
    const int row = (m_objectList->currentRow() + 1) % m_objectList->count();
    m_objectList->setCurrentRow(row);
}

void MainWindow::selectPreviousObject()
{
    if (!m_objectList || m_objectList->count() == 0)
        return;
    int row = m_objectList->currentRow() - 1;
    if (row < 0)
        row = m_objectList->count() - 1;
    m_objectList->setCurrentRow(row);
}

void MainWindow::selectObjectRows(const QVector<int>& rows)
{
    if (!m_objectList)
        return;

    {
        // Apply the whole selection in one pass: the list signals are blocked so
        // onObjectSelectionChanged() does not run once per row.
        QSignalBlocker blocker(m_objectList);
        m_objectList->clearSelection();
        m_selectedObjects.clear();
        m_selectedObject = -1;

        for (int row : rows) {
            if (row < 0 || row >= m_objectList->count())
                continue;
            if (QListWidgetItem* item = m_objectList->item(row)) {
                item->setSelected(true);
                m_selectedObjects.append(row);
            }
        }

        std::sort(m_selectedObjects.begin(), m_selectedObjects.end());
        const auto last = std::unique(m_selectedObjects.begin(), m_selectedObjects.end());
        m_selectedObjects.erase(last, m_selectedObjects.end());
        if (!m_selectedObjects.isEmpty())
            m_selectedObject = m_selectedObjects.first();
    }

    refreshProperties();
    updateSelectionOverlay();
    updatePanels();

    if (statusBar()) {
        if (m_selectedObjects.isEmpty())
            statusBar()->showMessage(QStringLiteral("No objects selected"), 3000);
        else
            statusBar()->showMessage(
                QStringLiteral("%1 object(s) selected").arg(m_selectedObjects.size()), 4000);
    }
}

void MainWindow::selectAllObjects()
{
    if (!m_objectList)
        return;
    QVector<int> rows;
    rows.reserve(m_objectList->count());
    for (int i = 0; i < m_objectList->count(); ++i)
        rows.append(i);
    selectObjectRows(rows);
}

void MainWindow::selectInverseObjects()
{
    if (!m_objectList)
        return;
    QVector<int> rows;
    rows.reserve(m_objectList->count());
    for (int i = 0; i < m_objectList->count(); ++i) {
        if (!m_selectedObjects.contains(i))
            rows.append(i);
    }
    selectObjectRows(rows);
}

void MainWindow::selectSameFillColor()
{
    if (!m_editor || m_selectedObject < 0)
        return;
    const auto* current = m_editor->object(m_selectedObject);
    if (!current || !current->hasFillColor) {
        if (statusBar())
            statusBar()->showMessage(
                QStringLiteral("Selected object has no fill colour to match"), 4000);
        return;
    }

    const unsigned int target = current->fillColor;
    QVector<int> rows;
    for (int i = 0; i < m_editor->objectCount(); ++i) {
        const auto* object = m_editor->object(i);
        if (object && object->hasFillColor && object->fillColor == target)
            rows.append(i);
    }
    selectObjectRows(rows);
}

void MainWindow::selectSameStrokeColor()
{
    if (!m_editor || m_selectedObject < 0)
        return;
    const auto* current = m_editor->object(m_selectedObject);
    if (!current || !current->hasStrokeColor) {
        if (statusBar())
            statusBar()->showMessage(
                QStringLiteral("Selected object has no stroke colour to match"), 4000);
        return;
    }

    const unsigned int target = current->strokeColor;
    QVector<int> rows;
    for (int i = 0; i < m_editor->objectCount(); ++i) {
        const auto* object = m_editor->object(i);
        if (object && object->hasStrokeColor && object->strokeColor == target)
            rows.append(i);
    }
    selectObjectRows(rows);
}

void MainWindow::updatePanels()
{
    const bool hasDoc = hasDocument();

    const bool editable = m_editor && m_editor->isValid();
    const bool canNudge = editable && m_selectedObject >= 0;
    for (QAction* action : {m_moveLeft, m_moveRight, m_moveUp, m_moveDown}) {
        if (action)
            action->setEnabled(canNudge);
    }
    if (m_nextObjectAction)
        m_nextObjectAction->setEnabled(editable && m_editor->objectCount() > 0);
    if (m_prevObjectAction)
        m_prevObjectAction->setEnabled(editable && m_editor->objectCount() > 0);

    const bool dirty = m_history ? m_history->isModified()
                                 : (editable && m_editor->isModified());
    if (m_undoAction) {
        const QString name = (m_history && m_history->canUndo()) ? m_history->undoName() : QString();
        m_undoAction->setEnabled(m_history && m_history->canUndo());
        m_undoAction->setText(name.isEmpty() ? QStringLiteral("Undo")
                                            : QStringLiteral("Undo %1").arg(name));
    }
    if (m_redoAction) {
        const QString name = (m_history && m_history->canRedo()) ? m_history->redoName() : QString();
        m_redoAction->setEnabled(m_history && m_history->canRedo());
        m_redoAction->setText(name.isEmpty() ? QStringLiteral("Redo")
                                            : QStringLiteral("Redo %1").arg(name));
    }
    if (m_saveEditedAction)
        m_saveEditedAction->setEnabled(editable && dirty);
    if (m_objectPanel)
        m_objectPanel->setEnabled(false);
    if (m_transformAction)
        m_transformAction->setEnabled(editable && m_selectedObject >= 0);
    if (m_transformAgainAction)
        m_transformAgainAction->setEnabled(editable && m_history && m_history->hasLastTransform()
                                             && m_selectedObject >= 0);
    if (m_zoomSelectionAction)
        m_zoomSelectionAction->setEnabled(editable && m_selectedObject >= 0);

    if (m_zoomSelectionAction)
        m_zoomSelectionAction->setEnabled(editable && m_selectedObject >= 0);

    if (m_layersPanel)
        m_layersPanel->setEnabled(hasDoc);
    if (m_separationsPanel)
        m_separationsPanel->setEnabled(hasDoc);
    if (m_qualityPanel)
        m_qualityPanel->setEnabled(hasDoc);

    // Selection tools. "Same fill/stroke colour" needs a selected object that
    // actually carries paint of that kind, otherwise there is nothing to match.
    const bool hasObjects = editable && m_editor->objectCount() > 0;
    if (m_selectAllAction)
        m_selectAllAction->setEnabled(hasObjects);
    if (m_selectInverseAction)
        m_selectInverseAction->setEnabled(hasObjects);
    if (m_selectSameFillAction) {
        const auto* current = (hasObjects && m_selectedObject >= 0)
                                  ? m_editor->object(m_selectedObject)
                                  : nullptr;
        m_selectSameFillAction->setEnabled(current && current->hasFillColor);
    }
    if (m_selectSameStrokeAction) {
        const auto* current = (hasObjects && m_selectedObject >= 0)
                                  ? m_editor->object(m_selectedObject)
                                  : nullptr;
        m_selectSameStrokeAction->setEnabled(current && current->hasStrokeColor);
    }

    // Run preflight checks when document is open
    if (hasDocument() && m_runPreflightButton) {
        onRunPreflight();
    }
    if (m_documentPanel)
        m_documentPanel->setEnabled(true);

    if (m_layerName)
        m_layerName->setText(QStringLiteral("Layer 1"));
    if (m_layerInfo) {
        if (hasDoc && m_snapshot) {
            const int total = m_snapshot->objectCount();
            const int text = m_snapshot->countOf(artplus::pdf::PageObjectKind::Text);
            const int paths = m_snapshot->countOf(artplus::pdf::PageObjectKind::Path);
            const int images = m_snapshot->countOf(artplus::pdf::PageObjectKind::Image);
            const int shadings = m_snapshot->countOf(artplus::pdf::PageObjectKind::Shading);
            const int forms = m_snapshot->countOf(artplus::pdf::PageObjectKind::Form);
            const int unknown = m_snapshot->objectCount() - text - paths - images - shadings - forms;
            m_layerInfo->setText(QStringLiteral("%1 objects · %2 text · %3 paths · %4 images · %5 shading · %6 form · %7 unknown")
                                     .arg(total).arg(text).arg(paths).arg(images).arg(shadings).arg(forms).arg(unknown));
        } else {
            m_layerInfo->setText(QStringLiteral("no document"));
        }
    }
    if (m_profileValue) {
        const QString profile = (hasDoc && m_doc) ? m_doc->outputIntentName() : QString();
        QFont f = theme::uiFont(8);
        f.setUnderline(!profile.isEmpty());
        m_profileValue->setFont(f);
        QPalette pal = m_profileValue->palette();
        pal.setColor(QPalette::WindowText,
                     profile.isEmpty() ? theme::kText : QColor(0x1F, 0x4F, 0xBF));
        m_profileValue->setPalette(pal);
        m_profileValue->setText(profile.isEmpty() ? QStringLiteral("(not specified)") : profile);
    }

    if (m_messageTree) {
        m_messageTree->clear();
        if (hasDoc) {
            auto* other = new QTreeWidgetItem(m_messageTree);
            other->setText(0, QStringLiteral("Other"));
            other->setExpanded(true);
            auto* missing = new QTreeWidgetItem(other);
            missing->setText(0, QStringLiteral("Missing Real Data"));
            missing->setIcon(0, theme::icon(theme::IconKind::Messages, theme::kError, 13));
            missing->setText(1, QStringLiteral("2"));
        }
    }
}

void MainWindow::updateWindowTitle()
{
    if (!hasDocument()) {
        setWindowTitle(QStringLiteral("ArtPlus"));
        return;
    }
    const QString name = QFileInfo(m_doc->filePath()).fileName();
    const QString zoomText = QString::number(m_view->zoom() * 100.0, 'f', 1)
                                 .replace(QLatin1Char('.'), QLatin1Char(','));
    const bool dirty = (m_history && m_history->isModified()) ||
                       (!m_history && m_editor && m_editor->isModified());
    setWindowTitle(QStringLiteral("%1 @ %2%%3 - ArtPlus")
                       .arg(name, zoomText, dirty ? QStringLiteral(" *") : QString()));
}

double MainWindow::toDisplayUnits(double points) const
{
    switch (m_units) {
    case 1: return points;                    // pt
    case 2: return points / 72.0;             // in
    default: return points * 25.4 / 72.0;     // mm
    }
}

QString MainWindow::unitSuffix() const
{
    switch (m_units) {
    case 1: return QStringLiteral("pt");
    case 2: return QStringLiteral("in");
    default: return QStringLiteral("mm");
    }
}

void MainWindow::onCursorMoved(const QPointF& pagePoint)
{
    if (!m_cursorLabel)
        return;
    // The readout is a document position, so it must not change when only the
    // view is rotated: map the displayed point back to PDF page space.
    const QPointF pdfPoint = displayedPointToPdf(pagePoint);
    const int decimals = (m_units == 1) ? 1 : 2;
    const QString x = QString::number(toDisplayUnits(pdfPoint.x()), 'f', decimals)
                          .replace(QLatin1Char('.'), QLatin1Char(','));
    const QString y = QString::number(toDisplayUnits(pdfPoint.y()), 'f', decimals)
                          .replace(QLatin1Char('.'), QLatin1Char(','));
    m_cursorLabel->setText(QStringLiteral("X: %1   Y: %2 %3").arg(x, y, unitSuffix()));
}

void MainWindow::updateUi()
{
    const bool hasDoc = hasDocument();
    const int pages = hasDoc ? m_doc->pageCount() : 0;

    m_suppressSpinSignal = true;
    m_pageSpin->setRange(1, std::max(1, pages));
    m_pageSpin->setValue(std::clamp(m_page + 1, 1, std::max(1, pages)));
    m_pageSpin->setEnabled(hasDoc);
    m_suppressSpinSignal = false;

    updateStatus();
    updateWindowTitle();
}

void MainWindow::updateStatus()
{
    const int pages = hasDocument() ? m_doc->pageCount() : 0;
    const QString zoomText = QString::number(m_view->zoom() * 100.0, 'f', 1)
                                 .replace(QLatin1Char('.'), QLatin1Char(',')) + QStringLiteral("%");

    if (hasDocument() && m_page >= 0 && m_page < pages) {
        const QSizeF sz = displayedPageSize(m_page);
        const int decimals = (m_units == 1) ? 1 : 2;
        const QString w = QString::number(toDisplayUnits(sz.width()), 'f', decimals)
                              .replace(QLatin1Char('.'), QLatin1Char(','));
        const QString h = QString::number(toDisplayUnits(sz.height()), 'f', decimals)
                              .replace(QLatin1Char('.'), QLatin1Char(','));
        m_pageInfoLabel->setText(QStringLiteral("%1 of %2   %3 x %4 %5")
                                     .arg(m_page + 1)
                                     .arg(pages)
                                     .arg(w, h, unitSuffix()));
    } else {
        m_pageInfoLabel->setText(QStringLiteral("0 of 0"));
        if (m_cursorLabel)
            m_cursorLabel->setText(QStringLiteral("X: —   Y: —"));
    }

    m_zoomLabel->setText(zoomText);
    updateWindowTitle();
}

void MainWindow::showError(const QString& title, const QString& message)
{
    logError(message);
    QMessageBox::critical(this, title, message);
}

void MainWindow::onNoDocument()
{
    resetDocumentState();
    updateUi();
    updatePanels();
    updateWindowTitle();
}

void MainWindow::onPageListChanged(int) {}

void MainWindow::onRunPreflight()
{
    if (!hasDocument()) {
        statusBar()->showMessage(QStringLiteral("No document open for preflight check"), 3000);
        return;
    }

    // Build profile from user settings
    artplus::preflight::Profile profile;
    profile.tacLimit = m_tacLimit->value();
    profile.enabledChecks.clear();

    QComboBox* combo = m_qualityPanel->findChild<QComboBox*>();
    if (combo) {
        int selectedIndex = combo->currentIndex();
        // Map combo box index to check categories
        switch (selectedIndex) {
        case 0: profile.enabledChecks << artplus::preflight::CheckCategory::Color; break;
        case 1: profile.enabledChecks << artplus::preflight::CheckCategory::PageBoxes; break;
        case 2: profile.enabledChecks << artplus::preflight::CheckCategory::Separations; break;
        case 3: profile.enabledChecks << artplus::preflight::CheckCategory::Font; break;
        case 4: profile.enabledChecks << artplus::preflight::CheckCategory::Image; break;
        case 5: profile.enabledChecks << artplus::preflight::CheckCategory::Resolution; break;
        default: break;
        }
    }

    // Run checks. The editor provides the real object paint colours, which is
    // what the TAC estimate is derived from.
    auto [results, updatedProfile] =
        artplus::preflight::checkDocument(*m_doc, m_editor.get(), profile);
    profile = updatedProfile;

    // Display results
    m_preflightResults->clear();
    for (const auto& result : results) {
        QListWidgetItem* item = new QListWidgetItem(m_preflightResults);
        item->setText(result.title);
        item->setToolTip(result.description + QLatin1String("\n") + result.details);
        switch (result.severity) {
        case artplus::preflight::Severity::Error:
            item->setForeground(QColor(0xCC, 0x00, 0x00));
            break;
        case artplus::preflight::Severity::Warning:
            item->setForeground(QColor(0xCC, 0x88, 0x00));
            break;
        case artplus::preflight::Severity::Info:
            item->setForeground(QColor(0x00, 0x5F, 0xB8));
            break;
        }
    }

    // Update status bar
    int errorCount = 0, warningCount = 0, infoCount = 0;
    for (const auto& r : results) {
        switch (r.severity) {
        case artplus::preflight::Severity::Error: errorCount++; break;
        case artplus::preflight::Severity::Warning: warningCount++; break;
        case artplus::preflight::Severity::Info: infoCount++; break;
        }
    }
    QString statusMsg;
    if (errorCount > 0)
        statusMsg = QStringLiteral("%1 error(es), %2 avisos, %3 informações").arg(errorCount).arg(warningCount).arg(infoCount);
    else if (warningCount > 0)
        statusMsg = QStringLiteral("%1 aviso(s), %2 informações").arg(warningCount).arg(infoCount);
    else
        statusMsg = QStringLiteral("%1 informação(n)").arg(infoCount);
    statusBar()->showMessage(statusMsg, 5000);
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData() && event->mimeData()->hasUrls()) {
        for (const QUrl& url : event->mimeData()->urls()) {
            if (url.isLocalFile() &&
                url.toLocalFile().endsWith(QLatin1String(".pdf"), Qt::CaseInsensitive)) {
                event->acceptProposedAction();
                return;
            }
        }
    }
    event->ignore();
}

void MainWindow::dropEvent(QDropEvent* event)
{
    if (!event->mimeData() || !event->mimeData()->hasUrls()) {
        event->ignore();
        return;
    }
    for (const QUrl& url : event->mimeData()->urls()) {
        if (!url.isLocalFile())
            continue;
        const QString path = url.toLocalFile();
        if (path.endsWith(QLatin1String(".pdf"), Qt::CaseInsensitive)) {
            event->acceptProposedAction();
            openDocument(path);
            return;
        }
    }
    event->ignore();
}

} // namespace artplus::ui
