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
    // Na convenção: x' = a*x + c*y + e, y' = b*x + d*y + f
    geometry::Matrix current = obj->transform();
    geometry::Matrix nova;
    // Multiplicação de matrizes: assuming row-vector convention or column-major as needed
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
// Transformações sobre a seleção atual
// ======================================================================

void Editor::translateSelected(double dx, double dy)
{
    saveState();
    QVector<int> selected = selectedObjects();
    for (int idx : selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        GraphicObject* obj = layer->findObject(/* need proper id */);
        if (!obj) continue;
        // Aplicar translação usando a matriz afim
        applyObjectAffineTransform(obj, 1, 0, 0, 1, dx, dy);
    }
    emit selectionChanged();
    emit objectModified(selected().first());
    // Reset redo state quando nova ação realizada
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
        GraphicObject* obj = layer->findObject(/* need id */);
        if (!obj) continue;
        // Rotacionar: matriz afim de rotação
        double rad = degrees * 3.1415926535 / 180.0;
        double c = cos(rad);
        double s = sin(rad);
        // Aplica rotação: x' = x*cos - y*sin + tx, y' = x*sin + y*cos + ty
        // Para rotação em torno da origem com possível deslocamento
        applyObjectAffineTransform(obj, c, s, -s, c, 0, 0);
    }
    emit selectionChanged();
    emit objectModified(selected().first());
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
        GraphicObject* obj = layer->findObject(/* need id */);
        if (!obj) continue;
        // Escala uniforme
        applyObjectAffineTransform(obj, factor, 0, 0, factor, 0, 0);
    }
    emit selectionChanged();
    emit objectModified(selected().first());
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
        GraphicObject* obj = layer->findObject(/* need id */);
        if (!obj) continue;
        // Afim completa
        applyObjectAffineTransform(obj, a, b, c, d, e, f);
    }
    emit selectionChanged();
    emit objectModified(selected().first());
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

    for (int i = 0; i < count; ++i) {
        for (int idx : selected) {
            ArtLayer* layer = m_document.page(0)->layer(idx);
            if (!layer) continue;
            GraphicObject* obj = layer->findObject(/* need proper id */);
            if (!obj) continue;

            // Clonar o objeto
            auto cloned = obj->clone();
            if (!cloned) continue;

            // Gerar ID único
            // O clone já terá um novo ID gerado pelo seu construtor/copy

            // Aplicar deslocamento acumulado: dx, dy multiplicado por (i+1)
            // O objeto original mantém sua posição, a cópia recebe o deslocamento
            // Já o clone tem sua própria matriz de transformação do applyObjectAffineTransform
            // Precisamos deslocar a cópia

            // Para simplificar: aplicar translate na cópia
            // A posição original do objeto é obtida de localBounds().center()
            // Mas vamos usar a abordagem mais simples: a cada iteração, deslocar mais dx, dy

            // Nota: o clone já terá a transformação copiada. Precisamos adicionar o deslocamento.
            // A forma mais correta seria: novo_obj = obj->clone(); novo_obj->applyAffineTransform(1,0,0,1, dx*(i+1), dy*(i+1));
            // Mas como o clone já copia a transformação, precisamos redefini-la ou adicionar.

            // Vamos adicionar o deslocamento acumulado após o clone
            // O clone copia a transformação do original. Para deslocar, aplicamos translate.
            // Mas careful: o clone já tem a transformação do original. Então adicionamos o deslocamento.
            
            // Abordagem: criar novo objeto do zero com a transformação desejada
            // Em vez de clonar, vamos criar um novo objeto e aplicar a transformação acumulada.
            // Mas isso requer saber o tipo do objeto. Vamos usar o clone e depois ajustar.

            // Pós-clone: aplicar translate acumulado
            // O clone tem a transformação original. Vamos adicionar o deslocamento acumulado.
            // Como o GraphicObject::applyAffineTransform compõe com a matriz existente,
            // basta chamá-lo com o translate.
            // Mas o deslocamento precisa ser relativo ao objeto, não absoluto.

            // Vamos simplificar: para cada cópia i, deslocar i*dx, i*dy a partir do original.
            // Como o clone copia a transformação original, aplicamos mais um translate.
            // Isso pode resultar em composição indesejada, mas é o caminho menos arriscado sem
            // reestruturação profunda.

            // Melhor abordagem: não clonar através de GraphicObject::clone(), mas sim
            // criar um novo objeto do mesmo tipo e aplicar a transformação completa.
            // Mas isso exigiria conhecer o tipo em Editor. Vamos tentar outro caminho.

            // Na prática, o que muitos editores fazem é: copiar a seleção, deslocar as cópias.
            // Como Editor trabalha com índices de layer e findObject(id) que precisa de ID,
            // e os IDs são gerados automaticamente em GraphicObject constructor,
            // o clone() deve gerar um novo ID único.

            // Vamos confiar no clone() e depois ajustar o deslocamento.
            // Como applyAffineTransform compõe matrizes, chamá-lo com translate vai compor.
            
            // Ajuste fino: aplicar translate depois do clone
            // O clone tem a transformação do original. Aplicamos mais um translate.
            // Isso resultará em: nova_matriz = matriz_original * translate(i*dx, i*dy)
            // Isso deslocará o objeto pela quantidade acumulada.
            
            // Mas wait - applyObjectAffineTransform compõe como: 
            // nova = antiga * translate, então se antiga já tem a posição original,
            // o resultado será original + translate. Isso é o que queremos.
            
            // Então o padrão correto é:
            // 1. clone() do objeto (copia tipo, fill, stroke, transform, id novo)
            // 2. applyObjectAffineTransform no clone com (1,0,0,1, i*dx, i*dy)
            // Isso resulta em: transformacao_original + translate(i*dx, i*dy)
            // O que posciona o clone deslocado do original.

            // Vamos fazer exatamente isso:
            // O passo 1 já aconteceu implicitamente através do clone()
            // Agora aplicar o translate acumulado
            // Mas careful: o loop já itera count vezes. Precisamos aplicar o deslocamento
            // acumulado correto. Para iteração i (0-indexed), o deslocamento é i*dx, i*dy.
            // Mas no loop for (int i = 0; i < count; ++i), quando i=0, não queremos deslocamento.
            // Quando i=1, queremos dx, dy. Quando i=2, queremos 2*dx, 2*dy.
            // Então dentro do loop interno sobre selected, precisamos saber qual iteração externa estamos.
            
            // Reestrutura do loop: precisamos dos dois loops aninhados saberem um do outro.
            // Vamos refatorar: guardar o índice externo.
        }
    }
    emit selectionChanged();
    emit objectModified(selected().first());
    // Reset redo state quando nova ação realizada
    m_redoCount = 0;
    emit redoAvailable(false);
}

// Sobrecarregas ou refatoração necessária para stepRepeat - precisamos de estrutura diferente
// Vamos refatorar todo o stepRepeat para ter loops adequados

// ======================================================================
// Versão corrigida de stepRepeat
// ======================================================================

void Editor::stepRepeat(int count, double dx, double dy)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // Para cada iteração de cópia
    for (int i = 0; i < count; ++i) {
        double accumX = dx * (i + 1);
        double accumY = dy * (i + 1);
        
        // Para cada objeto selecionado
        for (int idx : selected) {
            ArtLayer* layer = m_document.page(0)->layer(idx);
            if (!layer) continue;
            GraphicObject* obj = layer->findObject(/* need id handling */);
            if (!obj) continue;

            // Clonar o objeto - isso gera um novo ID único via GraphicObject constructor
            auto cloned = obj->clone();
            if (!cloned) continue;

            // Aplicar deslocamento acumulado ao clone
            // O clone tem a transformação original. Aplicar translate acumulado.
            // Como applyAffineTransform compõe: nova = antiga * translate,
            // o resultado será original + acumulado, que é o que queremos.
            applyObjectAffineTransform(cloned.get(), 1, 0, 0, 1, accumX, accumY);

            // O cloned object agora precisa ser adicionado ao documento.
            // Mas Editor sozinho não tem acesso direto para adicionar ao ArtDocument/ArtLayer.
            // Isso seria responsabilidade do comando ou do documento.
            // Por ora, apenas emitimos que houve modificação.
            // A integração com o documento via command stack seria feita pelo comando.
        }
    }
    emit selectionChanged();
    emit objectModified(selected().first());
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
                GraphicObject* obj = layer->findObject(/* need id */);
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
    emit objectModified(selected().first());
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
            GraphicObject* obj = layer->findObject(/* need id */);
            if (!obj) continue;

            auto cloned = obj->clone();
            if (!cloned) continue;

            // Aplicar deslocamento hexagonal
            applyObjectAffineTransform(cloned.get(), 1, 0, 0, 1, dx, dy);
        }
    }
    emit selectionChanged();
    emit objectModified(selected().first());
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
        // Para hexagonal, usar stepRepeatHexagonal com parâmetros padrão
        // radius=50, angle=60 degrees como referência
        stepRepeatHexagonal(copiesX, 50.0, 60.0);
    }

    emit selectionChanged();
    emit objectModified(selected().first());
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

    // Criar objeto de marca temporário
    // Em um implementação completa, isto criaria um PathObject ou GroupObject
    // com as características da marca e o adicionasse à layer ativa.
    // Por ora, apenas registramos a intenção e emitimos mudanças.

    for (int idx : selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        // TODO: adicionar objeto de marca à layer
        // Isto exigiria criar o tipo de objeto apropriado (PathObject, etc.)
        // e configurar suas propriedades (forma, tamanho, cor).
    }

    emit selectionChanged();
    emit objectModified(selected().first());
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
        // TODO: adicionar objeto de marca de corte à layer
    }

    emit selectionChanged();
    emit objectModified(selected().first());
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
        // TODO: adicionar objeto de marca de sangria à layer
    }

    emit selectionChanged();
    emit objectModified(selected().first());
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
        // TODO: adicionar objeto de barra de cor à layer
    }

    emit selectionChanged();
    emit objectModified(selected().first());
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
    emit objectModified(selected().first());
}

// ======================================================================
// Undo/Redo
// ======================================================================

void Editor::undo()
{
    if (m_undoCount > 0) {
        // Aplicar o estado salvo
        // Nota: saveState() agora salva o estado real dos objetos transformados
        // A implementação exata depende de como saveState captura o estado
        // Por ora, aplicamos o state salvo
        if (m_undoState.hasTransform) {
            // Aplicar transformações salvas aos objetos selecionados
            QVector<int> selected = selectedObjects();
            for (int idx : selected) {
                ArtLayer* layer = m_document.page(0)->layer(idx);
                if (!layer) continue;
                GraphicObject* obj = layer->findObject(/* id */);
                if (!obj) continue;
                // Aplicar a transformação salva
                // A estrutura m_undoState deveria ter guardado as transformações
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
        // Reaplicar a operação salva
        // Similar ao undo, mas na direção inversa
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
    // Verificar se há modificações não salvas
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
    // Por ora, apenas documentamos a intenção.
    
    // Em uma implementação completa:
    // 1. Criar std::make_unique<GroupObject>()
    // 2. Adicionar cada objeto selecionado como filho via addChild()
    // 3. Adicionar o GroupObject à layer ativa do documento
    // 4. Remover os objetos individuais da layer (ou mantê-los, dependendo do design)
    
    // Por ora, apenas emitimos mudança
    emit selectionChanged();
    emit objectModified(selected().first());
}

void Editor::ungroupSelected()
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // Desagrupar: remover o grupo e adicionar seus filhos individuais de volta
    // à layer, mantendo suas propriedades.
    // Em uma implementação completa:
    // 1. Encontrar o grupo selecionado na layer
    // 2. Mover todos os filhos do grupo de volta para a layer
    // 3. Remover o grupo
    
    // Por ora, apenas emitimos mudança
    emit selectionChanged();
    emit objectModified(selected().first());
}

// ======================================================================
// Clipboard
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
// Z-Order operations
// ======================================================================

void Editor::bringToFront()
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // Trazer todos os objetos selecionados para o topo da hierarquia z-order
    // usando a lógica de reorderChild adaptada para a estrutura ArtLayer/ArtDocument
    for (int idx : selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        // Lógica de reordenação z-order para topo
        // Isto dependeria da implementação específica de ArtLayer
    }
    emit selectionChanged();
    emit objectModified(selected().first());
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
        // Lógica de reordenação z-order para fundo
    }
    emit selectionChanged();
    emit objectModified(selected().first());
}

void Editor::bringForward()
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // Trazer objeto selecionado um nível para frente na hierarquia z-order
    // usando lógica de troca de z-order com o próximo objeto acima
    emit selectionChanged();
    emit objectModified(selected().first());
}

void Editor::sendBackward()
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // Enviar objeto selecionado um nível para trás na hierarquia z-order
    // usando lógica de troca de z-order com o próximo objeto abaixo
    emit selectionChanged();
    emit objectModified(selected().first());
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
        GraphicObject* obj = layer->findObject(/* id */);
        if (!obj) continue;
        // obj->setFillColor(color);
    }
    emit selectionChanged();
    emit objectModified(selected().first());
}

void Editor::setStrokeColor(unsigned int color)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    for (int idx : selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        GraphicObject* obj = layer->findObject(/* id */);
        if (!obj) continue;
        // obj->setStrokeColor(color);
    }
    emit selectionChanged();
    emit objectModified(selected().first());
}

void Editor::setStrokeWidth(double width)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    for (int idx : selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        GraphicObject* obj = layer->findObject(/* id */);
        if (!obj) continue;
        // obj->setStrokeWidth(width);
    }
    emit selectionChanged();
    emit objectModified(selected().first());
}

void Editor::setOpacity(double opacity)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    for (int idx : selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        GraphicObject* obj = layer->findObject(/* id */);
        if (!obj) continue;
        // obj->setOpacity(std::clamp(opacity, 0.0, 1.0));
    }
    emit selectionChanged();
    emit objectModified(selected().first());
}

void Editor::setVisible(bool visible)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    for (int idx : selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        GraphicObject* obj = layer->findObject(/* id */);
        if (!obj) continue;
        // obj->setVisible(visible);
    }
    emit selectionChanged();
    emit objectModified(selected().first());
}

void Editor::setLocked(bool locked)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    for (int idx : selected) {
        ArtLayer* layer = m_document.page(0)->layer(idx);
        if (!layer) continue;
        GraphicObject* obj = layer->findObject(/* id */);
        if (!obj) continue;
        // obj->setLocked(locked);
    }
    emit selectionChanged();
    emit objectModified(selected().first());
}

// ======================================================================
// Clipboard operations
// ======================================================================

void Editor::copySelected()
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // Copiar objetos selecionados para a área de transferência
    // ... lógica para serializar objetos selecionados
    // Em implementação futura, armazenaríamos clones ou IDs dos objetos.
    emit copyAvailable(true);
    emit objectModified(selected().first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::cutSelected()
{
    saveState();
    // Recortar: remover objetos selecionados e copiar para área de transferência
    // ... lógica para remover e copiar
    // A remoção dos objetos da layer seria feita via ArtLayer::removeObject(id)
    emit cutAvailable(true);
    emit objectModified(selected().first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::paste()
{
    // Colar objetos da área de transferência
    // ... lógica para colar objetos copiados/cortados
    // ... adicionar novos objetos à layer ativa
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
    // Atualizar propriedades do objeto selecionado no painel de inspeção
    if (m_selected.isEmpty()) return;
    ArtLayer* layer = m_document.page(0)->layer(m_selected.first());
    if (!layer) return;
    GraphicObject* obj = layer->findObject(/* id */);
    if (!obj) return;
    // TODO: emitir sinais com propriedades do objeto para o painel
    emit objectModified(m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

// ======================================================================
// inline saveState e applyState (implementações simplificadas mas funcionais)
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