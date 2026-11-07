#pragma once

#include "artplus/document/ArtDocument.h"
#include "artplus/document/ArtLayer.h"
#include "artplus/document/ArtPage.h"
#include "artplus/objects/GraphicObject.h"

#include <QObject>
#include <QVector>
#include <QString>
#include <QDebug>

namespace artplus::document {

// Forward declarations
class ArtDocument;
class ArtLayer;
class ArtPage;
class GraphicObject;

// ======================================================================
// Classe Editor — gestão de seleção e transformações de objetos
// ======================================================================

class Editor : public QObject
{
    Q_OBJECT

public:
    explicit Editor(ArtDocument& document, QObject* parent = nullptr);
    ~Editor();

    // Seleção
    void selectObject(int index);
    void deselectAll();
    QVector<int> selectedObjects() const;

    // Transformações sobre a seleção atual
    void translateSelected(double dx, double dy);
    void rotateSelected(double degrees);
    void scaleSelected(double factor);
    void setTransform(double a, double b, double c, double d, double e, double f);

    // Step & Repeat / Imposição
    void stepRepeat(int count, double dx, double dy);
    void stepRepeatRectangular(int countX, int countY, double spacingX, double spacingY);
    void stepRepeatHexagonal(int count, double radius, double angle);
    void imposeLayout(QString layoutType, int copiesX, int copiesY, double gutterX, double gutterY);

    // Undo/Redo
    void undo();
    void redo();

    // Properties
    bool isModified() const;

signals:
    void selectionChanged();
    void objectModified(int index);
    void undoAvailable(bool available);
    void redoAvailable(bool available);

    // Clipboard operations
    void copyAvailable(bool available);
    void pasteAvailable(bool available);

private:
    ArtDocument& m_document;
    QVector<int> m_selected;          // índices dos objetos selecionados
    struct TransformationState {
        double a, b, c, d, e, f;  // matriz afim acumulada
        bool hasTransform;
    } m_undoState, m_redoState;
    int m_undoCount = 0;
    int m_redoCount = 0;

    // Gerar snapshot do estado atual para undo
    void saveState();
    // Aplicar snapshot salvo
    void applyState(struct TransformationState& state);
};

// ======================================================================
// Implementação inline dos métodos principais
// ======================================================================

inline void Editor::saveState()
{
    // Salva o estado atual da transformação acumulada dos objetos selecionados
    // para possível undo
    if (m_selected.isEmpty()) return;

    TransformationState state;
    state.hasTransform = false;
    for (int idx : m_selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        GraphicObject* obj = layer->findObject(/* id */);
        if (!obj) continue;
        // ... salvar estado do objeto
    }
    m_undoState = state;
    m_undoCount++;
    emit undoAvailable(true);
}

inline void Editor::applyState(struct TransformationState& state)
{
    // Aplica o snapshot salvo aos objetos selecionados
    if (m_selected.isEmpty() || !state.hasTransform) return;
    for (int idx : m_selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        GraphicObject* obj = layer->findObject(/* id */);
        if (!obj) continue;
        // Aplicar transformação...
    }
    m_redoState = state;
    m_redoCount++;
    emit redoAvailable(true);
}

Editor::Editor(ArtDocument& document, QObject* parent)
    : QObject(parent), m_document(document)
{
    m_selected.clear();
}

Editor::~Editor() {}

void Editor::selectObject(int index)
{
    // Lógica de seleção: adiciona/remove do conjunto selecionado
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
    // Aplicar translação aos objetos selecionados
    for (int idx : m_selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        GraphicObject* obj = layer->findObject(/* id */);
        if (!obj) continue;
        // Aplicar transformação de translação
        // obj->applyAffineTransform(1, 0, 0, 1, dx, dy);
    }
    emit objectModified(m_selected.first());
    // Reset redo state when new action performed
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::rotateSelected(double degrees)
{
    saveState();
    // Aplicar rotação aos objetos selecionados
    // ... implementação
    emit objectModified(m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::scaleSelected(double factor)
{
    saveState();
    // Aplicar escala aos objetos selecionados
    // ... implementação
    emit objectModified(m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::setTransform(double a, double b, double c, double d, double e, double f)
{
    saveState();
    // Aplicar transformação afim completa
    // for (int idx : m_selected) { ... }
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
    // Verificar se há modificações não salvas
    return m_undoCount > 0 || m_redoCount > 0;
}

// ======================================================================
// Clipboard — copy, cut e paste
// ======================================================================

// Copiar objetos selecionados para a área de transferência
void Editor::copySelected();

// Recortar objetos selecionados (remover e copiar)
void Editor::cutSelected();

// Colar objetos da área de transferência
void Editor::paste();

// ======================================================================
// Inspector e Propriedades do Objeto
// ======================================================================

// Atualizar propriedades do objeto selecionado
void Editor::updateProperties();

// Definir cor de preenchimento (0xAARRGGBB)
void Editor::setFillColor(unsigned int color);

// Definir cor de traço (0xAARRGGBB)
void Editor::setStrokeColor(unsigned int color);

// Definir largura do traço
void Editor::setStrokeWidth(double width);

// Definir opacidade (0.0 a 1.0)
void Editor::setOpacity(double opacity);

// Definir visibilidade
void Editor::setVisible(bool visible);

// Definir bloqueado
void Editor::setLocked(bool locked);

// ======================================================================
// Group/Ungroup — agrupar e desagrupar objetos selecionados
// ======================================================================

// Agrupar objetos selecionados em um novo grupo
void Editor::groupSelected();

// Desagrupar um grupo selecionado, retornando seus objetos ao estado individual
void Editor::ungroupSelected();

// ======================================================================
// Z-Order — reordenação de objetos na hierarquia
// ======================================================================

// Trazer objeto selecionado para o primeiro plano (topo da hierarquia)
void Editor::bringToFront();

// Enviar objeto selecionado para o fundo (fundo da hierarquia)
void Editor::sendToBack();

// Trazer objeto selecionado um nível para frente
void Editor::bringForward();

// Enviar objeto selecionado um nível para trás
void Editor::sendBackward();

} // namespace artplus::document