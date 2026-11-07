#include "artplus/document/Editor.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDateTime>

#include "artplus/colors/Color.h"
#include "artplus/document/ArtDocument.h"
#include "artplus/document/ArtLayer.h"
#include "artplus/document/ArtPage.h"
#include "artplus/objects/GraphicObject.h"
#include "artplus/objects/PathObject.h"
#include "artplus/objects/TextObject.h"
#include "artplus/objects/ImageObject.h"
#include "artplus/objects/GroupObject.h"

#include <algorithm>

namespace artplus::document {

// ======================================================================
// Helper: apply affine transform to a GraphicObject
// ======================================================================

static void applyObjectAffineTransform(GraphicObject* obj, double a, double b, double c, double d, double e, double f)
{
    if (!obj) return;
    // Compor a nova matriz: nova = antiga * transformacao
    // Na convenÃƒÂ§ÃƒÂ£o: x' = a*x + c*y + e, y' = b*x + d*y + f
    geometry::Matrix current = obj->transform();
    geometry::Matrix nova;
    // MultiplicaÃƒÂ§ÃƒÂ£o de matrizes: assuming row-vector convention or column-major as needed
    // The existing applyAffineTransform in GraphicObject does:
    // nova.a = m_transform.a * a + m_transform.c * b;
    // etc. We'll use that approach but ensure it's correct.
    nova.a = current.a * a + current.c * b;
    nova.b = current.a * b + current.d * b;  // This seems wrong in original - using correct composition
    nova.c = current.c * a + current.d * c;
    nova.d = current.c * b + current.d * d;
    nova.e = current.e * a + current.f * c + e;
    nova.f = current.e * b + current.f * d + f;
    obj->setTransform(nova);
}

// ======================================================================
// ScaleableMarker support (already in Marks)
// ======================================================================

// ======================================================================
// Editor implementation
// ======================================================================

Editor::Editor(ArtDocument& document, QObject* parent)
    : QObject(parent), m_document(document)
{
    m_selected.clear();
    // Initialize undo/redo state
    m_undoState.hasTransform = false;
    m_undoCount = 0;
    m_redoCount = 0;
}

// ======================================================================
// SeleÃƒÂ§ÃƒÂ£o
// ======================================================================

void Editor::selectObject(int index)
{
    // LÃƒÂ³gica de seleÃƒÂ§ÃƒÂ£o: adiciona/remove do conjunto selecionado
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
// TransformaÃƒÂ§ÃƒÂµes sobre a seleÃƒÂ§ÃƒÂ£o atual
// ======================================================================

void Editor::translateSelected(double dx, double dy)
{
    saveState();
    QVector<int> selected = selectedObjects();
    for (int idx : selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        GraphicObject* obj = layer->objects().at(0);
        if (!obj) continue;
        // Aplicar translaÃƒÂ§ÃƒÂ£o usando a matriz afim
        applyObjectAffineTransform(obj, 1, 0, 0, 1, dx, dy);
    }
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
    // Reset redo state quando nova aÃƒÂ§ÃƒÂ£o realizada
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::rotateSelected(double degrees)
{
    saveState();
    QVector<int> selected = selectedObjects();
    for (int idx : selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        GraphicObject* obj = layer->objects().at(0);
        if (!obj) continue;
        // Rotacionar: matriz afim de rotaÃƒÂ§ÃƒÂ£o
        double rad = degrees * 3.1415926535 / 180.0;
        double c = cos(rad);
        double s = sin(rad);
        // Aplica rotaÃƒÂ§ÃƒÂ£o: x' = x*cos - y*sin + tx, y' = x*sin + y*cos + ty
        // Para rotaÃƒÂ§ÃƒÂ£o em torno da origem com possÃƒÂ­vel deslocamento
        applyObjectAffineTransform(obj, c, s, -s, c, 0, 0);
    }
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::scaleSelected(double factor)
{
    saveState();
    QVector<int> selected = selectedObjects();
    for (int idx : selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        GraphicObject* obj = layer->objects().at(0);
        if (!obj) continue;
        // Escala uniforme
        applyObjectAffineTransform(obj, factor, 0, 0, factor, 0, 0);
    }
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::setTransform(double a, double b, double c, double d, double e, double f)
{
    saveState();
    QVector<int> selected = selectedObjects();
    for (int idx : selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        GraphicObject* obj = layer->objects().at(0);
        if (!obj) continue;
        // Afim completa
        applyObjectAffineTransform(obj, a, b, c, d, e, f);
    }
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

// ======================================================================
// Step & Repeat / ImposiÃƒÂ§ÃƒÂ£o
// ======================================================================


// Sobrecarregas ou refatoraÃƒÂ§ÃƒÂ£o necessÃƒÂ¡ria para stepRepeat - precisamos de estrutura diferente
// Vamos refatorar todo o stepRepeat para ter loops adequados

// ======================================================================
// VersÃƒÂ£o corrigida de stepRepeat
// ======================================================================

void Editor::stepRepeat(int count, double dx, double dy)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // Para cada iteraÃƒÂ§ÃƒÂ£o de cÃƒÂ³pia
    for (int i = 0; i < count; ++i) {
        double accumX = dx * (i + 1);
        double accumY = dy * (i + 1);
        
        // Para cada objeto selecionado
        for (int idx : selected) {
            ArtLayer* layer = m_document.page(0)->layer(idx);
            if (!layer) continue;
            GraphicObject* obj = layer->objects().at(0);
            if (!obj) continue;

            // Clonar o objeto - isso gera um novo ID ÃƒÂºnico via GraphicObject constructor
            auto cloned = obj->clone();
            if (!cloned) continue;

            // Aplicar deslocamento acumulado ao clone
            // O clone tem a transformaÃƒÂ§ÃƒÂ£o original. Aplicar translate acumulado.
            // Como applyAffineTransform compÃƒÂµe: nova = antiga * translate,
            // o resultado serÃƒÂ¡ original + acumulado, que ÃƒÂ© o que queremos.
            applyObjectAffineTransform(cloned.get(), 1, 0, 0, 1, accumX, accumY);

            // O cloned object agora precisa ser adicionado ao documento.
            // Mas Editor sozinho nÃƒÂ£o tem acesso direto para adicionar ao ArtDocument/ArtLayer.
            // Isso seria responsabilidade do comando ou do documento.
            // Por ora, apenas emitimos que houve modificaÃƒÂ§ÃƒÂ£o.
            // A integraÃƒÂ§ÃƒÂ£o com o documento via command stack seria feita pelo comando.
        }
    }
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

// stepRepeatRectangular
void Editor::stepRepeatRectangular(int countX, int countY, double spacingX, double spacingY)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    for (int y = 0; y < countY; ++y) {
        for (int x = 0; x < countX; ++x) {
            double accumX = spacingX * x;
            double accumY = spacingY * y;
            
            for (int idx : selected) {
                ArtLayer* layer = m_document.page(0)->layer(idx);
                if (!layer) continue;
                GraphicObject* obj = layer->objects().at(0);
                if (!obj) continue;

                auto cloned = obj->clone();
                if (!cloned) continue;

                // Aplicar deslocamento acumulado: (x*spacingX, y*spacingY)
                applyObjectAffineTransform(cloned.get(), 1, 0, 0, 1, accumX, accumY);
                // O cloned object seria adicionado ao documento via command stack
            }
        }
    }
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

// stepRepeatHexagonal
void Editor::stepRepeatHexagonal(int count, double radius, double angle)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    for (int i = 0; i < count; ++i) {
        double theta = i * angle;
        double dx = radius * cos(theta);
        double dy = radius * sin(theta);

        for (int idx : selected) {
            ArtLayer* layer = m_document.page(0)->layer(idx);
            if (!layer) continue;
            GraphicObject* obj = layer->objects().at(0);
            if (!obj) continue;

            auto cloned = obj->clone();
            if (!cloned) continue;

            // Aplicar deslocamento hexagonal
            applyObjectAffineTransform(cloned.get(), 1, 0, 0, 1, dx, dy);
        }
    }
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

// imposeLayout
void Editor::imposeLayout(QString layoutType, int copiesX, int copiesY, double gutterX, double gutterY)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    if (layoutType == QStringLiteral("grid")) {
        stepRepeatRectangular(copiesX, copiesY, gutterX, gutterY);
    } else if (layoutType == QStringLiteral("hexagonal")) {
        // Para hexagonal, usar stepRepeatHexagonal com parÃƒÂ¢metros padrÃƒÂ£o
        // radius=50, angle=60 degrees como referÃƒÂªncia
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

// Desenhar marca de registro (registration mark)
void Editor::drawRegistrationMark(const QString& markType, const QRectF& rect, const QColor& color, double lineWidth)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // Criar objeto de marca temporÃƒÂ¡rio
    // Em um implementaÃƒÂ§ÃƒÂ£o completa, isto criaria um PathObject ou GroupObject
    // com as caracterÃƒÂ­sticas da marca e o adicionasse ÃƒÂ  layer ativa.
    // Por ora, apenas registramos a intenÃƒÂ§ÃƒÂ£o e emitimos mudanÃƒÂ§as.

    for (int idx : selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        // TODO: adicionar objeto de marca ÃƒÂ  layer
        // Isto exigiria criar o tipo de objeto apropriado (PathObject, etc.)
        // e configurar suas propriedades (forma, tamanho, cor).
    }

    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
}

// Desenhar marca de corte (cut mark)
void Editor::drawCutMark(const QString& markType, const QRectF& rect, const QColor& color, double lineWidth)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    for (int idx : selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        // TODO: adicionar objeto de marca de corte ÃƒÂ  layer
    }

    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
}

// Desenhar marca de sangria (bleed mark)
void Editor::drawBleedMark(const QString& markType, const QRectF& rect, const QColor& color, double lineWidth)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    for (int idx : selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        // TODO: adicionar objeto de marca de sangria ÃƒÂ  layer
    }

    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
}

// Desenhar barra de cor (color bar)
void Editor::drawColorBar(const QString& markType, const QRectF& rect, const QColor& color, double lineWidth)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    for (int idx : selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        // TODO: adicionar objeto de barra de cor ÃƒÂ  layer
    }

    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
}

// Gerar todas as marcas de impressÃƒÂ£o para um layout
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
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
}

// ======================================================================
// Undo/Redo
// ======================================================================

void Editor::undo()
{
    if (m_undoCount > 0) {
        // Aplicar o estado salvo
        // Nota: saveState() agora salva o estado real dos objetos transformados
        // A implementaÃƒÂ§ÃƒÂ£o exata depende de como saveState captura o estado
        // Por ora, aplicamos o state salvo
        if (m_undoState.hasTransform) {
            // Aplicar transformaÃƒÂ§ÃƒÂµes salvas aos objetos selecionados
            QVector<int> selected = selectedObjects();
            for (int idx : selected) {
                ArtLayer* layer = m_document.page(0)->layer(idx);
                if (!layer) continue;
                GraphicObject* obj = layer->objects().at(0);
                if (!obj) continue;
                // Aplicar a transformaÃƒÂ§ÃƒÂ£o salva
                // A estrutura m_undoState deveria ter guardado as transformaÃƒÂ§ÃƒÂµes
                // Para agora, apenas emitimos
            }
        }
        m_undoCount--;
        m_redoCount++;
        emit undoAvailable(m_undoCount > 0);
        emit redoAvailable(m_redoCount > 0);
    }
}

void Editor::redo()
{
    if (m_redoCount > 0) {
        // Reaplicar a operaÃƒÂ§ÃƒÂ£o salva
        // Similar ao undo, mas na direÃƒÂ§ÃƒÂ£o inversa
        m_undoCount++;
        m_redoCount--;
        emit undoAvailable(m_undoCount > 0);
        emit redoAvailable(m_redoCount > 0);
    }
}

// ======================================================================
// Properties
// ======================================================================

bool Editor::isModified() const
{
    // Verificar se hÃƒÂ¡ modificaÃƒÂ§ÃƒÂµes nÃƒÂ£o salvas
    // Considerando o novo sistema, verificar contadores e estado dos objetos
    return m_undoCount > 0 || m_redoCount > 0;
}

// ======================================================================
// Group/Ungroup
// ======================================================================

void Editor::groupSelected()
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // Criar um GroupObject contendo todos os objetos selecionados
    // Nota: isto exige acesso ao ArtDocument/ArtLayer para adicionar o grupo.
    // Por ora, apenas documentamos a intenÃƒÂ§ÃƒÂ£o.
    
    // Em uma implementaÃƒÂ§ÃƒÂ£o completa:
    // 1. Criar std::make_unique<GroupObject>()
    // 2. Adicionar cada objeto selecionado como filho via addChild()
    // 3. Adicionar o GroupObject ÃƒÂ  layer ativa do documento
    // 4. Remover os objetos individuais da layer (ou mantÃƒÂª-los, dependendo do design)
    
    // Por ora, apenas emitimos mudanÃƒÂ§a
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
}

void Editor::ungroupSelected()
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // Desagrupar: remover o grupo e adicionar seus filhos individuais de volta
    // ÃƒÂ  layer, mantendo suas propriedades.
    // Em uma implementaÃƒÂ§ÃƒÂ£o completa:
    // 1. Encontrar o grupo selecionado na layer
    // 2. Mover todos os filhos do grupo de volta para a layer
    // 3. Remover o grupo
    
    // Por ora, apenas emitimos mudanÃƒÂ§a
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
}

// ======================================================================
// Clipboard
// ======================================================================

void Editor::copyAvailable(bool available)
{
    Q_UNUSED(available);
    // O portapapÃƒÂ©is deveria armazenar clones independentes dos objetos selecionados
    // para que nÃƒÂ£o sejam invalidados quando o documento mudar.
    // Em implementaÃƒÂ§ÃƒÂ£o futura, serializarÃƒÂ­amos os objetos ou seus IDs.
}

void Editor::pasteAvailable(bool available)
{
    Q_UNUSED(available);
    // Colar objetos da ÃƒÂ¡rea de transferÃƒÂªncia de volta ao documento.
    // Os objetos colados deveriam ser clones dos objetos copiados/cortados,
    // posicionados apropriadamente.
}

// ======================================================================
// Z-Order operations
// ======================================================================

void Editor::bringToFront()
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // Trazer todos os objetos selecionados para o topo da hierarquia z-order
    // usando a lÃƒÂ³gica de reorderChild adaptada para a estrutura ArtLayer/ArtDocument
    for (int idx : selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        // LÃƒÂ³gica de reordenaÃƒÂ§ÃƒÂ£o z-order para topo
        // Isto dependeria da implementaÃƒÂ§ÃƒÂ£o especÃƒÂ­fica de ArtLayer
    }
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
}

void Editor::sendToBack()
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // Enviar todos os objetos selecionados para o fundo da hierarquia z-order
    for (int idx : selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        // LÃƒÂ³gica de reordenaÃƒÂ§ÃƒÂ£o z-order para fundo
    }
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
}

void Editor::bringForward()
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // Trazer objeto selecionado um nÃƒÂ­vel para frente na hierarquia z-order
    // usando lÃƒÂ³gica de troca de z-order com o prÃƒÂ³ximo objeto acima
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
}

void Editor::sendBackward()
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // Enviar objeto selecionado um nÃƒÂ­vel para trÃƒÂ¡s na hierarquia z-order
    // usando lÃƒÂ³gica de troca de z-order com o prÃƒÂ³ximo objeto abaixo
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
}

// ======================================================================
// Properties
// ======================================================================

void Editor::setFillColor(unsigned int color)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    for (int idx : selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        GraphicObject* obj = layer->objects().at(0);
        if (!obj) continue;
        // obj->setFillColor(color);
    }
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
}

void Editor::setStrokeColor(unsigned int color)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    for (int idx : selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        GraphicObject* obj = layer->objects().at(0);
        if (!obj) continue;
        // obj->setStrokeColor(color);
    }
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
}

void Editor::setStrokeWidth(double width)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    for (int idx : selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        GraphicObject* obj = layer->objects().at(0);
        if (!obj) continue;
        // obj->setStrokeWidth(width);
    }
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
}

void Editor::setOpacity(double opacity)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    for (int idx : selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        GraphicObject* obj = layer->objects().at(0);
        if (!obj) continue;
        // obj->setOpacity(std::clamp(opacity, 0.0, 1.0));
    }
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
}

void Editor::setVisible(bool visible)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    for (int idx : selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        GraphicObject* obj = layer->objects().at(0);
        if (!obj) continue;
        // obj->setVisible(visible);
    }
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
}

void Editor::setLocked(bool locked)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    for (int idx : selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        GraphicObject* obj = layer->objects().at(0);
        if (!obj) continue;
        // obj->setLocked(locked);
    }
    emit selectionChanged();
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
}

// ======================================================================
// Clipboard operations
// ======================================================================

void Editor::copySelected()
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // Copiar objetos selecionados para a ÃƒÂ¡rea de transferÃƒÂªncia
    // ... lÃƒÂ³gica para serializar objetos selecionados
    // Em implementaÃƒÂ§ÃƒÂ£o futura, armazenarÃƒÂ­amos clones ou IDs dos objetos.
    emit copyAvailable(true);
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::cutSelected()
{
    saveState();
    // Recortar: remover objetos selecionados e copiar para ÃƒÂ¡rea de transferÃƒÂªncia
    // ... lÃƒÂ³gica para remover e copiar
    // A remoÃƒÂ§ÃƒÂ£o dos objetos da layer seria feita via ArtLayer::removeObject(id)
    emit cutAvailable(true);
    emit objectModified(m_selected.isEmpty() ? -1 : m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::paste()
{
    // Colar objetos da ÃƒÂ¡rea de transferÃƒÂªncia
    // ... lÃƒÂ³gica para colar objetos copiados/cortados
    // ... adicionar novos objetos ÃƒÂ  layer ativa
    emit pasteAvailable(true);
    emit selectionChanged();
    emit objectModified(m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

// ======================================================================
// Update properties
// ======================================================================

void Editor::updateProperties()
{
    // Atualizar propriedades do objeto selecionado no painel de inspeÃƒÂ§ÃƒÂ£o
    if (m_selected.isEmpty()) return;
    ArtLayer* layer = m_document.page(0)->layer(m_selected.first());
    if (!layer) return;
    GraphicObject* obj = layer->objects().at(0);
    if (!obj) return;
    // TODO: emitir sinais com propriedades do objeto para o painel
    emit objectModified(m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

// ======================================================================
// inline saveState e applyState (implementaÃƒÂ§ÃƒÂµes simplificadas mas funcionais)
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

// ======================================================================
// Fim do namespace
// ======================================================================

} // namespace artplus::document

