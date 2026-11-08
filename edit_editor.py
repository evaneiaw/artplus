#!/usr/bin/env python3
# Script to add step repeat methods to Editor.cpp

with open('src/document/src/Editor.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Find the scaleSelected method and add after it
old_method = """void Editor::scaleSelected(double factor)
{
    saveState();
    // TODO: aplicar escala factor aos objetos selecionados
    emit objectModified(m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}"""

new_methods = """void Editor::scaleSelected(double factor)
{
    saveState();
    // TODO: aplicar escala factor aos objetos selecionados
    emit objectModified(m_selected.first());
    m_redoCount = 0;
    emit redoAvailable(false);
}

void Editor::stepRepeat(int count, double dx, double dy)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    for (int i = 0; i < count; ++i)
    {
        for (int idx : selected)
        {
            ArtLayer* layer = m_document.page(0)->layer(idx);
            if (!layer) continue;
            GraphicObject* obj = layer->findObject(/* id */);
            if (!obj) continue;

            // Aplicar translação acumulada
            double newX = obj->transform().a + dx * (i + 1);
            double newY = obj->transform().d + dy * (i + 1);
            // TODO: aplicar nova posição ao objeto
        }
    }
    emit selectionChanged();
    emit objectModified(selected.first());
}

void Editor::stepRepeatRectangular(int countX, int countY, double spacingX, double spacingY)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    for (int y = 0; y < countY; ++y)
    {
        for (int x = 0; x < countX; ++x)
        {
            for (int idx : selected)
            {
                ArtLayer* layer = m_document.page(0)->layer(idx);
                if (!layer) continue;
                GraphicObject* obj = layer->findObject(/* id */);
                if (!obj) continue;

                // Calcular posição da réplica na grade
                double baseX = obj->transform().a;
                double baseY = obj->transform().d;
                double newX = baseX + x * spacingX;
                double newY = baseY + y * spacingY;
                // TODO: criar réplica ou repositionar objeto
            }
        }
    }
    emit selectionChanged();
    emit objectModified(selected.first());
}

void Editor::stepRepeatHexagonal(int count, double radius, double angle)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    for (int i = 0; i < count; ++i)
    {
        double theta = i * angle;
        double dx = radius * cos(theta);
        double dy = radius * sin(theta);

        for (int idx : selected)
        {
            ArtLayer* layer = m_document.page(0)->layer(idx);
            if (!layer) continue;
            GraphicObject* obj = layer->findObject(/* id */);
            if (!obj) continue;

            // Aplicar deslocamento hexagonal
            double newX = obj->transform().a + dx;
            double newY = obj->transform().d + dy;
            // TODO: aplicar nova posição ao objeto
        }
    }
    emit selectionChanged();
    emit objectModified(selected.first());
}

void Editor::imposeLayout(QString layoutType, int copiesX, int copiesY, double gutterX, double gutterY)
{
    saveState();
    QVector<int> selected = selectedObjects();
    if (selected.isEmpty()) return;

    // Implementação baseada no tipo de layout
    if (layoutType == QStringLiteral("grid"))
    {
        stepRepeatRectangular(copiesX, copiesY, gutterX, gutterY);
    }
    else if (layoutType == QStringLiteral("hexagonal"))
    {
        stepRepeatHexagonal(copiesX, 50.0, 60.0); // radius=50, angle=60 degrees
    }

    emit selectionChanged();
    emit objectModified(selected.first());
}"""

if old_method in content:
    new_content = content.replace(old_method, new_methods)
    with open('src/document/src/Editor.cpp', 'w', encoding='utf-8') as f:
        f.write(new_content)
    print("File updated successfully")
else:
    print("old_method not found, trying alternate...")
    # Try finding just the end of scaleSelected
    idx = content.find("emit redoAvailable(false);\n}")
    if idx >= 0:
        insert_pos = idx + len("emit redoAvailable(false);\n}")
        new_content = content[:insert_pos] + new_methods + "\n" + content[insert_pos:]
        with open('src/document/src/Editor.cpp', 'w', encoding='utf-8') as f:
            f.write(new_content)
        print("File updated successfully (alternate)")
    else:
        print("Could not find target location")
        print("Looking for pattern...")
        idx2 = content.find("emit redoAvailable(false);")
        if idx2 >= 0:
            print("Found at:", idx2)