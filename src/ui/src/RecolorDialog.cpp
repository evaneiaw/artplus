#include "artplus/ui/RecolorDialog.h"

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
constexpr double kMinTolerance = 0.1;
constexpr double kMaxTolerance = 100.0;
} // namespace

RecolorDialog::RecolorDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Recolor Artwork"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setMinimumWidth(420);
    buildUi();
}

void RecolorDialog::buildUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // --- Color Space ---
    auto* spaceGroup = new QWidget(this);
    auto* spaceLayout = new QFormLayout(spaceGroup);
    spaceLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    spaceLayout->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
    spaceLayout->setHorizontalSpacing(12);
    spaceLayout->setVerticalSpacing(8);

    m_colorSpace = new QComboBox(spaceGroup);
    m_colorSpace->addItems({QStringLiteral("CMYK"), QStringLiteral("RGB"), QStringLiteral("Lab"), QStringLiteral("Grayscale")});
    spaceLayout->addRow(QStringLiteral("Color space:"), m_colorSpace);

    mainLayout->addWidget(spaceGroup);

    // --- Target Color ---
    auto* targetGroup = new QWidget(this);
    auto* targetLayout = new QFormLayout(targetGroup);
    targetLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    targetLayout->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
    targetLayout->setHorizontalSpacing(12);
    targetLayout->setVerticalSpacing(8);

    m_targetColor = new QComboBox(targetGroup);
    m_targetColor->addItems({QStringLiteral("Cyan"), QStringLiteral("Magenta"), QStringLiteral("Yellow"),
                             QStringLiteral("Black"), QStringLiteral("Red"), QStringLiteral("Green"),
                             QStringLiteral("Blue"), QStringLiteral("Custom...")});
    targetLayout->addRow(QStringLiteral("Target color:"), m_targetColor);

    mainLayout->addWidget(targetGroup);

    // --- Tolerance ---
    auto* tolGroup = new QWidget(this);
    auto* tolLayout = new QFormLayout(tolGroup);
    tolLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    tolLayout->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
    tolLayout->setHorizontalSpacing(12);
    tolLayout->setVerticalSpacing(8);

    m_tolerance = new QDoubleSpinBox(tolGroup);
    m_tolerance->setRange(0.1, 100.0);
    m_tolerance->setDecimals(1);
    m_tolerance->setSingleStep(1.0);
    m_tolerance->setSuffix(QStringLiteral(" %"));
    m_tolerance->setValue(10.0);
    tolLayout->addRow(QStringLiteral("Tolerance:"), m_tolerance);

    mainLayout->addWidget(tolGroup);

    // --- Options ---
    auto* optionsGroup = new QWidget(this);
    auto* optionsLayout = new QFormLayout(optionsGroup);
    optionsLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    optionsLayout->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
    optionsLayout->setHorizontalSpacing(12);
    optionsLayout->setVerticalSpacing(8);

    m_preserveBlack = new QCheckBox(QStringLiteral("Preserve black"), optionsGroup);
    m_preserveBlack->setChecked(true);
    optionsLayout->addRow(m_preserveBlack);

    m_preserveWhite = new QCheckBox(QStringLiteral("Preserve white"), optionsGroup);
    m_preserveWhite->setChecked(true);
    optionsLayout->addRow(m_preserveWhite);

    m_useICC = new QCheckBox(QStringLiteral("Use ICC profiles"), optionsGroup);
    optionsLayout->addRow(m_useICC);

    mainLayout->addWidget(optionsGroup);

    // --- Buttons ---
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_recolorButton = new QPushButton(QStringLiteral("Recolor"), this);
    m_recolorButton->setDefault(true);
    m_recolorButton->setFixedWidth(120);
    connect(m_recolorButton, &QPushButton::clicked, this, &RecolorDialog::onRecolorClicked);
    buttonLayout->addWidget(m_recolorButton);

    m_cancelButton = new QPushButton(QStringLiteral("Cancel"), this);
    m_cancelButton->setFixedWidth(100);
    connect(m_cancelButton, &QPushButton::clicked, this, &RecolorDialog::onCancelClicked);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}

void RecolorDialog::setParams(const Params& params)
{
    m_colorSpace->setCurrentText(params.colorSpace);
    m_targetColor->setCurrentText(params.targetColor);
    m_tolerance->setValue(params.tolerance);
    m_preserveBlack->setChecked(params.preserveBlack);
    m_preserveWhite->setChecked(params.preserveWhite);
    m_useICC->setChecked(params.useICC);
}

RecolorDialog::Params RecolorDialog::params() const
{
    Params p;
    p.colorSpace = m_colorSpace->currentText();
    p.targetColor = m_targetColor->currentText();
    p.tolerance = m_tolerance->value();
    p.preserveBlack = m_preserveBlack->isChecked();
    p.preserveWhite = m_preserveWhite->isChecked();
    p.useICC = m_useICC->isChecked();
    return p;
}

void RecolorDialog::onRecolorClicked()
{
    accept();
}

void RecolorDialog::onCancelClicked()
{
    reject();
}

} // namespace artplus::ui