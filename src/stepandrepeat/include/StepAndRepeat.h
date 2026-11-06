#pragma once
#include <QString>
namespace artplus::stepandrepeat { enum class Stage { ReadingPDF, PreparingLayout, GeneratingOutput, SavingOutput }; enum class GridSpec { Columns, Rows, CellSizeX, CellSizeY, GapX, GapY, Rotation, Overlap, Stagger, Rollover, HeadTurn, FillOrder, ContinuousDirection }; double gridParameter(const QString& key); int cellNumbersX(const QString& params); int cellNumbersY(const QString& params); }
