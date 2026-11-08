#include "artplus/document/Editor.h"

#include "artplus/document/ArtDocument.h"
#include "artplus/document/ArtLayer.h"
#include "artplus/document/ArtPage.h"
#include "artplus/objects/GraphicObject.h"
#include "artplus/objects/PathObject.h"
#include "artplus/colors/Paint.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDateTime>
#include <QMimeData>
#include <QApplication>

#include <algorithm>
#include <cmath>

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
    geometry::Matrix nova;
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
    m_undoState.hasTransform = false;
    m_redoState.hasTransform = false;
    m_undoCount = 0;
    m_redoCount = 0;
}

Editor::~Editor() {}

// ======================================================================
// Seleção - usando ObjectId estável
// ======================================================================

void Editor::selectObject(objects::ObjectId id)
{
    if (m_selected.contains(id)) {
        m_selected.erase(id);
    } else {
        m_selected.insert(id);
    }
    emit selectionChanged();
}

void Editor::deselectAll()
{
    m_selected.clear();
    emit selectionChanged();
}

QVector<objects::ObjectId> Editor::selectedObjects() const
{
    QVector<objects::ObjectId> result;
    result.reserve(m_selected.size());
    for (const auto& id : m_selected) {
        result.append(id);
    }
    return result;
}

bool Editor::isSelected(objects::ObjectId id) const
{
    return m_selected.contains(id);
}

// ======================================================================
// Hit-testing / object access - usando ObjectId estável
// ======================================================================

int Editor::objectCount() const
{
    int count = 0;
    for (const auto& page : m_document.pages()) {
        count += page->objectCount();
    }
    return count;
}

GraphicObject* Editor::object(objects::ObjectId id)
{
    if (!id) return nullptr;
    
    for (auto& page : m_document.pages()) {
        for (auto& layer : page->layers()) {
            if (auto* obj = layer->findObject(id)) {
                return obj;
            }
        }
    }
    return nullptr;
}

const GraphicObject* Editor::object(objects::ObjectId id) const
{
    return const_cast<Editor*>(this)->object(id);
}

GraphicObject* Editor::objectAt(int pageIndex, int layerIndex, int localIndex)
{
    if (ArtPage* page = m_document.page(pageIndex)) {
        if (ArtLayer* layer = page->layer(layerIndex)) {
            const auto& objects = layer->objects();
            if (localIndex >= 0 && localIndex < (int)objects.size()) {
                return objects[localIndex].get();
            }
        }
    }
    return nullptr;
}

const GraphicObject* Editor::objectAt(int pageIndex, int layerIndex, int localIndex) const
{
    return const_cast<Editor*>(this)->objectAt(pageIndex, layerIndex, localIndex);
}

// Helper: collect all objects with their IDs
QVector<std::pair<objects::ObjectId, GraphicObject*>> Editor::collectAllObjectsWithIds() const
{
    QVector<std::pair<objects::ObjectId, GraphicObject*>> result;
    
    for (auto& page : m_document.pages()) {
        for (auto& layer : page->layers()) {
            for (const auto& obj : layer->objects()) {
                result.append({obj->id(), obj.get()});
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
    
    auto objects = collectAllObjectsWithIds();
    for (const auto& pair : objects) {
        objects::ObjectId id = pair.first;
        GraphicObject* obj = pair.second;
        
        if (!m_selected.contains(id) || !obj)
            continue;
        
        // Apply translation
        applyObjectAffineTransform(obj, 1, 0, 0, 1, dx, dy);
    }
    
    emit selectionChanged();
    emit objectModified(m_selected.empty() ? objects::ObjectId{} : *m_selected.begin());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::rotateSelected(double degrees)
{
    saveState();
    
    double rad = degrees * 3.14159265358979323846 / 180.0;
    double c = cos(rad);
    double s = sin(rad);
    
    auto objects = collectAllObjectsWithIds();
    for (const auto& pair : objects) {
        objects::ObjectId id = pair.first;
        GraphicObject* obj = pair.second;
        
        if (!m_selected.contains(obj->id()) || !obj)
            continue;
        
        // Apply rotation around object center
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
    emit objectModified(m_selected.empty() ? objects::ObjectId{} : *m_selected.begin());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::scaleSelected(double factor)
{
    saveState();
    
    auto objects = collectAllObjectsWithIds();
    for (const auto& pair : objects) {
        objects::ObjectId id = pair.first;
        GraphicObject* obj = pair.second;
        
        if (!m_selected.contains(id) || !obj)
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
    emit objectModified(m_selected.empty() ? objects::ObjectId{} : *m_selected.begin());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::setTransform(double a, double b, double c, double d, double e, double f)
{
    saveState();
    
    auto objects = collectAllObjectsWithIds();
    for (const auto& pair : objects) {
        objects::ObjectId id = pair.first;
        GraphicObject* obj = pair.second;
        
        if (!m_selected.contains(id) || !obj)
            continue;
        
        applyObjectAffineTransform(obj, a, b, c, d, e, f);
    }
    
    emit selectionChanged();
    emit objectModified(m_selected.empty() ? objects::ObjectId{} : *m_selected.begin());
    m_redoCount = 0;
    emit redoAvailable(false);
}

// ======================================================================
// Step & Repeat / Imposição
// ======================================================================

void Editor::stepRepeat(int count, double dx, double dy)
{
    saveState();
    QVector<objects::ObjectId> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // Get the layer of the first selected object to add copies there
    objects::ObjectId firstId = selected.first();
    GraphicObject* firstObj = object(firstId);
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
    }
    
    if (!targetLayer) return;

    // For each copy iteration
    for (int i = 0; i < count; ++i) {
        double accumX = dx * (i + 1);
        double accumY = dy * (i + 1);
        
        // For each selected object
        for (objects::ObjectId objId : selected) {
            GraphicObject* obj = object(objId);
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
    emit objectModified(m_selected.empty() ? objects::ObjectId{} : *m_selected.begin());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::stepRepeatRectangular(int countX, int countY, double spacingX, double spacingY)
{
    saveState();
    QVector<objects::ObjectId> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // Find target layer
    objects::ObjectId firstId = selected.first();
    GraphicObject* firstObj = object(firstId);
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
            
            for (objects::ObjectId objId : selected) {
                GraphicObject* obj = object(objId);
                if (!obj) continue;

                auto cloned = obj->clone();
                if (!cloned) continue;

                applyObjectAffineTransform(cloned.get(), 1, 0, 0, 1, accumX, accumY);
                targetLayer->addObject(std::move(cloned));
            }
        }
    }
    
    emit selectionChanged();
    emit objectModified(m_selected.empty() ? objects::ObjectId{} : *m_selected.begin());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::stepRepeatHexagonal(int count, double radius, double angle)
{
    saveState();
    QVector<objects::ObjectId> selected = selectedObjects();
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

        for (objects::ObjectId objId : selected) {
            GraphicObject* obj = object(objId);
            if (!obj) continue;

            auto cloned = obj->clone();
            if (!cloned) continue;

            applyObjectAffineTransform(cloned.get(), 1, 0, 0, 1, dx, dy);
            targetLayer->addObject(std::move(cloned));
        }
    }
    
    emit selectionChanged();
    emit objectModified(m_selected.empty() ? objects::ObjectId{} : *m_selected.begin());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::imposeLayout(QString layoutType, int copiesX, int copiesY, double gutterX, double gutterY)
{
    saveState();
    QVector<objects::ObjectId> selected = selectedObjects();
    if (selected.isEmpty()) return;

    if (layoutType == QStringLiteral("grid")) {
        stepRepeatRectangular(copiesX, copiesY, gutterX, gutterY);
    } else if (layoutType == QStringLiteral("hexagonal")) {
        stepRepeatHexagonal(copiesX, 50.0, 60.0);
    }

    emit selectionChanged();
    emit objectModified(m_selected.empty() ? objects::ObjectId{} : *m_selected.begin());
    m_redoCount = 0;
    emit redoAvailable(false);
}

// ======================================================================
// Marks Engine integration
// ======================================================================

void Editor::drawRegistrationMark(const QString& markType, const QRectF& rect, const QColor& color, double lineWidth)
{
    saveState();
    QVector<objects::ObjectId> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // TODO: Implementar criação de marca de registro
    // Por enquanto, apenas emitir sinal de modificação

    emit selectionChanged();
    emit objectModified(m_selected.empty() ? objects::ObjectId{} : *m_selected.begin());
}

void Editor::drawCutMark(const QString& markType, const QRectF& rect, const QColor& color, double lineWidth)
{
    saveState();
    QVector<objects::ObjectId> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // TODO: Implementar criação de marca de corte

    emit selectionChanged();
    emit objectModified(m_selected.empty() ? objects::ObjectId{} : *m_selected.begin());
}

void Editor::drawBleedMark(const QString& markType, const QRectF& rect, const QColor& color, double lineWidth)
{
    saveState();
    QVector<objects::ObjectId> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // TODO: Implementar criação de marca de sangria

    emit selectionChanged();
    emit objectModified(m_selected.empty() ? objects::ObjectId{} : *m_selected.begin());
}

void Editor::drawColorBar(const QString& markType, const QRectF& rect, const QColor& color, double lineWidth)
{
    saveState();
    QVector<objects::ObjectId> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // TODO: Implementar criação de barra de cor

    emit selectionChanged();
    emit objectModified(m_selected.empty() ? objects::ObjectId{} : *m_selected.begin());
}

void Editor::generatePrintMarks(QString markSet, int copiesX, int copiesY, double gutterX, double gutterY)
{
    saveState();
    QVector<objects::ObjectId> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // TODO: Implementar geração de marcas de impressão

    emit selectionChanged();
    emit objectModified(m_selected.empty() ? objects::ObjectId{} : *m_selected.begin());
}

// ======================================================================
// Undo/Redo
// ======================================================================

void Editor::saveState()
{
    if (m_selected.empty()) return;

    m_undoState.objects.clear();
    m_undoState.hasTransform = false;
    
    for (const auto& id : m_selected) {
        if (GraphicObject* obj = object(id)) {
            TransformationState::ObjectState state;
            state.id = id;
            state.transform = obj->transform();
            m_undoState.objects.append(state);
        }
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
        if (GraphicObject* obj = object(objState.id)) {
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
        if (!m_selected.empty()) {
            emit objectModified(*m_selected.begin());
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
        if (!m_selected.empty()) {
            emit objectModified(*m_selected.begin());
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

void Editor::copySelected()
{
    saveState();
    QVector<objects::ObjectId> selected = selectedObjects();
    if (selected.isEmpty()) return;

    m_clipboard.clear();
    for (objects::ObjectId id : selected) {
        if (GraphicObject* obj = object(id)) {
            m_clipboard.append(obj->clone());
        }
    }
    emit copyAvailable(true);
    emit objectModified(m_selected.empty() ? objects::ObjectId{} : *m_selected.begin());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::cutSelected()
{
    saveState();
    QVector<objects::ObjectId> selected = selectedObjects();
    if (selected.isEmpty()) return;

    m_clipboard.clear();
    for (objects::ObjectId id : selected) {
        if (GraphicObject* obj = object(id)) {
            m_clipboard.append(obj->clone());
        }
    }

    // Remove selected objects from document
    for (objects::ObjectId id : selected) {
        // Find and remove from layer
        for (auto& page : m_document.pages()) {
            for (auto& layer : page->layers()) {
                if (layer->removeObject(id)) {
                    break;
                }
            }
        }
    }
    
    m_selected.clear();
    emit selectionChanged();
    emit copyAvailable(true);
    emit objectModified(objects::ObjectId{});
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::paste()
{
    if (m_clipboard.isEmpty()) return;
    
    saveState();
    
    // Find target layer (first layer of first page, or create default)
    ArtLayer* targetLayer = nullptr;
    if (ArtPage* page = m_document.page(0)) {
        if (page->layerCount() > 0) {
            targetLayer = page->layer(0);
        } else {
            targetLayer = page->addLayer(QStringLiteral("Paste"), LayerType::Artwork);
        }
    }
    
    if (!targetLayer) return;

    QVector<objects::ObjectId> pastedIds;
    for (const auto& obj : m_clipboard) {
        auto cloned = obj->clone();
        if (cloned) {
            pastedIds.append(cloned->id());
            targetLayer->addObject(std::move(cloned));
        }
    }
    
    // Select pasted objects
    m_selected.clear();
    for (auto id : pastedIds) {
        m_selected.insert(id);
    }
    
    emit selectionChanged();
    emit pasteAvailable(true);
    emit objectModified(m_selected.empty() ? objects::ObjectId{} : *m_selected.begin());
    m_redoCount = 0;
    emit redoAvailable(false);
}

// ======================================================================
// Group/Ungroup
// ======================================================================

void Editor::groupSelected()
{
    saveState();
    QVector<objects::ObjectId> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // Find target layer
    GraphicObject* firstObj = object(selected.first());
    if (!firstObj) return;
    
    ArtLayer* targetLayer = nullptr;
    int pageIdx = -1, layerIdx = -1;
    for (int p = 0; p < m_document.pageCount(); ++p) {
        if (ArtPage* page = m_document.page(p)) {
            for (int l = 0; l < page->layerCount(); ++l) {
                if (ArtLayer* layer = page->layer(l)) {
                    const auto& objects = layer->objects();
                    for (const auto& obj : objects) {
                        if (obj.get() == firstObj) {
                            targetLayer = layer;
                            pageIdx = p;
                            layerIdx = l;
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
    
    // Collect selected objects and remove them from layers
    QVector<std::unique_ptr<objects::GraphicObject>> children;
    for (objects::ObjectId id : selected) {
        if (auto* obj = object(id)) {
            // We need to remove from layer and take ownership
            // For simplicity, clone and remove original
            children.append(obj->clone());
        }
    }
    
    // Remove original objects
    for (objects::ObjectId id : selected) {
        for (auto& page : m_document.pages()) {
            for (auto& layer : page->layers()) {
                layer->removeObject(id);
            }
        }
    }
    
    // Add children to group
    for (auto& child : children) {
        group->addChild(std::move(child));
    }
    
    // Add group to target layer
    objects::ObjectId groupId = group->id();
    targetLayer->addObject(std::move(group));
    
    // Select the new group
    m_selected.clear();
    m_selected.insert(groupId);
    
    emit selectionChanged();
    emit objectModified(groupId);
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::ungroupSelected()
{
    saveState();
    QVector<objects::ObjectId> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // Find group objects among selected
    for (objects::ObjectId id : selected) {
        if (GraphicObject* obj = object(id)) {
            if (obj->type() == objects::ObjectType::Group) {
                auto* group = static_cast<objects::GroupObject*>(obj);
                
                // Find layer containing this group
                ArtLayer* parentLayer = nullptr;
                int pageIdx = -1;
                for (int p = 0; p < m_document.pageCount(); ++p) {
                    if (ArtPage* page = m_document.page(p)) {
                        for (int l = 0; l < page->layerCount(); ++l) {
                            if (ArtLayer* layer = page->layer(l)) {
                                if (layer->findObject(id)) {
                                    // Remove group from layer
                                    layer->removeObject(id);
                                    
                                    // Add children to layer
                                    for (auto& child : group->children()) {
                                        layer->addObject(child->clone());
                                    }
                                    break;
                                }
                            }
                            if (parentLayer) break;
                        }
                        if (parentLayer) break;
                    }
                }
            }
        }
    
    m_selected.clear();
    emit selectionChanged();
    emit objectModified(objects::ObjectId{});
    m_redoCount = 0;
    emit redoAvailable(false);
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
    QVector<objects::ObjectId> selected = selectedObjects();
    if (selected.isEmpty()) return;

    for (objects::ObjectId id : selected) {
        if (GraphicObject* obj = object(id)) {
            // Find layer and move to front
            for (auto& page : m_document.pages()) {
                for (auto& layer : page->layers()) {
                    if (layer->findObject(id)) {
                        // Move to end of vector (top of z-order)
                        auto& objects = const_cast<std::vector<std::unique_ptr<objects::GraphicObject>>&>(layer->objects());
                        auto it = std::find_if(objects.begin(), objects.end(),
                            [id](const auto& o) { return o->id() == id; });
                        if (it != objects.end()) {
                            auto obj = std::move(*it);
                            objects.erase(it);
                            objects.push_back(std::move(obj));
                        }
                        break;
                    }
                }
            }
        }
    }
    
    emit selectionChanged();
    emit objectModified(m_selected.empty() ? objects::ObjectId{} : *m_selected.begin());
}

void Editor::sendToBack()
{
    saveState();
    QVector<objects::ObjectId> selected = selectedObjects();
    if (selected.isEmpty()) return;

    for (objects::ObjectId id : selected) {
        if (GraphicObject* obj = object(id)) {
            for (auto& page : m_document.pages()) {
                for (auto& layer : page->layers()) {
                    if (layer->findObject(id)) {
                        auto& objects = const_cast<std::vector<std::unique_ptr<objects::GraphicObject>>&>(layer->objects());
                        auto it = std::find_if(objects.begin(), objects.end(),
                            [id](const auto& o) { return o->id() == id; });
                        if (it != objects.end()) {
                            auto obj = std::move(*it);
                            objects.erase(it);
                            objects.insert(objects.begin(), std::move(obj));
                        }
                        break;
                    }
                }
            }
        }
    }
    
    emit selectionChanged();
    emit objectModified(m_selected.empty() ? objects::ObjectId{} : *m_selected.begin());
}

void Editor::bringForward()
{
    saveState();
    QVector<objects::ObjectId> selected = selectedObjects();
    if (selected.isEmpty()) return;

    for (objects::ObjectId id : selected) {
        if (GraphicObject* obj = object(id)) {
            for (auto& page : m_document.pages()) {
                for (auto& layer : page->layers()) {
                    if (layer->findObject(id)) {
                        auto& objects = const_cast<std::vector<std::unique_ptr<objects::GraphicObject>>&>(layer->objects());
                        auto it = std::find_if(objects.begin(), objects.end(),
                            [id](const auto& o) { return o->id() == id; });
                        if (it != objects.end() && std::next(it) != objects.end()) {
                            std::iter_swap(it, std::next(it));
                        }
                        break;
                    }
                }
            }
        }
    }
    
    emit selectionChanged();
    emit objectModified(m_selected.empty() ? objects::ObjectId{} : *m_selected.begin());
}

void Editor::sendBackward()
{
    saveState();
    QVector<objects::ObjectId> selected = selectedObjects();
    if (selected.isEmpty()) return;

    for (objects::ObjectId id : selected) {
        if (GraphicObject* obj = object(id)) {
            for (auto& page : m_document.pages()) {
                for (auto& layer : page->layers()) {
                    if (layer->findObject(id)) {
                        auto& objects = const_cast<std::vector<std::unique_ptr<objects::GraphicObject>>&>(layer->objects());
                        auto it = std::find_if(objects.begin(), objects.end(),
                            [id](const auto& o) { return o->id() == id; });
                        if (it != objects.begin()) {
                            std::iter_swap(it, std::prev(it));
                        }
                        break;
                    }
                }
            }
        }
    }
    
    emit selectionChanged();
    emit objectModified(m_selected.empty() ? objects::ObjectId{} : *m_selected.begin());
}

// ======================================================================
// Properties
// ======================================================================

void Editor::setFillColor(unsigned int color)
{
    saveState();
    for (objects::ObjectId id : m_selected) {
        if (GraphicObject* obj = object(id)) {
            obj->setFill(artplus::colors::Paint::fromColor(color));
        }
    }
    emit selectionChanged();
    emit objectModified(m_selected.empty() ? objects::ObjectId{} : *m_selected.begin());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::setStrokeColor(unsigned int color)
{
    saveState();
    for (objects::ObjectId id : m_selected) {
        if (GraphicObject* obj = object(id)) {
            obj->setStroke(artplus::colors::Paint::fromColor(color));
        }
    }
    emit selectionChanged();
    emit objectModified(m_selected.empty() ? objects::ObjectId{} : *m_selected.begin());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::setStrokeWidth(double width)
{
    saveState();
    for (objects::ObjectId id : m_selected) {
        if (GraphicObject* obj = object(id)) {
            obj->setStrokeWidth(width);
        }
    }
    emit selectionChanged();
    emit objectModified(m_selected.empty() ? objects::ObjectId{} : *m_selected.begin());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::setOpacity(double opacity)
{
    saveState();
    for (objects::ObjectId id : m_selected) {
        if (GraphicObject* obj = object(id)) {
            // Opacity is handled via paint opacity in fill/stroke
            // For now, we adjust the fill opacity
            auto fill = obj->fill();
            fill.setOpacity(std::clamp(opacity, 0.0, 1.0));
            obj->setFill(fill);
        }
    }
    emit selectionChanged();
    emit objectModified(m_selected.empty() ? objects::ObjectId{} : *m_selected.begin());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::setVisible(bool visible)
{
    saveState();
    for (objects::ObjectId id : m_selected) {
        if (GraphicObject* obj = object(id)) {
            obj->setVisible(visible);
        }
    }
    emit selectionChanged();
    emit objectModified(m_selected.empty() ? objects::ObjectId{} : *m_selected.begin());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::setLocked(bool locked)
{
    saveState();
    for (objects::ObjectId id : m_selected) {
        if (GraphicObject* obj = object(id)) {
            obj->setLocked(locked);
        }
    }
    emit selectionChanged();
    emit objectModified(m_selected.empty() ? objects::ObjectId{} : *m_selected.begin());
    m_redoCount = 0;
    emit redoAvailable(false);
}

// ======================================================================
// Clipboard operations
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
// Properties
// ======================================================================

void Editor::updateProperties()
{
    if (m_selected.empty()) return;
    
    // Emit signal with current object's properties for inspector panel
    objects::ObjectId id = *m_selected.begin();
    if (GraphicObject* obj = object(id)) {
        // Emit signals with object properties for inspector panel
        // This would connect to a property panel
        emit objectModified(id);
    }
    m_redoCount = 0;
    emit redoAvailable(false);
}

// ======================================================================
// Fim do namespace
// ======================================================================

} // namespace artplus::document