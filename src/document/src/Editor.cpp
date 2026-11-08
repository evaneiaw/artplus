#include "artplus/document/Editor.h"

#include "artplus/document/ArtDocument.h"
#include "artplus/document/ArtLayer.h"
#include "artplus/document/ArtPage.h"
#include "artplus/objects/GraphicObject.h"
#include "artplus/objects/PathObject.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDateTime>

#include <algorithm>

namespace artplus::document {

// ======================================================================
// Helper: apply affine transform to a GraphicObject
// ======================================================================

static void applyObjectAffineTransform(GraphicObject* obj, double a, double b, double c, double d, double e, double f)
{
    if (!obj) return;
    // Compor a nova matriz: nova = antiga * transformacao
    // Na convenção PDF: x' = a*x + c*y + e, y' = b*x + d*y + f
    // Nova matriz = matriz_atual * transformacao_nova
    geometry::Matrix current = obj->transform();
    geometry::Matrix nova;
    
    // Matriz de transformação atual: [a c e]
    //                                   [b d f]
    //                                   [0 0 1]
    // Nova transformação: [a c e]
    //                     [b d f]
    //                     [0 0 1]
    // Nova = atual * nova_transformação
    // Multiplicação correta: current * new_transform
    // [a c e] * [a c e] = [a*a' + c*b'   a*c' + c*d'   a*e' + c*f' + e]
    // [b d f]   [b d f]   [b*a' + d*b'   b*c' + d*d'   b*e' + d*f' + f]
    nova.a = current.a * a + current.c * b;
    nova.b = current.b * a + current.d * b;
    nova.c = current.a * c + current.c * d;
    nova.d = current.b * c + current.d * d;
    nova.e = current.a * e + current.c * f + current.e;
    nova.f = current.b * e + current.d * f + current.f;
    
    obj->setTransform(nova);
}

// ======================================================================
// Editor implementation
// ======================================================================

Editor::Editor(ArtDocument& document, QObject* parent)
    : QObject(parent), m_document(document)
{
    m_selected.clear();
    m_undoState.hasTransform = false;
    m_redoState.hasTransform = false;
    m_undoCount = 0;
    m_redoCount = 0;
}

Editor::~Editor() {}

// ======================================================================
// Seleção
// ======================================================================

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

// ======================================================================
// Hit-testing / object access
// ======================================================================

int Editor::objectCount() const
{
    int count = 0;
    if (ArtPage* page = m_document.page(0)) {
        for (int i = 0; i < page->layerCount(); ++i) {
            if (ArtLayer* layer = page->layer(i)) {
                count += layer->objectCount();
            }
        }
    }
    return count;
}

GraphicObject* Editor::object(int globalIndex)
{
    if (globalIndex < 0)
        return nullptr;
    
    int currentIndex = 0;
    if (ArtPage* page = m_document.page(0)) {
        // Iterate layers in visual order (bottom to top = layer 0 to layer N)
        for (int i = 0; i < page->layerCount(); ++i) {
            if (ArtLayer* layer = page->layer(i)) {
                int layerObjCount = layer->objectCount();
                if (globalIndex < currentIndex + layerObjCount) {
                    // Object is in this layer
                    const auto& objects = layer->objects();
                    int localIndex = globalIndex - currentIndex;
                    if (localIndex >= 0 && localIndex < (int)objects.size()) {
                        return objects[localIndex].get();
                    }
                    return nullptr;
                }
                currentIndex += layerObjCount;
            }
        }
    }
    return nullptr;
}

const GraphicObject* Editor::object(int globalIndex) const
{
    return const_cast<Editor*>(this)->object(globalIndex);
}

// Helper: collect all objects with their global indices
QVector<std::pair<int, GraphicObject*>> Editor::collectAllObjectsWithIndices() const
{
    QVector<std::pair<int, GraphicObject*>> result;
    int globalIndex = 0;
    
    if (ArtPage* page = m_document.page(0)) {
        for (int i = 0; i < page->layerCount(); ++i) {
            if (ArtLayer* layer = page->layer(i)) {
                const auto& objects = layer->objects();
                for (int j = 0; j < (int)objects.size(); ++j) {
                    result.append({globalIndex, objects[j].get()});
                    ++globalIndex;
                }
            }
        }
    }
    return result;
}

// ======================================================================
// Transformations
// ======================================================================

void Editor::translateSelected(double dx, double dy)
{
    saveState();
    
    auto objects = collectAllObjectsWithIndices();
    for (const auto& pair : objects) {
        int globalIdx = pair.first;
        GraphicObject* obj = pair.second;
        
        if (!m_selected.contains(globalIdx) || !obj)
            continue;
        
        // Apply translation
        applyObjectAffineTransform(obj, 1, 0, 0, 1, dx, dy);
    }
    
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::rotateSelected(double degrees)
{
    saveState();
    
    double rad = degrees * 3.14159265358979323846 / 180.0;
    double c = cos(rad);
    double s = sin(rad);
    
    auto objects = collectAllObjectsWithIndices();
    for (const auto& pair : objects) {
        int globalIdx = pair.first;
        GraphicObject* obj = pair.second;
        
        if (!m_selected.contains(globalIdx) || !obj)
            continue;
        
        // Apply rotation around object center
        // Get object center in local coordinates
        geometry::Rect bounds = obj->bounds();
        geometry::Point center = bounds.center();
        
        // Translate to origin, rotate, translate back
        // T(-center) * R * T(center)
        double cx = center.x;
        double cy = center.y;
        
        double a = cos(rad);
        double b = sin(rad);
        double c = -sin(rad);
        double d = cos(rad);
        double e = cx - (a * cx + c * cy);
        double f = cy - (b * cx + d * cy);
        
        applyObjectAffineTransform(obj, a, b, c, d, e, f);
    }
    
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::scaleSelected(double factor)
{
    saveState();
    
    auto objects = collectAllObjectsWithIndices();
    for (const auto& pair : objects) {
        int globalIdx = pair.first;
        GraphicObject* obj = pair.second;
        
        if (!m_selected.contains(globalIdx) || !obj)
            continue;
        
        // Scale around object center
        geometry::Rect bounds = obj->bounds();
        geometry::Point center = bounds.center();
        double cx = center.x;
        double cy = center.y;
        
        double e = cx - factor * cx;
        double f = cy - factor * cy;
        
        applyObjectAffineTransform(obj, factor, 0, 0, factor, e, f);
    }
    
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::setTransform(double a, double b, double c, double d, double e, double f)
{
    saveState();
    
    auto objects = collectAllObjectsWithIndices();
    for (const auto& pair : objects) {
        int globalIdx = pair.first;
        GraphicObject* obj = pair.second;
        
        if (!m_selected.contains(globalIdx) || !obj)
            continue;
        
        applyObjectAffineTransform(obj, a, b, c, d, e, f);
    }
    
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

// ======================================================================
// Step & Repeat / Imposição
// ======================================================================

void Editor::stepRepeat(int count, double dx, double dy)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // Get the layer of the first selected object to add copies there
    int firstGlobalIdx = selected.first();
    GraphicObject* firstObj = object(firstGlobalIdx);
    if (!firstObj) return;
    
    // Find the layer containing the first selected object
    ArtLayer* targetLayer = nullptr;
    if (ArtPage* page = m_document.page(0)) {
        for (int i = 0; i < page->layerCount(); ++i) {
            if (ArtLayer* layer = page->layer(i)) {
                const auto& objects = layer->objects();
                for (const auto& obj : objects) {
                    if (obj.get() == firstObj) {
                        targetLayer = layer;
                        break;
                    }
                }
                if (targetLayer) break;
            }
    }
    
    if (!targetLayer) return;

    // For each copy iteration
    for (int i = 0; i < count; ++i) {
        double accumX = dx * (i + 1);
        double accumY = dy * (i + 1);
        
        // For each selected object
        for (int globalIdx : selected) {
            GraphicObject* obj = object(globalIdx);
            if (!obj) continue;

            // Clone the object
            auto cloned = obj->clone();
            if (!cloned) continue;

            // Apply accumulated displacement to clone
            applyObjectAffineTransform(cloned.get(), 1, 0, 0, 1, accumX, accumY);

            // Add cloned object to the target layer
            targetLayer->addObject(std::move(cloned));
        }
    }
    
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::stepRepeatRectangular(int countX, int countY, double spacingX, double spacingY)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // Find target layer
    int firstGlobalIdx = selected.first();
    GraphicObject* firstObj = object(firstGlobalIdx);
    if (!firstObj) return;
    
    ArtLayer* targetLayer = nullptr;
    if (ArtPage* page = m_document.page(0)) {
        for (int i = 0; i < page->layerCount(); ++i) {
            if (ArtLayer* layer = page->layer(i)) {
                const auto& objects = layer->objects();
                for (const auto& obj : objects) {
                    if (obj.get() == firstObj) {
                        targetLayer = layer;
                        break;
                    }
                }
                if (targetLayer) break;
            }
        }
    }
    
    if (!targetLayer) return;

    for (int y = 0; y < countY; ++y) {
        for (int x = 0; x < countX; ++x) {
            double accumX = spacingX * x;
            double accumY = spacingY * y;
            
            for (int globalIdx : selected) {
                GraphicObject* obj = object(globalIdx);
                if (!obj) continue;

                auto cloned = obj->clone();
                if (!cloned) continue;

                applyObjectAffineTransform(cloned.get(), 1, 0, 0, 1, accumX, accumY);
                targetLayer->addObject(std::move(cloned));
            }
        }
    }
    
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::stepRepeatHexagonal(int count, double radius, double angle)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    int firstGlobalIdx = selected.first();
    GraphicObject* firstObj = object(firstGlobalIdx);
    if (!firstObj) return;
    
    ArtLayer* targetLayer = nullptr;
    if (ArtPage* page = m_document.page(0)) {
        for (int i = 0; i < page->layerCount(); ++i) {
            if (ArtLayer* layer = page->layer(i)) {
                const auto& objects = layer->objects();
                for (const auto& obj : objects) {
                    if (obj.get() == firstObj) {
                        targetLayer = layer;
                        break;
                    }
                }
                if (targetLayer) break;
            }
        }
    }
    
    if (!targetLayer) return;

    for (int i = 0; i < count; ++i) {
        double theta = i * angle;
        double dx = radius * cos(theta);
        double dy = radius * sin(theta);

        for (int globalIdx : selected) {
            GraphicObject* obj = object(globalIdx);
            if (!obj) continue;

            auto cloned = obj->clone();
            if (!cloned) continue;

            applyObjectAffineTransform(cloned.get(), 1, 0, 0, 1, dx, dy);
            targetLayer->addObject(std::move(cloned));
        }
    }
    
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::imposeLayout(QString layoutType, int copiesX, int copiesY, double gutterX, double gutterY)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    if (layoutType == QStringLiteral("grid")) {
        stepRepeatRectangular(copiesX, copiesY, gutterX, gutterY);
    } else if (layoutType == QStringLiteral("hexagonal")) {
        stepRepeatHexagonal(copiesX, 50.0, 60.0);
    }

    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

// ======================================================================
// Marks Engine integration
// ======================================================================

void Editor::drawRegistrationMark(const QString& markType, const QRectF& rect, const QColor& color, double lineWidth)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // TODO: Implementar criação de marca de registro
    // Por enquanto, apenas emitir sinal de modificação

    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
}

void Editor::drawCutMark(const QString& markType, const QRectF& rect, const QColor& color, double lineWidth)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // TODO: Implementar criação de marca de corte

    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
}

void Editor::drawBleedMark(const QString& markType, const QRectF& rect, const QColor& color, double lineWidth)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // TODO: Implementar criação de marca de sangria

    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
}

void Editor::drawColorBar(const QString& markType, const QRectF& rect, const QColor& color, double lineWidth)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // TODO: Implementar criação de barra de cor

    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
}

void Editor::generatePrintMarks(QString markSet, int copiesX, int copiesY, double gutterX, double gutterY)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // TODO: Implementar geração de marcas de impressão

    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
}

// ======================================================================
// Undo/Redo
// ======================================================================

void Editor::saveState()
{
    if (m_selected.isEmpty()) return;

    m_undoState.objects.clear();
    m_undoState.hasTransform = false;
    
    auto objects = collectAllObjectsWithIndices();
    for (const auto& pair : objects) {
        int globalIdx = pair.first;
        GraphicObject* obj = pair.second;
        
        if (!m_selected.contains(globalIdx) || !obj)
            continue;
        
        TransformationState::ObjectState state;
        state.globalIndex = globalIdx;
        state.transform = obj->transform();
        m_undoState.objects.append(state);
    }
    
    m_undoState.hasTransform = !m_undoState.objects.isEmpty();
    m_undoCount++;
    emit undoAvailable(true);
}

void Editor::applyState(const TransformationState& state)
{
    if (!state.hasTransform)
        return;

    for (const auto& objState : state.objects) {
        GraphicObject* obj = object(objState.globalIndex);
        if (obj) {
            obj->setTransform(objState.transform);
        }
    }
}

void Editor::undo()
{
    if (m_undoCount > 0 && m_undoState.hasTransform) {
        applyState(m_undoState);
        
        // Move current state to redo stack
        m_redoState = m_undoState;
        m_redoState.hasTransform = true;
        
        // Clear undo state
        m_undoState.hasTransform = false;
        m_undoState.objects.clear();
        
        m_undoCount--;
        m_redoCount++;
        emit undoAvailable(m_undoCount > 0);
        emit redoAvailable(true);
        emit selectionChanged();
        if (!m_selected.isEmpty()) {
            emit objectModified(m_selected.first());
        }
    }
}

void Editor::redo()
{
    if (m_redoCount > 0 && m_redoState.hasTransform) {
        applyState(m_redoState);
        
        // Move redo state back to undo stack
        m_undoState = m_redoState;
        m_undoState.hasTransform = true;
        
        // Clear redo state
        m_redoState.hasTransform = false;
        m_redoState.objects.clear();
        
        m_undoCount++;
        m_redoCount--;
        emit undoAvailable(true);
        emit redoAvailable(m_redoCount > 0);
        emit selectionChanged();
        if (!m_selected.isEmpty()) {
            emit objectModified(m_selected.first());
        }
    }
}

// ======================================================================
// Properties
// ======================================================================

bool Editor::isModified() const
{
    return m_undoCount > 0 || m_redoCount > 0;
}

// ======================================================================
// Clipboard — copy, cut e paste
// ======================================================================

void Editor::copyAvailable(bool available)
{
    Q_UNUSED(available);
    // O portapapéis deveria armazenar clones independentes dos objetos selecionados
    // para que não sejam invalidados quando o documento mudar.
    // Em implementação futura, serializaríamos os objetos ou seus IDs.
}

void Editor::pasteAvailable(bool available)
{
    Q_UNUSED(available);
    // Colar objetos da área de transferência de volta ao documento.
    // Os objetos colados deveriam ser clones dos objetos copiados/cortados,
    // posicionados apropriadamente.
}

// ======================================================================
// Group/Ungroup
// ======================================================================

void Editor::groupSelected()
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // Find target layer
    GraphicObject* firstObj = object(selected.first());
    if (!firstObj) return;
    
    ArtLayer* targetLayer = nullptr;
    if (ArtPage* page = m_document.page(0)) {
        for (int i = 0; i < page->layerCount(); ++i) {
            if (ArtLayer* layer = page->layer(i)) {
                const auto& objects = layer->objects();
                for (const auto& obj : objects) {
                    if (obj.get() == firstObj) {
                        targetLayer = layer;
                        break;
                    }
                }
                if (targetLayer) break;
            }
        }
    }
    
    if (!targetLayer) return;

    // Create group object
    auto group = std::make_unique<objects::GroupObject>();
    group->setName(QStringLiteral("Group"));
    
    // Collect selected objects (in reverse order to maintain z-order)
    QVector<std::unique_ptr<objects::GraphicObject>> children;
    for (int i = selected.size() - 1; i >= 0; --i) {
        int globalIdx = selected[i];
        if (ArtPage* page = m_document.page(0)) {
            for (int i = 0; i < page->layerCount(); ++i) {
                if (ArtLayer* layer = page->layer(i)) {
                    if (layer->removeObject(globalIdx)) {
                        // This is complex - we need to find the actual object
                        // For now, just emit signals
                    }
                }
            }
        }
    }
    
    // For now, just emit signals - full implementation requires more infrastructure
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
}

void Editor::ungroupSelected()
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // TODO: Implement ungrouping
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
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
// Z-Order operations
// ======================================================================

void Editor::bringToFront()
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // TODO: Implement z-order operations
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
}

void Editor::sendToBack()
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
}

void Editor::bringForward()
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
}

void Editor::sendBackward()
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
}

// ======================================================================
// Properties
// ======================================================================

void Editor::setFillColor(unsigned int color)
{
    saveState();
    auto objects = collectAllObjectsWithIndices();
    for (const auto& pair : objects) {
        int globalIdx = pair.first;
        GraphicObject* obj = pair.second;
        
        if (!m_selected.contains(globalIdx) || !obj)
            continue;
        
        obj->setFill(artplus::colors::Paint::fromColor(color));
    }
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::setStrokeColor(unsigned int color)
{
    saveState();
    auto objects = collectAllObjectsWithIndices();
    for (const auto& pair : objects) {
        int globalIdx = pair.first;
        GraphicObject* obj = pair.second;
        
        if (!m_selected.contains(globalIdx) || !obj)
            continue;
        
        obj->setStroke(artplus::colors::Paint::fromColor(color));
    }
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::setStrokeWidth(double width)
{
    saveState();
    auto objects = collectAllObjectsWithIndices();
    for (const auto& pair : objects) {
        int globalIdx = pair.first;
        GraphicObject* obj = pair.second;
        
        if (!m_selected.contains(globalIdx) || !obj)
            continue;
        
        obj->setStrokeWidth(width);
    }
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::setOpacity(double opacity)
{
    saveState();
    auto objects = collectAllObjectsWithIndices();
    for (const auto& pair : objects) {
        int globalIdx = pair.first;
        GraphicObject* obj = pair.second;
        
        if (!m_selected.contains(globalIdx) || !obj)
            continue;
        
        obj->setVisible(obj->isVisible());
        // Opacity would be set via paint opacity in fill/stroke
        // For now just emit signal
    }
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::setVisible(bool visible)
{
    saveState();
    auto objects = collectAllObjectsWithIndices();
    for (const auto& pair : objects) {
        int globalIdx = pair.first;
        GraphicObject* obj = pair.second;
        
        if (!m_selected.contains(globalIdx) || !obj)
            continue;
        
        obj->setVisible(visible);
    }
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::setLocked(bool locked)
{
    saveState();
    auto objects = collectAllObjectsWithIndices();
    for (const auto& pair : objects) {
        int globalIdx = pair.first;
        GraphicObject* obj = pair.second;
        
        if (!m_selected.contains(globalIdx) || !obj)
            continue;
        
        obj->setLocked(locked);
    }
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

// ======================================================================
// Clipboard operations
// ======================================================================

void Editor::copySelected()
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // TODO: Implement clipboard copy with serialization
    emit copyAvailable(true);
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::cutSelected()
{
    saveState();
    // TODO: Implement cut
    emit cutAvailable(true);
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::paste()
{
    // TODO: Implement paste
    emit pasteAvailable(true);
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

// ======================================================================
// Z-Order operations
// ======================================================================

void Editor::bringToFront()
{
    saveState();
    // TODO: Implement z-order operations
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
}

void Editor::sendToBack()
{
    saveState();
    // TODO
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
}

void Editor::bringForward()
{
    saveState();
    // TODO
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
}

void Editor::sendBackward()
{
    saveState();
    // TODO
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
}

// ======================================================================
// Properties
// ======================================================================

void Editor::updateProperties()
{
    if (m_selected.isEmpty()) return;
    
    // Emit signal with current object's properties for inspector panel
    int globalIdx = m_selected.first();
    GraphicObject* obj = object(globalIdx);
    if (obj) {
        // Emit signals with object properties for inspector panel
        // TODO: Implement property signals
    }
    emit objectModified(globalIdx);
    m_redoCount = 0;
    emit redoAvailable(false);
}

// ======================================================================
// Fim do namespace
// ======================================================================

} // namespace artplus::document