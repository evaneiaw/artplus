#include "artplus/ui/WrapDialog.h"

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
constexpr double kMinOffset = 0.0;
constexpr double kMaxOffset = 1000.0;
} // namespace

WrapDialog::WrapDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Text Wrap"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setMinimumWidth(420);
    buildUi();
}

void WrapDialog::buildUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // --- Wrap Type ---
    auto* typeGroup = new QWidget(this);
    auto* typeLayout = new QFormLayout(typeGroup);
    typeLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    typeLayout->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
    typeLayout->setHorizontalSpacing(12);
    typeLayout->setVerticalSpacing(8);

    m_wrapType = new QComboBox(typeGroup);
    m_wrapType->addItems({QStringLiteral("No Text Wrap"), QStringLiteral("Bounding Box"),
                          QStringLiteral("Object Shape"), QStringLiteral("Jump Object"),
                          QStringLiteral("Jump to Next Column")});
    typeLayout->addRow(QStringLiteral("Wrap type:"), m_wrapType);

    mainLayout->addWidget(typeGroup);

    // --- Offset ---
    auto* offsetGroup = new QWidget(this);
    auto* offsetLayout = new QFormLayout(offsetGroup);
    offsetLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    offsetLayout->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
    offsetLayout->setHorizontalSpacing(12);
    offsetLayout->setVerticalSpacing(8);

    m_offset = new QDoubleSpinBox(offsetGroup);
    m_offset->setRange(0.0, 1000.0);
    m_offset->setDecimals(1);
    m_offset->setSingleStep(1.0);
    m_offset->setSuffix(QStringLiteral(" pt"));
    m_offset->setValue(6.0);
    offsetLayout->addRow(QStringLiteral("Offset:"), m_offset);

    mainLayout->addWidget(offsetGroup);

    // --- Options ---
    auto* optionsGroup = new QWidget(this);
    auto* optionsLayout = new QFormLayout(optionsGroup);
    optionsLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    optionsLayout->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
    optionsLayout->setHorizontalSpacing(12);
    optionsLayout->setVerticalSpacing(8);

    m_invert = new QCheckBox(QStringLiteral("Invert wrap"), optionsGroup);
    optionsLayout->addRow(m_invert);

    m_wrapInside = new QCheckBox(QStringLiteral("Wrap inside"), optionsGroup);
    optionsLayout->addRow(m_wrapInside);

    m_applyToAll = new QCheckBox(QStringLiteral("Apply to all selected"), optionsGroup);
    optionsLayout->addRow(m_applyToAll);

    mainLayout->addWidget(optionsGroup);

    // --- Buttons ---
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_wrapButton = new QPushButton(QStringLiteral("Wrap"), this);
    m_wrapButton->setDefault(true);
    m_wrapButton->setFixedWidth(120);
    connect(m_wrapButton, &QPushButton::clicked, this, &WrapDialog::onWrapClicked);
    buttonLayout->addWidget(m_wrapButton);

    m_cancelButton = new QPushButton(QStringLiteral("Cancel"), this);
    m_cancelButton->setFixedWidth(100);
    connect(m_cancelButton, &QPushButton::clicked, this, &WrapDialog::onCancelClicked);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}

void WrapDialog::setParams(const Params& params)
{
    m_wrapType->setCurrentText(params.wrapType);
    m_offset->setValue(params.offset);
    m_invert->setChecked(params.invert);
    m_wrapInside->setChecked(params.wrapInside);
    m_applyToAll->setChecked(params.applyToAll);
}

WrapDialog::Params WrapDialog::params() const
{
    Params p;
    p.wrapType = m_wrapType->currentText();
    p.offset = m_offset->value();
    p.invert = m_invert->isChecked();
    p.wrapInside = m_wrapInside->isChecked();
    p.applyToAll = m_applyToAll->isChecked();
    return p;
}

void WrapDialog::onWrapClicked()
{
    accept();
}

void WrapDialog::onCancelClicked()
{
    reject();
}

} // namespace artplus::ui