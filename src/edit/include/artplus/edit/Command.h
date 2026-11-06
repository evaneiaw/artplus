#pragma once

#include "artplus/edit/Transform.h"
#include "artplus/pdf/PdfEditor.h"

#include <QString>

#include <memory>
#include <optional>
#include <vector>

namespace artplus::edit {

// A reversible document operation. `apply()` moves the document to the "after"
// state, `revert()` back to the "before" state. Commands store absolute object
// matrices so repeated undo/redo never accumulates numeric drift.
class Command
{
public:
    virtual ~Command() = default;
    virtual QString name() const = 0;
    virtual bool apply() = 0;
    virtual bool revert() = 0;
};

class CommandStack
{
public:
    // Applies the command and pushes it. Returns false (and pushes nothing)
    // when the command reports that it changed nothing.
    bool execute(std::unique_ptr<Command> command);

    bool canUndo() const { return m_index > 0; }
    bool canRedo() const { return m_index < static_cast<int>(m_commands.size()); }
    bool undo();
    bool redo();

    int undoCount() const { return m_index; }
    int redoCount() const { return static_cast<int>(m_commands.size()) - m_index; }

    QString undoName() const;
    QString redoName() const;

    // Modified state relative to the last saved/clean point.
    bool isModified() const { return m_index != m_cleanIndex; }
    void markClean();

    void clear();

    // Transform Again infrastructure.
    // Returns true if the most recently executed command was a TransformObjectsCommand.
    bool hasLastTransform() const { return m_lastTransform.has_value(); }

    // Extracts the transform parameters and anchor from the last TransformObjectsCommand.
    // Returns nullopt if the last command was not a transform or if it had no effect.
    std::optional<std::tuple<TransformParams, TransformAnchor, bool>> lastTransform() const
    {
        return m_lastTransform;
    }

private:
    std::vector<std::unique_ptr<Command>> m_commands;
    int m_index = 0;
    int m_cleanIndex = 0;

    // Cache for Transform Again (Ctrl+D). Stores params + anchor only when a
    // TransformObjectsCommand was successfully applied.
    std::optional<std::tuple<artplus::edit::TransformParams, artplus::edit::TransformAnchor, bool>> m_lastTransform;
};

// Sets an object to an absolute PDF matrix.
class ObjectMatrixCommand : public Command
{
public:
    ObjectMatrixCommand(pdf::PdfEditor& editor, int objectIndex, const pdf::PdfMatrix& target,
                        QString label);

    QString name() const override { return m_label; }
    bool apply() override;
    bool revert() override;

protected:
    pdf::PdfEditor& m_editor;
    int m_index;
    pdf::PdfMatrix m_before;
    pdf::PdfMatrix m_after;
    QString m_label;
};

class MoveObjectCommand : public ObjectMatrixCommand
{
public:
    MoveObjectCommand(pdf::PdfEditor& editor, int objectIndex, double dx, double dy);
};

class ScaleObjectCommand : public ObjectMatrixCommand
{
public:
    // Scales about (anchorX, anchorY) in page coordinates.
    ScaleObjectCommand(pdf::PdfEditor& editor, int objectIndex, double anchorX, double anchorY,
                       double sx, double sy);
};

class RotateObjectCommand : public ObjectMatrixCommand
{
public:
    // Rotates about (centerX, centerY) in page coordinates by degrees.
    RotateObjectCommand(pdf::PdfEditor& editor, int objectIndex, double centerX, double centerY,
                        double degrees);
};

// Applies one complete ArtPro+ Transform operation to one or more objects.
//
// The transformation order (scale -> flip -> shear -> rotate -> move), the
// anchor semantics and the repetition/copy behaviour were measured against
// ArtPro+ v24. With `copy` set, the originals stay untouched and one copy per
// repetition is inserted (Alt+Transform). Without it, every selected object is
// transformed `params.repetitions` times.
class TransformObjectsCommand : public Command
{
public:
    TransformObjectsCommand(pdf::PdfEditor& editor, const std::vector<int>& objectIndices,
                            const TransformParams& params, TransformAnchor anchor, bool copy);

    QString name() const override;
    bool apply() override;
    bool revert() override;

    bool hasEffect() const { return m_effect; }

    // Accessor for Transform Again (Ctrl+D).
    const TransformParams& params() const { return m_params; }
    TransformAnchor anchor() const { return m_anchor; }
    bool isCopy() const { return m_copy; }
    const std::vector<int>& objectIndices() const { return m_originalIndices; }

private:
    struct Target {
        int index = -1;
        pdf::PdfMatrix before;
        pdf::PdfMatrix after;
    };
    struct Duplicate {
        int source = -1;
        pdf::PdfMatrix matrix;
        double strokeWidth = 0.0;
        int actualIndex = -1;
    };
    struct StrokeChange {
        int index = -1;
        double before = 0.0;
        double after = 0.0;
    };

    pdf::PdfEditor& m_editor;
    TransformParams m_params;
    TransformAnchor m_anchor = TransformAnchor::Center;
    bool m_copy = false;
    bool m_effect = false;
    int m_originalCount = 0;
    std::vector<int> m_originalIndices;
    std::vector<Target> m_targets;
    std::vector<Duplicate> m_duplicates;
    std::vector<StrokeChange> m_strokes;
};

} // namespace artplus::edit
