#include "artplus/ui/StepAndRepeatDialog.h"

#include "artplus/ui/Theme.h"

#include <QApplication>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace artplus::ui {

namespace {
constexpr double kMinOffset = -10000.0;
constexpr double kMaxOffset = 10000.0;
constexpr double kMinGap = -10000.0;
constexpr double kMaxGap = 10000.0;
constexpr double kStaggerMax = 10000.0;
} // namespace

StepAndRepeatDialog::StepAndRepeatDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Step & Repeat"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setMinimumWidth(420);
    buildUi();
}

void StepAndRepeatDialog::buildUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // --- Copies ---
    auto* copiesGroup = new QWidget(this);
    auto* copiesLayout = new QFormLayout(copiesGroup);
    copiesLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    copiesLayout->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
    copiesLayout->setHorizontalSpacing(12);
    copiesLayout->setVerticalSpacing(8);

    m_copiesX = new QSpinBox(copiesGroup);
    m_copiesX->setRange(1, 9999);
    m_copiesX->setValue(1);
    m_copiesX->setSuffix(QStringLiteral("  cols"));
    m_copiesX->setButtonSymbols(QAbstractSpinBox::PlusMinus);
    copiesLayout->addRow(QStringLiteral("Copies X:"), m_copiesX);

    m_copiesY = new QSpinBox(copiesGroup);
    m_copiesY->setRange(1, 9999);
    m_copiesY->setValue(1);
    m_copiesY->setSuffix(QStringLiteral("  rows"));
    m_copiesY->setButtonSymbols(QAbstractSpinBox::PlusMinus);
    copiesLayout->addRow(QStringLiteral("Copies Y:"), m_copiesY);

    mainLayout->addWidget(copiesGroup);

    // --- Offsets ---
    auto* offsetGroup = new QWidget(this);
    auto* offsetLayout = new QFormLayout(offsetGroup);
    offsetLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    offsetLayout->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
    offsetLayout->setHorizontalSpacing(12);
    offsetLayout->setVerticalSpacing(8);

    m_offsetX = new QDoubleSpinBox(offsetGroup);
    m_offsetX->setRange(kMinOffset, kMaxOffset);
    m_offsetX->setDecimals(2);
    m_offsetX->setSingleStep(1.0);
    m_offsetX->setSuffix(QStringLiteral(" mm"));
    offsetLayout->addRow(QStringLiteral("Offset X:"), m_offsetX);

    m_offsetY = new QDoubleSpinBox(offsetGroup);
    m_offsetY->setRange(kMinOffset, kMaxOffset);
    m_offsetY->setDecimals(2);
    m_offsetY->setSingleStep(1.0);
    m_offsetY->setSuffix(QStringLiteral(" mm"));
    offsetLayout->addRow(QStringLiteral("Offset Y:"), m_offsetY);

    m_gapX = new QDoubleSpinBox(offsetGroup);
    m_gapX->setRange(kMinGap, kMaxGap);
    m_gapX->setDecimals(2);
    m_gapX->setSingleStep(1.0);
    m_gapX->setSuffix(QStringLiteral(" mm"));
    offsetLayout->addRow(QStringLiteral("Gap X:"), m_gapX);

    m_gapY = new QDoubleSpinBox(offsetGroup);
    m_gapY->setRange(kMinGap, kMaxGap);
    m_gapY->setDecimals(2);
    m_gapY->setSingleStep(1.0);
    m_gapY->setSuffix(QStringLiteral(" mm"));
    offsetLayout->addRow(QStringLiteral("Gap Y:"), m_gapY);

    mainLayout->addWidget(offsetGroup);

    // --- Options ---
    auto* optionsGroup = new QWidget(this);
    auto* optionsLayout = new QFormLayout(optionsGroup);
    optionsLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    optionsLayout->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
    optionsLayout->setHorizontalSpacing(12);
    optionsLayout->setVerticalSpacing(8);

    m_stagger = new QCheckBox(QStringLiteral("Stagger alternate rows"), optionsGroup);
    optionsLayout->addRow(m_stagger);

    m_staggerOffset = new QDoubleSpinBox(optionsGroup);
    m_staggerOffset->setRange(-kStaggerMax, kStaggerMax);
    m_staggerOffset->setDecimals(2);
    m_staggerOffset->setSingleStep(1.0);
    m_staggerOffset->setSuffix(QStringLiteral(" mm"));
    m_staggerOffset->setEnabled(false);
    optionsLayout->addRow(QStringLiteral("Stagger offset:"), m_staggerOffset);

    m_continuous = new QCheckBox(QStringLiteral("Continuous (no gap between repetitions)"), optionsGroup);
    optionsLayout->addRow(m_continuous);

    m_copyObjects = new QCheckBox(QStringLiteral("Copy objects (leave original)"), optionsGroup);
    m_copyObjects->setChecked(true);
    optionsLayout->addRow(m_copyObjects);

    mainLayout->addWidget(optionsGroup);

    // Connect stagger checkbox to enable/disable stagger offset
    connect(m_stagger, &QCheckBox::toggled, m_staggerOffset, &QDoubleSpinBox::setEnabled);

    // --- Buttons ---
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_stepRepeatButton = new QPushButton(QStringLiteral("Step & Repeat"), this);
    m_stepRepeatButton->setDefault(true);
    m_stepRepeatButton->setFixedWidth(120);
    connect(m_stepRepeatButton, &QPushButton::clicked, this, &StepAndRepeatDialog::onStepRepeatClicked);
    buttonLayout->addWidget(m_stepRepeatButton);

    m_cancelButton = new QPushButton(QStringLiteral("Cancel"), this);
    m_cancelButton->setFixedWidth(100);
    connect(m_cancelButton, &QPushButton::clicked, this, &StepAndRepeatDialog::onCancelClicked);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}

void StepAndRepeatDialog::setParams(const Params& params)
{
    m_copiesX->setValue(params.copiesX);
    m_copiesY->setValue(params.copiesY);
    m_offsetX->setValue(params.offsetX);
    m_offsetY->setValue(params.offsetY);
    m_gapX->setValue(params.gapX);
    m_gapY->setValue(params.gapY);
    m_stagger->setChecked(params.stagger);
    m_staggerOffset->setValue(params.staggerOffset);
    m_staggerOffset->setEnabled(params.stagger);
    m_continuous->setChecked(params.continuous);
    m_copyObjects->setChecked(params.copyObjects);
}

StepAndRepeatDialog::Params StepAndRepeatDialog::params() const
{
    Params p;
    p.copiesX = m_copiesX->value();
    p.copiesY = m_copiesY->value();
    p.offsetX = m_offsetX->value();
    p.offsetY = m_offsetY->value();
    p.gapX = m_gapX->value();
    p.gapY = m_gapY->value();
    p.stagger = m_stagger->isChecked();
    p.staggerOffset = m_staggerOffset->value();
    p.continuous = m_continuous->isChecked();
    p.copyObjects = m_copyObjects->isChecked();
    return p;
}

void StepAndRepeatDialog::onStepRepeatClicked()
{
    accept();
}

void StepAndRepeatDialog::onCancelClicked()
{
    reject();
}

} // namespace artplus::ui