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

    // Hit-testing / object access
    int objectCount() const;
    GraphicObject* object(int globalIndex);
    const GraphicObject* object(int globalIndex) const;

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
    QVector<int> m_selected;          // índices globais dos objetos selecionados
    
    struct TransformationState {
        // Para cada objeto selecionado, guarda o índice global e a matriz de transformação
        struct ObjectState {
            int globalIndex;
            geometry::Matrix transform;
        };
        QVector<ObjectState> objects;
        bool hasTransform = false;
    } m_undoState, m_redoState;
    int m_undoCount = 0;
    int m_redoCount = 0;

    // Gerar snapshot do estado atual para undo
    void saveState();
    // Aplicar snapshot salvo
    void applyState(const TransformationState& state);
    
    // Helper: collect all objects from page 0 in visual order (bottom to top)
    QVector<std::pair<int, GraphicObject*>> collectAllObjectsWithIndices() const;
};

} // namespace artplus::document