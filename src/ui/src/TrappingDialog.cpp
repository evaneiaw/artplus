#include "artplus/ui/TrappingDialog.h"

#include "artplus/ui/Theme.h"

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace artplus::ui {

namespace {
constexpr double kMinTrapWidth = 0.01;
constexpr double kMaxTrapWidth = 10.0;
constexpr double kMinMaxTrapWidth = 0.01;
constexpr double kMaxMaxTrapWidth = 20.0;
constexpr double kMinPercent = 0.0;
constexpr double kMaxPercent = 100.0;
constexpr double kMinMiter = 1.0;
constexpr double kMaxMiter = 100.0;
} // namespace

TrappingDialog::TrappingDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Trapping"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setMinimumWidth(420);
    buildUi();
}

void TrappingDialog::buildUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // --- Trap Width ---
    auto* widthGroup = new QWidget(this);
    auto* widthLayout = new QFormLayout(widthGroup);
    widthLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    widthLayout->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
    widthLayout->setHorizontalSpacing(12);
    widthLayout->setVerticalSpacing(8);

    m_trapWidth = new QDoubleSpinBox(widthGroup);
    m_trapWidth->setRange(kMinTrapWidth, kMaxTrapWidth);
    m_trapWidth->setDecimals(2);
    m_trapWidth->setSingleStep(0.01);
    m_trapWidth->setSuffix(QStringLiteral(" mm"));
    widthLayout->addRow(QStringLiteral("Trap width:"), m_trapWidth);

    m_maxTrapWidth = new QDoubleSpinBox(widthGroup);
    m_maxTrapWidth->setRange(kMinMaxTrapWidth, kMaxMaxTrapWidth);
    m_maxTrapWidth->setDecimals(2);
    m_maxTrapWidth->setSingleStep(0.1);
    m_maxTrapWidth->setSuffix(QStringLiteral(" mm"));
    widthLayout->addRow(QStringLiteral("Maximum trap width:"), m_maxTrapWidth);

    mainLayout->addWidget(widthGroup);

    // --- Limits ---
    auto* limitsGroup = new QWidget(this);
    auto* limitsLayout = new QFormLayout(limitsGroup);
    limitsLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    limitsLayout->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
    limitsLayout->setHorizontalSpacing(12);
    limitsLayout->setVerticalSpacing(8);

    m_blackLimit = new QCheckBox(QStringLiteral("Black limit"), limitsGroup);
    m_blackLimit->setChecked(true);
    limitsLayout->addRow(m_blackLimit);

    m_blackLimitValue = new QDoubleSpinBox(limitsGroup);
    m_blackLimitValue->setRange(kMinPercent, kMaxPercent);
    m_blackLimitValue->setDecimals(1);
    m_blackLimitValue->setSingleStep(1.0);
    m_blackLimitValue->setSuffix(QStringLiteral(" %"));
    m_blackLimitValue->setValue(30.0);
    limitsLayout->addRow(QStringLiteral("Black limit value:"), m_blackLimitValue);

    m_richBlackLimit = new QCheckBox(QStringLiteral("Rich black limit"), limitsGroup);
    m_richBlackLimit->setChecked(true);
    limitsLayout->addRow(m_richBlackLimit);

    m_richBlackLimitValue = new QDoubleSpinBox(limitsGroup);
    m_richBlackLimitValue->setRange(kMinPercent, kMaxPercent);
    m_richBlackLimitValue->setDecimals(1);
    m_richBlackLimitValue->setSingleStep(1.0);
    m_richBlackLimitValue->setSuffix(QStringLiteral(" %"));
    m_richBlackLimitValue->setValue(50.0);
    limitsLayout->addRow(QStringLiteral("Rich black limit value:"), m_richBlackLimitValue);

    mainLayout->addWidget(limitsGroup);

    // --- Sliding Trap ---
    auto* slidingGroup = new QWidget(this);
    auto* slidingLayout = new QFormLayout(slidingGroup);
    slidingLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    slidingLayout->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
    slidingLayout->setHorizontalSpacing(12);
    slidingLayout->setVerticalSpacing(8);

    m_slidingTrap = new QCheckBox(QStringLiteral("Sliding trap"), slidingGroup);
    slidingLayout->addRow(m_slidingTrap);

    m_slidingTrapValue = new QDoubleSpinBox(slidingGroup);
    m_slidingTrapValue->setRange(kMinPercent, kMaxPercent);
    m_slidingTrapValue->setDecimals(1);
    m_slidingTrapValue->setSingleStep(1.0);
    m_slidingTrapValue->setSuffix(QStringLiteral(" %"));
    m_slidingTrapValue->setValue(50.0);
    m_slidingTrapValue->setEnabled(false);
    slidingLayout->addRow(QStringLiteral("Sliding trap value:"), m_slidingTrapValue);

    connect(m_slidingTrap, &QCheckBox::toggled, m_slidingTrapValue, &QDoubleSpinBox::setEnabled);

    mainLayout->addWidget(slidingGroup);

    // --- Options ---
    auto* optionsGroup = new QWidget(this);
    auto* optionsLayout = new QFormLayout(optionsGroup);
    optionsLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    optionsLayout->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
    optionsLayout->setHorizontalSpacing(12);
    optionsLayout->setVerticalSpacing(8);

    m_overprintBlack = new QCheckBox(QStringLiteral("Overprint black"), optionsGroup);
    m_overprintBlack->setChecked(true);
    optionsLayout->addRow(m_overprintBlack);

    m_trapToRichBlack = new QCheckBox(QStringLiteral("Trap to rich black"), optionsGroup);
    optionsLayout->addRow(m_trapToRichBlack);

    // Join style
    m_joinStyleRound = new QCheckBox(QStringLiteral("Round joins (off = miter)"), optionsGroup);
    m_joinStyleRound->setChecked(true);
    optionsLayout->addRow(m_joinStyleRound);

    m_miterLimit = new QDoubleSpinBox(optionsGroup);
    m_miterLimit->setRange(kMinMiter, kMaxMiter);
    m_miterLimit->setDecimals(1);
    m_miterLimit->setSingleStep(0.5);
    m_miterLimit->setValue(4.0);
    m_miterLimit->setEnabled(false);
    optionsLayout->addRow(QStringLiteral("Miter limit:"), m_miterLimit);

    connect(m_joinStyleRound, &QCheckBox::toggled, [this](bool checked) {
        m_miterLimit->setEnabled(!checked);
    });

    mainLayout->addWidget(optionsGroup);

    // --- Buttons ---
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_trapButton = new QPushButton(QStringLiteral("Trap"), this);
    m_trapButton->setDefault(true);
    m_trapButton->setFixedWidth(120);
    connect(m_trapButton, &QPushButton::clicked, this, &TrappingDialog::onTrapClicked);
    buttonLayout->addWidget(m_trapButton);

    m_cancelButton = new QPushButton(QStringLiteral("Cancel"), this);
    m_cancelButton->setFixedWidth(100);
    connect(m_cancelButton, &QPushButton::clicked, this, &TrappingDialog::onCancelClicked);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}

void TrappingDialog::setParams(const Params& params)
{
    m_trapWidth->setValue(params.trapWidth);
    m_maxTrapWidth->setValue(params.maxTrapWidth);
    m_blackLimit->setChecked(params.blackLimit);
    m_blackLimitValue->setValue(params.blackLimitValue);
    m_richBlackLimit->setChecked(params.richBlackLimit);
    m_richBlackLimitValue->setValue(params.richBlackLimitValue);
    m_slidingTrap->setChecked(params.slidingTrap);
    m_slidingTrapValue->setValue(params.slidingTrapValue);
    m_slidingTrapValue->setEnabled(params.slidingTrap);
    m_overprintBlack->setChecked(params.overprintBlack);
    m_trapToRichBlack->setChecked(params.trapToRichBlack);
    m_joinStyleRound->setChecked(params.joinStyle);
    m_miterLimit->setValue(params.miterLimit);
    m_miterLimit->setEnabled(!params.joinStyle);
}

TrappingDialog::Params TrappingDialog::params() const
{
    Params p;
    p.trapWidth = m_trapWidth->value();
    p.maxTrapWidth = m_maxTrapWidth->value();
    p.blackLimit = m_blackLimit->isChecked();
    p.blackLimitValue = m_blackLimitValue->value();
    p.richBlackLimit = m_richBlackLimit->isChecked();
    p.richBlackLimitValue = m_richBlackLimitValue->value();
    p.slidingTrap = m_slidingTrap->isChecked();
    p.slidingTrapValue = m_slidingTrapValue->value();
    p.overprintBlack = m_overprintBlack->isChecked();
    p.trapToRichBlack = m_trapToRichBlack->isChecked();
    p.joinStyle = m_joinStyleRound->isChecked();
    p.miterLimit = m_miterLimit->value();
    return p;
}

void TrappingDialog::onTrapClicked()
{
    accept();
}

void TrappingDialog::onCancelClicked()
{
    reject();
}

} // namespace artplus::ui