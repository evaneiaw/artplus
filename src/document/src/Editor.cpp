#include "artplus/document/Editor.h"

#include <QDebug>

namespace artplus::document {

Editor::Editor(ArtDocument& document, QObject* parent)
    : QObject(parent), m_document(document)
{
    m_selected.clear();
    m_undoState.hasTransform = false;
    m_redoState.hasTransform = false;
    m_undoCount = 0;
    m_redoCount = 0;
}

Editor::~Editor()
{
}

void Editor::saveState()
{
    if (m_selected.isEmpty()) return;
    m_undoState.hasTransform = false;
    // TODO: salvar estado real dos objetos selecionados
    // Isso seria uma captura profunda dos objetos transformados
    m_undoCount++;
    emit undoAvailable(m_undoCount > 0);
}

void Editor::applyState(struct TransformationState& state)
{
    if (m_selected.isEmpty() || !state.hasTransform) return;
    // TODO: aplicar transformação salva aos objetos selecionados
    m_redoCount++;
    emit redoAvailable(m_redoCount > 0);
}

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

void Editor::translateSelected(double dx, double dy)
{
    saveState();
    // TODO: aplicar translação dx, dy aos objetos selecionados
    // for (int idx : m_selected) { ArtLayer* layer = ...; GraphicObject* obj = ...; ... }
    emit objectModified(m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::rotateSelected(double degrees)
{
    saveState();
    // TODO: aplicar rotação aos objetos selecionados
    emit objectModified(m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::alignHorizontalLeft()
{
    saveState();
    if (m_selected.size() < 2) return;
    // Encontrar o mínimo x entre os objetos selecionados
    double minX = 1e100;
    for (int idx : m_selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        GraphicObject* obj = layer->findObject(/* id */);
        if (!obj) continue;
        minX = qMin(minX, obj->localBounds().left);
    }
    // Mover todos os objetos para que o de menor x fique em minX (já está)
    // então alinhar os demais ao mínimo
    for (int idx : m_selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        GraphicObject* obj = layer->findObject(/* id */);
        if (!obj) continue;
        // Ajustar x para minX (já está alinhado, mas podemos forçar)
    }
    emit objectModified(m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::alignHorizontalRight()
{
    saveState();
    if (m_selected.size() < 2) return;
    double maxX = -1e100;
    for (int idx : m_selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        GraphicObject* obj = layer->findObject(/* id */);
        if (!obj) continue;
        maxX = qMax(maxX, obj->localBounds().right);
    }
    emit objectModified(m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::alignHorizontalCenter()
{
    saveState();
    if (m_selected.size() < 2) return;
    double centerX = 0;
    int count = 0;
    for (int idx : m_selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        GraphicObject* obj = layer->findObject(/* id */);
        if (!obj) continue;
        centerX += obj->localBounds().left + obj->localBounds().width() / 2.0;
        count++;
    }
    if (count > 0) centerX /= count;
    emit objectModified(m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::alignVerticalTop()
{
    saveState();
    if (m_selected.size() < 2) return;
    double minY = 1e100;
    for (int idx : m_selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        GraphicObject* obj = layer->findObject(/* id */);
        if (!obj) continue;
        minY = qMin(minY, obj->localBounds().top);
    }
    emit objectModified(m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::alignVerticalBottom()
{
    saveState();
    if (m_selected.size() < 2) return;
    double maxY = -1e100;
    for (int idx : m_selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        GraphicObject* obj = layer->findObject(/* id */);
        if (!obj) continue;
        maxY = qMax(maxY, obj->localBounds().bottom);
    }
    emit objectModified(m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::alignVerticalCenter()
{
    saveState();
    if (m_selected.size() < 2) return;
    double centerY = 0;
    int count = 0;
    for (int idx : m_selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        GraphicObject* obj = layer->findObject(/* id */);
        if (!obj) continue;
        centerY += obj->localBounds().top + obj->localBounds().height() / 2.0;
        count++;
    }
    if (count > 0) centerY /= count;
    emit objectModified(m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::distributeHorizontal()
{
    saveState();
    if (m_selected.size() < 3) return;
    // Ordenar objetos pelo centro x
    QVector<double> centers;
    centers.reserve(m_selected.size());
    for (int idx : m_selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        GraphicObject* obj = layer->findObject(/* id */);
        if (!obj) continue;
        centers.append(obj->localBounds().left + obj->localBounds().width() / 2.0);
    }
    // O objeto de extremidade esquerda e direita já definem o intervalo
    // Espaço a ser dividido: (n-1) intervalos entre n objetos
    if (centers.size() < 2) return;
    double minCenter = *std::min_element(centers.begin(), centers.end());
    double maxCenter = *std::max_element(centers.begin(), centers.end());
    double span = maxCenter - minCenter;
    double interval = span / (centers.size() - 1);
    
    // Mover cada objeto para sua posição alinhada
    for (int i = 0; i < (int)m_selected.size(); ++i) {
        int idx = m_selected[i];
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        GraphicObject* obj = layer->findObject(/* id */);
        if (!obj) continue;
        double newCenter = minCenter + i * interval;
        // Calcular novo x: center - width/2
        double width = obj->localBounds().width();
        double newX = newCenter - width / 2.0;
        // TODO: aplicar nova posição x ao objeto
    }
    emit objectModified(m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::distributeVertical()
{
    saveState();
    if (m_selected.size() < 3) return;
    QVector<double> centers;
    centers.reserve(m_selected.size());
    for (int idx : m_selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        GraphicObject* obj = layer->findObject(/* id */);
        if (!obj) continue;
        centers.append(obj->localBounds().top + obj->localBounds().height() / 2.0);
    }
    if (centers.size() < 2) return;
    double minCenter = *std::min_element(centers.begin(), centers.end());
    double maxCenter = *std::max_element(centers.begin(), centers.end());
    double span = maxCenter - minCenter;
    double interval = span / (centers.size() - 1);
    
    for (int i = 0; i < (int)m_selected.size(); ++i) {
        int idx = m_selected[i];
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        GraphicObject* obj = layer->findObject(/* id */);
        if (!obj) continue;
        double newCenter = minCenter + i * interval;
        double height = obj->localBounds().height();
        double newY = newCenter - height / 2.0;
        // TODO: aplicar nova posição y ao objeto
    }
    emit objectModified(m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::scaleSelected(double factor)
{
    saveState();
    // TODO: aplicar escala factor aos objetos selecionados
    emit objectModified(m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::setTransform(double a, double b, double c, double d, double e, double f)
{
    saveState();
    // TODO: aplicar transformação afim completa aos objetos selecionados
    emit objectModified(m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::undo()
{
    if (m_undoCount > 0) {
        applyState(m_undoState);
        m_undoCount--;
        m_redoCount++;
        emit undoAvailable(m_undoCount > 0);
        emit redoAvailable(m_redoCount > 0);
    }
}

void Editor::redo()
{
    if (m_redoCount > 0) {
        applyState(m_redoState);
        m_redoCount--;
        m_undoCount++;
        emit undoAvailable(m_undoCount > 0);
        emit redoAvailable(m_redoCount > 0);
    }
}

bool Editor::isModified() const
{
    return m_undoCount > 0 || m_redoCount > 0;
}

} // namespace artplus::document