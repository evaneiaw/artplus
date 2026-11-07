#include "artplus/document/Editor.h"

#include <QDebug>
#include <QCoreApplication>

namespace artplus::document {

Editor::Editor(ArtDocument& document, QObject* parent)
    : QObject(parent), m_document(document)
{
}

Editor::~Editor()
{
}

// ======================================================================
// Seleção
// ======================================================================

void Editor::selectObject(int index)
{
    if (m_selected.contains(index)) {
        m_selected.removeOne(index);
    } else {
        m_selected.append(index);
    }
    emit selectionChanged();
}

void Editor::deselectAll()
{
    m_selected.clear();
    emit selectionChanged();
}

QVector<int> Editor::selectedObjects() const
{
    return m_selected;
}

// ======================================================================
// Transformações sobre a seleção atual
// ======================================================================

void Editor::translateSelected(double dx, double dy)
{
    saveState();
    QVector<int> selected = selectedObjects();
    for (int idx : selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        GraphicObject* obj = layer->findObject(/* id */);
        if (!obj) continue;
        // TODO: aplicar translação ao objeto
    }
    emit selectionChanged();
    emit objectModified(selected.first());
}

void Editor::rotateSelected(double degrees)
{
    saveState();
    QVector<int> selected = selectedObjects();
    for (int idx : selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        GraphicObject* obj = layer->findObject(/* id */);
        if (!obj) continue;
        // TODO: aplicar rotação ao objeto
    }
    emit selectionChanged();
    emit objectModified(selected.first());
}

void Editor::scaleSelected(double factor)
{
    saveState();
    QVector<int> selected = selectedObjects();
    for (int idx : selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        GraphicObject* obj = layer->findObject(/* id */);
        if (!obj) continue;
        // TODO: aplicar escala ao objeto
    }
    emit selectionChanged();
    emit objectModified(selected.first());
}

void Editor::setTransform(double a, double b, double c, double d, double e, double f)
{
    saveState();
    QVector<int> selected = selectedObjects();
    for (int idx : selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        GraphicObject* obj = layer->findObject(/* id */);
        if (!obj) continue;
        // TODO: aplicar transforma��o ao objeto
    }
    emit selectionChanged();
    emit objectModified(selected.first());
}

// ======================================================================
// Step & Repeat / Imposição
// ======================================================================

void Editor::stepRepeat(int count, double dx, double dy)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    for (int i = 0; i < count; ++i)
    {
        for (int idx : selected)
        {
            ArtLayer* layer = m_document.page(0)->layer(idx);
            if (!layer) continue;
            GraphicObject* obj = layer->findObject(/* id */);
            if (!obj) continue;

            // Aplicar translação acumulada
            double newX = obj->transform().a + dx * (i + 1);
            double newY = obj->transform().d + dy * (i + 1);
            // TODO: aplicar nova posição ao objeto
        }
    }
    emit selectionChanged();
    emit objectModified(selected.first());
}

void Editor::stepRepeatRectangular(int countX, int countY, double spacingX, double spacingY)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    for (int y = 0; y < countY; ++y)
    {
        for (int x = 0; x < countX; ++x)
        {
            for (int idx : selected)
            {
                ArtLayer* layer = m_document.page(0)->layer(idx);
                if (!layer) continue;
                GraphicObject* obj = layer->findObject(/* id */);
                if (!obj) continue;

                // Calcular posição da réplica na grade
                double baseX = obj->transform().a;
                double baseY = obj->transform().d;
                double newX = baseX + x * spacingX;
                double newY = baseY + y * spacingY;
                // TODO: criar réplica ou repositionar objeto
            }
        }
    }
    emit selectionChanged();
    emit objectModified(selected.first());
}

void Editor::stepRepeatHexagonal(int count, double radius, double angle)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    for (int i = 0; i < count; ++i)
    {
        double theta = i * angle;
        double dx = radius * cos(theta);
        double dy = radius * sin(theta);

        for (int idx : selected)
        {
            ArtLayer* layer = m_document.page(0)->layer(idx);
            if (!layer) continue;
            GraphicObject* obj = layer->findObject(/* id */);
            if (!obj) continue;

            // Aplicar deslocamento hexagonal
            double newX = obj->transform().a + dx;
            double newY = obj->transform().d + dy;
            // TODO: aplicar nova posição ao objeto
        }
    }
    emit selectionChanged();
    emit objectModified(selected.first());
}

void Editor::imposeLayout(QString layoutType, int copiesX, int copiesY, double gutterX, double gutterY)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // Implementação baseada no tipo de layout
    if (layoutType == QStringLiteral("grid"))
    {
        stepRepeatRectangular(copiesX, copiesY, gutterX, gutterY);
    }
    else if (layoutType == QStringLiteral("hexagonal"))
    {
        stepRepeatHexagonal(copiesX, 50.0, 60.0); // radius=50, angle=60 degrees
    }

    emit selectionChanged();
    emit objectModified(selected.first());
}

// ======================================================================
// Marks Engine (M29)
// ======================================================================

// Desenhar marca de registro (registration mark)
void Editor::drawRegistrationMark(const QString& markType, const QRectF& rect, const QColor& color, double lineWidth)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // Criar objeto de marca temporário
    for (int idx : selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        // TODO: adicionar objeto de marca à layer
    }

    emit selectionChanged();
    emit objectModified(selected.first());
}

// Desenhar marca de corte (cut mark)
void Editor::drawCutMark(const QString& markType, const QRectF& rect, const QColor& color, double lineWidth)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // Criar objeto de marca de corte
    for (int idx : selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        // TODO: adicionar objeto de corte à layer
    }

    emit selectionChanged();
    emit objectModified(selected.first());
}

// Desenhar marca de sangria (bleed mark)
void Editor::drawBleedMark(const QString& markType, const QRectF& rect, const QColor& color, double lineWidth)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // Criar objeto de marca de sangria
    for (int idx : selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        // TODO: adicionar objeto de sangria à layer
    }

    emit selectionChanged();
    emit objectModified(selected.first());
}

// Desenhar barra de cor (color bar)
void Editor::drawColorBar(const QString& markType, const QRectF& rect, const QColor& color, double lineWidth)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // Criar objeto de barra de cor
    for (int idx : selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        // TODO: adicionar barra de cor à layer
    }

    emit selectionChanged();
    emit objectModified(selected.first());
}

// Desenhar marca de controle (control mark)
void Editor::drawControlMark(const QString& markType, const QPointF& position, const QColor& color, double size)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // Criar objeto de marca de controle
    for (int idx : selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        // TODO: adicionar marca de controle à layer
    }

    emit selectionChanged();
    emit objectModified(selected.first());
}

// Gerar todas as marcas de impressão para um layout
void Editor::generatePrintMarks(QString markSet, int copiesX, int copiesY, double gutterX, double gutterY)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // Parse do markSet para determinar quais marcas gerar
    // markSet pode conter: "registration", "cut", "bleed", "colorbars", etc.

    if (markSet.contains(QStringLiteral("registration"))) {
        drawRegistrationMark(QStringLiteral("registration"), QRectF(0, 0, 5, 5), Qt::black, 0.5);
    }
    if (markSet.contains(QStringLiteral("cut"))) {
        drawCutMark(QStringLiteral("cut"), QRectF(0, 0, 10, 10), Qt::black, 0.5);
    }
    if (markSet.contains(QStringLiteral("bleed"))) {
        drawBleedMark(QStringLiteral("bleed"), QRectF(0, 0, 5, 5), Qt::black, 0.5);
    }
    if (markSet.contains(QStringLiteral("colorbars"))) {
        drawColorBar(QStringLiteral("colorbar"), QRectF(0, 0, 20, 1), Qt::black, 0.5);
    }

    emit selectionChanged();
    emit objectModified(selected.first());
}

// ======================================================================
// Undo/Redo
// ======================================================================

void Editor::undo()
{
    // TODO: implementar undo
}

void Editor::redo()
{
    // TODO: implementar redo
}

// ======================================================================
// Properties
// ======================================================================

bool Editor::isModified() const
{
    // TODO: implementar
    return false;
}

// ======================================================================
// Group/Ungroup
// ======================================================================

void Editor::groupSelected()
{
    // TODO: implementar group
}

void Editor::ungroupSelected()
{
    // TODO: implementar ungroup
}

// ======================================================================
// Z-Order
// ======================================================================

void Editor::bringToFront()
{
    // TODO: implementar bringToFront
}

void Editor::sendToBack()
{
    // TODO: implementar sendToBack
}

void Editor::bringForward()
{
    // TODO: implementar bringForward
}

void Editor::sendBackward()
{
    // TODO: implementar sendBackward
}

// ======================================================================
// Clipboard
// ======================================================================

void Editor::copyAvailable(bool available)
{
    Q_UNUSED(available);
}

void Editor::pasteAvailable(bool available)
{
    Q_UNUSED(available);
}

// ======================================================================
// Implementa��o inline dos m�todos principais
// ======================================================================

inline void Editor::saveState()
{
    if (m_selected.isEmpty()) return;

    m_undoState.hasTransform = false;
    m_undoCount++;
    emit undoAvailable(true);
}

inline void Editor::applyState(struct TransformationState& state)
{
    if (m_selected.isEmpty() || !state.hasTransform) return;
    m_redoState = state;
    m_redoCount++;
    emit redoAvailable(true);
}

} // namespace artplus::document