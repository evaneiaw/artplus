# ADR-0003 — UI toolkit: Qt 6 Widgets, custom page viewport

- Status: accepted (2026-11-02)
- Context: milestone 21, first executable

## Decision

Use Qt 6.8 Widgets for the application shell and a custom `QWidget`
(`artplus::ui::PageView`) for the page viewport.

Reasons:

- Qt 6.8.3 (msvc2022_64) was already installed and used by the existing code.
- Widgets give a mature desktop shell (menus, docks, status bar) with far less
  work than a QML rewrite.
- A custom painted widget gives precise control over zoom-to-cursor, pan,
  fit modes, page borders and DPI behaviour — required for a prepress viewer.
- `PageView` deliberately has no `Q_OBJECT` so it stays moc-free and trivially
  testable; the window receives notifications through `std::function`.

## Consequences

- Positive: fast, predictable desktop UI; straightforward automated tests.
- Negative: Qt LGPL obligations (see `DEPENDENCIES_AND_LICENSES.md`); a QML
  rewrite would be needed later only if animation-heavy UI is required.
- Follow-up: revisit if performance profiling shows widget painting is the
  render bottleneck (it currently is not: PDFium is).
