#include "artplus/ui/ScreeningDialog.h"

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
constexpr double kMinFrequency = 1.0;
constexpr double kMaxFrequency = 1000.0;
constexpr double kMinAngle = 0.0;
constexpr double kMaxAngle = 180.0;
constexpr double kMinDotShape = 0.0;
constexpr double kMaxDotShape = 10.0;
constexpr double kMinOffset = -180.0;
constexpr double kMaxOffset = 180.0;
} // namespace

ScreeningDialog::ScreeningDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Screening"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setMinimumWidth(420);
    buildUi();
}

void ScreeningDialog::buildUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // --- Screen Set ---
    auto* setGroup = new QWidget(this);
    auto* setLayout = new QFormLayout(setGroup);
    setLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    setLayout->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
    setLayout->setHorizontalSpacing(12);
    setLayout->setVerticalSpacing(8);

    m_screenSet = new QComboBox(setGroup);
    m_screenSet->addItems({QStringLiteral("Default"), QStringLiteral("Custom 1"),
                           QStringLiteral("Custom 2"), QStringLiteral("Custom 3")});
    setLayout->addRow(QStringLiteral("Screen set:"), m_screenSet);

    mainLayout->addWidget(setGroup);

    // --- Frequency & Angle ---
    auto* freqGroup = new QWidget(this);
    auto* freqLayout = new QFormLayout(freqGroup);
    freqLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    freqLayout->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
    freqLayout->setHorizontalSpacing(12);
    freqLayout->setVerticalSpacing(8);

    m_autoFrequency = new QCheckBox(QStringLiteral("Auto frequency"), freqGroup);
    m_autoFrequency->setChecked(true);
    freqLayout->addRow(m_autoFrequency);

    m_frequency = new QDoubleSpinBox(freqGroup);
    m_frequency->setRange(kMinFrequency, kMaxFrequency);
    m_frequency->setDecimals(1);
    m_frequency->setSingleStep(1.0);
    m_frequency->setSuffix(QStringLiteral(" lpi"));
    m_frequency->setValue(150.0);
    m_frequency->setEnabled(false);
    freqLayout->addRow(QStringLiteral("Frequency:"), m_frequency);

    connect(m_autoFrequency, &QCheckBox::toggled, [this](bool checked) {
        m_frequency->setEnabled(!checked);
    });

    m_angle = new QDoubleSpinBox(freqGroup);
    m_angle->setRange(kMinAngle, kMaxAngle);
    m_angle->setDecimals(1);
    m_angle->setSingleStep(1.0);
    m_angle->setSuffix(QStringLiteral("°"));
    m_angle->setValue(45.0);
    freqLayout->addRow(QStringLiteral("Angle:"), m_angle);

    mainLayout->addWidget(freqGroup);

    // --- Dot Shape ---
    auto* dotGroup = new QWidget(this);
    auto* dotLayout = new QFormLayout(dotGroup);
    dotLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    dotLayout->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
    dotLayout->setHorizontalSpacing(12);
    dotLayout->setVerticalSpacing(8);

    m_dotShape = new QDoubleSpinBox(dotGroup);
    m_dotShape->setRange(kMinDotShape, kMaxDotShape);
    m_dotShape->setDecimals(1);
    m_dotShape->setSingleStep(0.1);
    m_dotShape->setValue(0.0);
    dotLayout->addRow(QStringLiteral("Dot shape (0=round..10=square):"), m_dotShape);

    mainLayout->addWidget(dotGroup);

    // --- Options ---
    auto* optionsGroup = new QWidget(this);
    auto* optionsLayout = new QFormLayout(optionsGroup);
    optionsLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    optionsLayout->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
    optionsLayout->setHorizontalSpacing(12);
    optionsLayout->setVerticalSpacing(8);

    m_useAngleOffset = new QCheckBox(QStringLiteral("Use angle offset"), optionsGroup);
    optionsLayout->addRow(m_useAngleOffset);

    m_angleOffset = new QDoubleSpinBox(optionsGroup);
    m_angleOffset->setRange(kMinOffset, kMaxOffset);
    m_angleOffset->setDecimals(1);
    m_angleOffset->setSingleStep(1.0);
    m_angleOffset->setSuffix(QStringLiteral("°"));
    m_angleOffset->setEnabled(false);
    optionsLayout->addRow(QStringLiteral("Angle offset:"), m_angleOffset);

    connect(m_useAngleOffset, &QCheckBox::toggled, m_angleOffset, &QDoubleSpinBox::setEnabled);

    m_accurateScreens = new QCheckBox(QStringLiteral("Accurate screens"), optionsGroup);
    optionsLayout->addRow(m_accurateScreens);

    mainLayout->addWidget(optionsGroup);

    // --- Buttons ---
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_screenButton = new QPushButton(QStringLiteral("Screen"), this);
    m_screenButton->setDefault(true);
    m_screenButton->setFixedWidth(120);
    connect(m_screenButton, &QPushButton::clicked, this, &ScreeningDialog::onScreenClicked);
    buttonLayout->addWidget(m_screenButton);

    m_cancelButton = new QPushButton(QStringLiteral("Cancel"), this);
    m_cancelButton->setFixedWidth(100);
    connect(m_cancelButton, &QPushButton::clicked, this, &ScreeningDialog::onCancelClicked);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);

    this->setLayout(mainLayout);
}

void ScreeningDialog::setParams(const Params& params)
{
    m_screenSet->setCurrentText(params.screenSet);
    m_autoFrequency->setChecked(params.autoFrequency);
    m_frequency->setValue(params.frequency);
    m_frequency->setEnabled(!params.autoFrequency);
    m_angle->setValue(params.angle);
    m_dotShape->setValue(params.dotShape);
    m_useAngleOffset->setChecked(params.useAngleOffset);
    m_angleOffset->setValue(params.angleOffset);
    m_angleOffset->setEnabled(params.useAngleOffset);
    m_accurateScreens->setChecked(params.accurateScreens);
}

ScreeningDialog::Params ScreeningDialog::params() const
{
    Params p;
    p.screenSet = m_screenSet->currentText();
    p.frequency = m_frequency->value();
    p.angle = m_angle->value();
    p.dotShape = m_dotShape->value();
    p.autoFrequency = m_autoFrequency->isChecked();
    p.useAngleOffset = m_useAngleOffset->isChecked();
    p.angleOffset = m_angleOffset->value();
    p.accurateScreens = m_accurateScreens->isChecked();
    return p;
}

void ScreeningDialog::onScreenClicked()
{
    accept();
}

void ScreeningDialog::onCancelClicked()
{
    reject();
}

} // namespace artplus::ui