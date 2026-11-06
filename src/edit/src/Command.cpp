#include "artplus/edit/Command.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <utility>

namespace artplus::edit {

namespace {

pdf::PdfMatrix multiply(const pdf::PdfMatrix& a, const pdf::PdfMatrix& b)
{
    // Applies b first, then a.
    pdf::PdfMatrix r;
    r.a = a.a * b.a + a.c * b.b;
    r.b = a.b * b.a + a.d * b.b;
    r.c = a.a * b.c + a.c * b.d;
    r.d = a.b * b.c + a.d * b.d;
    r.e = a.a * b.e + a.c * b.f + a.e;
    r.f = a.b * b.e + a.d * b.f + a.f;
    return r;
}

pdf::PdfMatrix translation(double tx, double ty)
{
    pdf::PdfMatrix m;
    m.e = tx;
    m.f = ty;
    return m;
}

pdf::PdfMatrix scaling(double sx, double sy)
{
    pdf::PdfMatrix m;
    m.a = sx;
    m.d = sy;
    return m;
}

pdf::PdfMatrix rotation(double degrees)
{
    const double rad = degrees * 3.14159265358979323846 / 180.0;
    pdf::PdfMatrix m;
    m.a = std::cos(rad);
    m.b = std::sin(rad);
    m.c = -std::sin(rad);
    m.d = std::cos(rad);
    return m;
}

} // namespace

bool CommandStack::execute(std::unique_ptr<Command> command)
{
    if (!command)
        return false;
    if (!command->apply())
        return false;

    // Drop the redo branch.
    if (m_index < static_cast<int>(m_commands.size()))
        m_commands.erase(m_commands.begin() + m_index, m_commands.end());

    // Cache transform parameters for Transform Again (Ctrl+D).
    if (TransformObjectsCommand* t =
            dynamic_cast<TransformObjectsCommand*>(command.get())) {
        if (t->hasEffect()) {
            m_lastTransform = std::make_tuple(t->params(), t->anchor(), t->isCopy());
        }
    }

    m_commands.push_back(std::move(command));
    ++m_index;
    if (m_cleanIndex > m_index)
        m_cleanIndex = -1; // the saved state is no longer reachable
    return true;
}

bool CommandStack::undo()
{
    if (!canUndo())
        return false;
    Command& command = *m_commands[static_cast<size_t>(m_index - 1)];
    if (!command.revert())
        return false;
    --m_index;
    return true;
}

bool CommandStack::redo()
{
    if (!canRedo())
        return false;
    Command& command = *m_commands[static_cast<size_t>(m_index)];
    if (!command.apply())
        return false;
    ++m_index;
    return true;
}

QString CommandStack::undoName() const
{
    if (!canUndo())
        return {};
    return m_commands[static_cast<size_t>(m_index - 1)]->name();
}

QString CommandStack::redoName() const
{
    if (!canRedo())
        return {};
    return m_commands[static_cast<size_t>(m_index)]->name();
}

void CommandStack::markClean()
{
    m_cleanIndex = m_index;
}

void CommandStack::clear()
{
    m_commands.clear();
    m_index = 0;
    m_cleanIndex = 0;
    m_lastTransform.reset();
}

ObjectMatrixCommand::ObjectMatrixCommand(pdf::PdfEditor& editor, int objectIndex,
                                         const pdf::PdfMatrix& target, QString label)
    : m_editor(editor)
    , m_index(objectIndex)
    , m_before(editor.objectMatrix(objectIndex))
    , m_after(target)
    , m_label(std::move(label))
{
}

bool ObjectMatrixCommand::apply()
{
    return m_editor.setObjectMatrix(m_index, m_after);
}

bool ObjectMatrixCommand::revert()
{
    return m_editor.setObjectMatrix(m_index, m_before);
}

MoveObjectCommand::MoveObjectCommand(pdf::PdfEditor& editor, int objectIndex, double dx, double dy)
    : ObjectMatrixCommand(editor, objectIndex,
                          multiply(translation(dx, -dy), editor.objectMatrix(objectIndex)),
                          QStringLiteral("Move object"))
{
}

ScaleObjectCommand::ScaleObjectCommand(pdf::PdfEditor& editor, int objectIndex, double anchorX,
                                       double anchorY, double sx, double sy)
    : ObjectMatrixCommand(
          editor, objectIndex,
          multiply(multiply(multiply(translation(anchorX, editor.pageHeightPoints() - anchorY),
                                     scaling(sx, sy)),
                            translation(-anchorX, -(editor.pageHeightPoints() - anchorY))),
                   editor.objectMatrix(objectIndex)),
          QStringLiteral("Resize object"))
{
}

RotateObjectCommand::RotateObjectCommand(pdf::PdfEditor& editor, int objectIndex, double centerX,
                                         double centerY, double degrees)
    : ObjectMatrixCommand(
          editor, objectIndex,
          multiply(multiply(multiply(translation(centerX, editor.pageHeightPoints() - centerY),
                                     rotation(-degrees)),
                            translation(-centerX, -(editor.pageHeightPoints() - centerY))),
                   editor.objectMatrix(objectIndex)),
          QStringLiteral("Rotate object"))
{
}

namespace {

bool matricesEqual(const pdf::PdfMatrix& a, const pdf::PdfMatrix& b)
{
    constexpr double eps = 1e-9;
    return std::abs(a.a - b.a) <= eps && std::abs(a.b - b.b) <= eps &&
           std::abs(a.c - b.c) <= eps && std::abs(a.d - b.d) <= eps &&
           std::abs(a.e - b.e) <= eps && std::abs(a.f - b.f) <= eps;
}

} // namespace

TransformObjectsCommand::TransformObjectsCommand(pdf::PdfEditor& editor,
                                                const std::vector<int>& objectIndices,
                                                const TransformParams& params,
                                                TransformAnchor anchor, bool copy)
    : m_editor(editor)
    , m_params(params)
    , m_copy(copy)
{
    m_params.repetitions = std::max(1, m_params.repetitions);
    m_originalCount = editor.objectCount();

    std::vector<int> indices;
    indices.reserve(objectIndices.size());
    QRectF groupBounds;
    for (int index : objectIndices) {
        if (index < 0 || index >= editor.objectCount() ||
            std::find(indices.begin(), indices.end(), index) != indices.end())
            continue;
        const auto* object = editor.object(index);
        if (!object)
            continue;
        indices.push_back(index);
        groupBounds = groupBounds.isNull() ? object->bounds : groupBounds.united(object->bounds);
    }

    const double pageHeight = editor.pageHeightPoints();
    const double strokeFactor = strokeScaleFactor(m_params);

    for (int index : indices) {
        const auto* object = editor.object(index);
        if (!object)
            continue;
        const QPointF anchorPointPage =
            anchorPoint(m_params.transformEach ? object->bounds : groupBounds, anchor);
        const pdf::PdfMatrix before = editor.objectMatrix(index);

        if (m_copy) {
            for (int repetition = 1; repetition <= m_params.repetitions; ++repetition) {
                Duplicate duplicate;
                duplicate.source = index;
                duplicate.matrix = objectTargetMatrix(before, m_params, anchorPointPage,
                                                      pageHeight, repetition);
                if (object->hasStrokeColor) {
                    duplicate.strokeWidth =
                        object->strokeWidth * std::pow((std::abs(m_params.scaleX) +
                                                        std::abs(m_params.scaleY)) / 2.0,
                                                       repetition);
                }
                m_duplicates.push_back(duplicate);
            }
            m_effect = true;
            continue;
        }

        Target target;
        target.index = index;
        target.before = before;
        target.after = objectTargetMatrix(before, m_params, anchorPointPage, pageHeight,
                                          m_params.repetitions);
        m_targets.push_back(target);
        if (!matricesEqual(target.before, target.after))
            m_effect = true;

        if (m_params.includeStrokeWeightsAndEffects && object->hasStrokeColor) {
            const double widthBefore = editor.objectStrokeWidth(index);
            const double widthAfter = widthBefore * strokeFactor;
            if (std::abs(widthAfter - widthBefore) > 1e-9) {
                m_strokes.push_back({index, widthBefore, widthAfter});
                m_effect = true;
            }
        }
    }
}

QString TransformObjectsCommand::name() const
{
    return QStringLiteral("Transform");
}

bool TransformObjectsCommand::apply()
{
    if (!m_effect)
        return false;

    if (m_copy) {
        for (Duplicate& duplicate : m_duplicates)
            duplicate.actualIndex = -1;

        for (Duplicate& duplicate : m_duplicates) {
            duplicate.actualIndex =
                m_editor.duplicateObject(duplicate.source, duplicate.matrix);
            if (duplicate.actualIndex < 0) {
                for (auto it = m_duplicates.rbegin(); it != m_duplicates.rend(); ++it) {
                    if (it->actualIndex >= 0)
                        m_editor.removeObject(it->actualIndex);
                }
                m_editor.refresh();
                return false;
            }
        }

        m_editor.refresh();
        for (const Duplicate& duplicate : m_duplicates) {
            if (duplicate.actualIndex >= 0 && duplicate.strokeWidth > 0.0)
                m_editor.setObjectStrokeWidth(duplicate.actualIndex, duplicate.strokeWidth);
        }
        return true;
    }

    bool ok = true;
    for (const Target& target : m_targets) {
        if (!m_editor.setObjectMatrix(target.index, target.after))
            ok = false;
    }
    for (const StrokeChange& change : m_strokes)
        m_editor.setObjectStrokeWidth(change.index, change.after);
    return ok;
}

bool TransformObjectsCommand::revert()
{
    if (!m_effect)
        return false;

    if (m_copy) {
        std::vector<int> inserted;
        for (const Duplicate& duplicate : m_duplicates) {
            if (duplicate.actualIndex >= 0)
                inserted.push_back(duplicate.actualIndex);
        }
        std::sort(inserted.begin(), inserted.end(), std::greater<int>());
        for (int index : inserted)
            m_editor.removeObject(index);
        m_editor.refresh();
        return true;
    }

    bool ok = true;
    for (const Target& target : m_targets) {
        if (!m_editor.setObjectMatrix(target.index, target.before))
            ok = false;
    }
    for (const StrokeChange& change : m_strokes)
        m_editor.setObjectStrokeWidth(change.index, change.before);
    return ok;
}

} // namespace artplus::edit
