#include "artplus/ui/ShaperDialog.h"

#include "artplus/ui/Theme.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace artplus::ui {

namespace {
constexpr double kMinBend = -100.0;
constexpr double kMaxBend = 100.0;
constexpr double kMinDistortion = -100.0;
constexpr double kMaxDistortion = 100.0;
} // namespace

ShaperDialog::ShaperDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Envelope Distort (Shaper)"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setMinimumWidth(420);
    buildUi();
}

void ShaperDialog::buildUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // --- Shape Type ---
    auto* shapeGroup = new QWidget(this);
    auto* shapeLayout = new QFormLayout(shapeGroup);
    shapeLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    shapeLayout->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
    shapeLayout->setHorizontalSpacing(12);
    shapeLayout->setVerticalSpacing(8);

    m_shapeType = new QComboBox(shapeGroup);
    m_shapeType->addItems({QStringLiteral("Arc"), QStringLiteral("Flag"), QStringLiteral("Wave"),
                           QStringLiteral("Fish"), QStringLiteral("Rise"), QStringLiteral("Fish Eye"),
                           QStringLiteral("Inflate"), QStringLiteral("Squeeze"), QStringLiteral("Twist")});
    shapeLayout->addRow(QStringLiteral("Style:"), m_shapeType);

    mainLayout->addWidget(shapeGroup);

    // --- Bend ---
    auto* bendGroup = new QWidget(this);
    auto* bendLayout = new QFormLayout(bendGroup);
    bendLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    bendLayout->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
    bendLayout->setHorizontalSpacing(12);
    bendLayout->setVerticalSpacing(8);

    m_bend = new QDoubleSpinBox(bendGroup);
    m_bend->setRange(-100.0, 100.0);
    m_bend->setDecimals(1);
    m_bend->setSingleStep(1.0);
    m_bend->setSuffix(QStringLiteral(" %"));
    m_bend->setValue(0.0);
    bendLayout->addRow(QStringLiteral("Bend:"), m_bend);

    m_horizontalDistortion = new QDoubleSpinBox(bendGroup);
    m_horizontalDistortion->setRange(-100.0, 100.0);
    m_horizontalDistortion->setDecimals(1);
    m_horizontalDistortion->setSingleStep(1.0);
    m_horizontalDistortion->setSuffix(QStringLiteral(" %"));
    m_horizontalDistortion->setValue(0.0);
    bendLayout->addRow(QStringLiteral("Horizontal distortion:"), m_horizontalDistortion);

    m_verticalDistortion = new QDoubleSpinBox(bendGroup);
    m_verticalDistortion->setRange(-100.0, 100.0);
    m_verticalDistortion->setDecimals(1);
    m_verticalDistortion->setSingleStep(1.0);
    m_verticalDistortion->setSuffix(QStringLiteral(" %"));
    m_verticalDistortion->setValue(0.0);
    bendLayout->addRow(QStringLiteral("Vertical distortion:"), m_verticalDistortion);

    mainLayout->addWidget(bendGroup);

    // --- Options ---
    auto* optionsGroup = new QWidget(this);
    auto* optionsLayout = new QFormLayout(optionsGroup);
    optionsLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    optionsLayout->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
    optionsLayout->setHorizontalSpacing(12);
    optionsLayout->setVerticalSpacing(8);

    m_preview = new QCheckBox(QStringLiteral("Preview"), optionsGroup);
    m_preview->setChecked(true);
    optionsLayout->addRow(m_preview);

    mainLayout->addWidget(optionsGroup);

    // --- Buttons ---
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_shapeButton = new QPushButton(QStringLiteral("Apply"), this);
    m_shapeButton->setDefault(true);
    m_shapeButton->setFixedWidth(120);
    connect(m_shapeButton, &QPushButton::clicked, this, &ShaperDialog::onShapeClicked);
    buttonLayout->addWidget(m_shapeButton);

    m_cancelButton = new QPushButton(QStringLiteral("Cancel"), this);
    m_cancelButton->setFixedWidth(100);
    connect(m_cancelButton, &QPushButton::clicked, this, &ShaperDialog::onCancelClicked);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}

void ShaperDialog::setParams(const Params& params)
{
    m_shapeType->setCurrentText(params.shapeType);
    m_bend->setValue(params.bend);
    m_horizontalDistortion->setValue(params.horizontalDistortion);
    m_verticalDistortion->setValue(params.verticalDistortion);
    m_preview->setChecked(params.preview);
}

ShaperDialog::Params ShaperDialog::params() const
{
    Params p;
    p.shapeType = m_shapeType->currentText();
    p.bend = m_bend->value();
    p.horizontalDistortion = m_horizontalDistortion->value();
    p.verticalDistortion = m_verticalDistortion->value();
    p.preview = m_preview->isChecked();
    return p;
}

void ShaperDialog::onShapeClicked()
{
    accept();
}

void ShaperDialog::onCancelClicked()
{
    reject();
}

} // namespace artplus::ui