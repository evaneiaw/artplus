#pragma once

#include "artplus/document/ArtDocument.h"
#include "artplus/document/ArtLayer.h"
#include "artplus/document/ArtPage.h"
#include "artplus/objects/GraphicObject.h"

#include <QObject>
#include <QVector>
#include <QString>
#include <QDebug>
#include <unordered_set>

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

    // Seleção - usa ObjectId estável
    void selectObject(objects::ObjectId id);
    void deselectAll();
    QVector<objects::ObjectId> selectedObjects() const;
    bool isSelected(objects::ObjectId id) const;

    // Hit-testing / object access - por ID estável
    int objectCount() const;
    GraphicObject* object(objects::ObjectId id);
    const GraphicObject* object(objects::ObjectId id) const;
    GraphicObject* objectAt(int pageIndex, int layerIndex, int localIndex);
    const GraphicObject* objectAt(int pageIndex, int layerIndex, int localIndex) const;

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

    // Clipboard operations
    void copySelected();
    void cutSelected();
    void paste();

    // Group/Ungroup
    void groupSelected();
    void ungroupSelected();

    // Z-Order operations
    void bringToFront();
    void sendToBack();
    void bringForward();
    void sendBackward();

    // Properties
    void setFillColor(unsigned int color);
    void setStrokeColor(unsigned int color);
    void setStrokeWidth(double width);
    void setOpacity(double opacity);
    void setVisible(bool visible);
    void setLocked(bool locked);
    void updateProperties();

    // Clipboard operations (clipboard state notifications)
    // void copyAvailable(bool available);
    // void pasteAvailable(bool available);

signals:
    void selectionChanged();
    void objectModified(objects::ObjectId id);
    void undoAvailable(bool available);
    void redoAvailable(bool available);
    void copyAvailable(bool available);
    void cutAvailable(bool available);
    void pasteAvailable(bool available);

private:
    ArtDocument& m_document;
    std::unordered_set<objects::ObjectId> m_selected;  // IDs estáveis dos objetos selecionados
    
    struct TransformationState {
        struct ObjectState {
            objects::ObjectId id;
            geometry::Matrix transform;
        };
        QVector<ObjectState> objects;
        bool hasTransform = false;
    } m_undoState, m_redoState;
    int m_undoCount = 0;
    int m_redoCount = 0;

    // Clipboard
    QVector<std::unique_ptr<objects::GraphicObject>> m_clipboard;

    // Gerar snapshot do estado atual para undo
    void saveState();
    // Aplicar snapshot salvo
    void applyState(const TransformationState& state);
    
    // Helper: collect all objects with their IDs
    QVector<std::pair<objects::ObjectId, GraphicObject*>> collectAllObjectsWithIds() const;
};

} // namespace artplus::document