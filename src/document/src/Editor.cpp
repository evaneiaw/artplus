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