// =================================================================== //
// Copyright (C) 2018 Kimura Ryo                                       //
//                                                                     //
// This Source Code Form is subject to the terms of the Mozilla Public //
// License, v. 2.0. If a copy of the MPL was not distributed with this //
// file, You can obtain one at http://mozilla.org/MPL/2.0/.            //
// =================================================================== //

#include "AnalyticBsdfDockWidget.h"

#include <iostream>

#include <libbsdf/Brdf/HalfDifferenceCoordinatesBrdf.h>
#include <libbsdf/Brdf/SpecularCoordinatesBrdf.h>
#include <libbsdf/Brdf/SphericalCoordinatesBrdf.h>

#include "ColorButton.h"
#include "Utility.h"

AnalyticBsdfDockWidget::AnalyticBsdfDockWidget(QWidget* parent)
                                               : QDockWidget(parent),
                                                 ui_(new Ui::ReflectanceModelDockWidgetBase)
{
    ui_->setupUi(this);
}

AnalyticBsdfDockWidget::~AnalyticBsdfDockWidget()
{
    for (auto it = reflectanceModels_.begin(); it != reflectanceModels_.end(); ++it) {
        delete it->second;
    }

    delete ui_;
}

void AnalyticBsdfDockWidget::updateParameterWidget(int index)
{
    // Remove items.
    while (ui_->parameterFormLayout->count() > 0) {
        QLayoutItem* item = ui_->parameterFormLayout->takeAt(0);
        delete item->widget();
        delete item;
    }

    currentParameters_.clear();

    // Add labels and fields.
    QString name = ui_->reflectanceModelComboBox->itemText(index);
    lb::ReflectanceModel* model = reflectanceModels_[name.toLocal8Bit().data()];
    lb::ReflectanceModel::Parameters& params = model->getParameters();
    for (auto it = params.begin(); it != params.end(); ++it) {
        QWidget* paramWidget = 0;

        switch (it->getType()) {
            case lb::ReflectanceModel::Parameter::FLOAT_PARAMETER:
            {
                QDoubleSpinBox* spinBox = new QDoubleSpinBox(ui_->parameterWidget);
                
                spinBox->setMinimum(*it->getMinFloat());
                spinBox->setMaximum(*it->getMaxFloat());
                spinBox->setMaximumWidth(75);

                if (spinBox->maximum() >= 1000.0) {
                    spinBox->setDecimals(4);
                    spinBox->setSingleStep(1.0);
                }
                else {
                    spinBox->setDecimals(5);
                    spinBox->setSingleStep(0.1);
                }

                spinBox->setValue(*it->getFloat());

                connect(spinBox, SIGNAL(editingFinished()),
                        this, SLOT(updateParameter()));

                paramWidget = spinBox;

                break;
            }
            case lb::ReflectanceModel::Parameter::VEC3_PARAMETER:
            {
                ColorButton* colorButton = new ColorButton(ui_->parameterWidget);
                colorButton->setAutoFillBackground(true);
                colorButton->setFlat(false);
                colorButton->setMaximumWidth(40);
                colorButton->setColor(util::lbToQt(*it->getVec3()));

                connect(colorButton, SIGNAL(colorChanged(QColor)),
                        this, SLOT(updateParameter()));

                paramWidget = colorButton;

                break;
            }
            case lb::ReflectanceModel::Parameter::INT_PARAMETER:
            {
                QSpinBox* spinBox = new QSpinBox(ui_->parameterWidget);
                spinBox->setMinimum(*it->getMinInt());
                spinBox->setMaximum(*it->getMaxInt());
                spinBox->setSingleStep(1);
                spinBox->setMaximumWidth(75);
                spinBox->setValue(*it->getInt());

                connect(spinBox, SIGNAL(editingFinished()),
                        this, SLOT(updateParameter()));

                paramWidget = spinBox;

                break;
            }
            default:
                break;
        }

        if (paramWidget) {
            std::string label = it->getName() + ":";
            paramWidget->setToolTip(it->getDescription().c_str());
            ui_->parameterFormLayout->addRow(label.c_str(), paramWidget);
            currentParameters_[paramWidget] = &(*it);
        }
    }
}

void AnalyticBsdfDockWidget::updateCoordSysWidget(int index)
{
    QString name = ui_->reflectanceModelComboBox->itemText(index);
    lb::ReflectanceModel* model = reflectanceModels_[name.toLocal8Bit().data()];

    QLabel* halfLabel = ui_->halfDiffCsNumAngle1Label;
    QLabel* specLabel = ui_->specularCsNumAngle1Label;
    QLabel* spheLabel = ui_->sphericalCsNumAngle1Label;
    QSpinBox* halfSpinBox = ui_->halfDiffCsNumAngle1SpinBox;
    QSpinBox* specSpinBox = ui_->specularCsNumAngle1SpinBox;
    QSpinBox* spheSpinBox = ui_->sphericalCsNumAngle1SpinBox;

    // Hide labels and fields for an anisotropic BRDF if an isotropic BSDF model is selected.
    if (model->isIsotropic()) {
        halfLabel->hide();
        halfSpinBox->hide();
        ui_->halfDiffCoordSysFormLayout->removeWidget(halfLabel);
        ui_->halfDiffCoordSysFormLayout->removeWidget(halfSpinBox);

        specLabel->hide();
        specSpinBox->hide();
        ui_->specularCoordSysFormLayout->removeWidget(specLabel);
        ui_->specularCoordSysFormLayout->removeWidget(specSpinBox);

        spheLabel->hide();
        spheSpinBox->hide();
        ui_->sphericalCoordSysFormLayout->removeWidget(spheLabel);
        ui_->sphericalCoordSysFormLayout->removeWidget(spheSpinBox);
    }
    else {
        ui_->halfDiffCoordSysFormLayout->insertRow(2, halfLabel, halfSpinBox);
        halfLabel->show();
        halfSpinBox->show();

        ui_->specularCoordSysFormLayout->insertRow(2, specLabel, specSpinBox);
        specLabel->show();
        specSpinBox->show();

        ui_->sphericalCoordSysFormLayout->insertRow(2, spheLabel, spheSpinBox);
        spheLabel->show();
        spheSpinBox->show();
    }
}

void AnalyticBsdfDockWidget::updateParameter()
{
    for (auto it = currentParameters_.begin(); it != currentParameters_.end(); ++it) {
        if (QDoubleSpinBox* spinBox = dynamic_cast<QDoubleSpinBox*>(it->first)) {
            *it->second->getFloat() = spinBox->value();
        }
        else if (ColorButton* colorButton = dynamic_cast<ColorButton*>(it->first)) {
            *it->second->getVec3() = util::qtToLb(colorButton->getColor());
        }
        else if (QSpinBox* spinBox = dynamic_cast<QSpinBox*>(it->first)) {
            *it->second->getInt() = spinBox->value();
        }
    }
}

void AnalyticBsdfDockWidget::initializeReflectanceModels()
{
    for (auto it = reflectanceModels_.begin(); it != reflectanceModels_.end(); ++it) {
        ui_->reflectanceModelComboBox->addItem(it->first.c_str());
    }

    connect(ui_->reflectanceModelComboBox, SIGNAL(activated(int)),
            this, SLOT(updateParameterWidget(int)));

    connect(ui_->reflectanceModelComboBox, SIGNAL(activated(int)),
            this, SLOT(updateCoordSysWidget(int)));
}

lb::Brdf* AnalyticBsdfDockWidget::initializeBrdf(bool isotropic)
{
    lb::Brdf* brdf;
    std::string coordinateSystemName(ui_->coordSysComboBox->currentText().toLocal8Bit());
    if (coordinateSystemName == "Half difference") {
        lb::Arrayf halfThetaAngles  = lb::Arrayf::LinSpaced(ui_->halfDiffCsNumAngle0SpinBox->value() + 1,
                                                            0.0, lb::HalfDifferenceCoordinateSystem::MAX_ANGLE0);

        lb::Arrayf halfPhiAngles;
        if (isotropic || ui_->halfDiffCsNumAngle1SpinBox->value() <= 1) {
            halfPhiAngles.setZero(1);
        }
        else {
            halfPhiAngles.setLinSpaced(ui_->halfDiffCsNumAngle1SpinBox->value() + 1,
                                       0.0, lb::HalfDifferenceCoordinateSystem::MAX_ANGLE1);
        }

        lb::Arrayf diffThetaAngles  = lb::Arrayf::LinSpaced(ui_->halfDiffCsNumAngle2SpinBox->value() + 1,
                                                            0.0, lb::HalfDifferenceCoordinateSystem::MAX_ANGLE2);
        lb::Arrayf diffPhiAngles    = lb::Arrayf::LinSpaced(ui_->halfDiffCsNumAngle3SpinBox->value() + 1,
                                                            0.0, lb::HalfDifferenceCoordinateSystem::MAX_ANGLE3);

        // Create narrow intervals near specular directions.
        for (int i = 1; i < halfThetaAngles.size() - 1; ++i) {
            lb::Arrayf::Scalar ratio = halfThetaAngles[i] / lb::HalfDifferenceCoordinateSystem::MAX_ANGLE2;
            ratio = std::pow(ratio, static_cast<lb::Arrayf::Scalar>(2.0));
            halfThetaAngles[i] = ratio * lb::HalfDifferenceCoordinateSystem::MAX_ANGLE2;
        }

        brdf = new lb::HalfDifferenceCoordinatesBrdf(halfThetaAngles.size(),
                                                     halfPhiAngles.size(),
                                                     diffThetaAngles.size(),
                                                     diffPhiAngles.size(),
                                                     lb::RGB_MODEL, 3, false);

        lb::SampleSet* ss = brdf->getSampleSet();
        ss->getAngles0() = halfThetaAngles;
        ss->getAngles1() = halfPhiAngles;
        ss->getAngles2() = diffThetaAngles;
        ss->getAngles3() = diffPhiAngles;
    }
    else if (coordinateSystemName == "Specular") {
        lb::Arrayf inThetaAngles    = lb::Arrayf::LinSpaced(ui_->specularCsNumAngle0SpinBox->value() + 1,
                                                            0.0, lb::SpecularCoordinateSystem::MAX_ANGLE0);

        lb::Arrayf inPhiAngles;
        if (isotropic || ui_->specularCsNumAngle1SpinBox->value() <= 1) {
            inPhiAngles.setZero(1);
        }
        else {
            inPhiAngles.setLinSpaced(ui_->specularCsNumAngle1SpinBox->value() + 1,
                                     0.0, lb::SpecularCoordinateSystem::MAX_ANGLE1);
        }

        lb::Arrayf specThetaAngles  = lb::Arrayf::LinSpaced(ui_->specularCsNumAngle2SpinBox->value() + 1,
                                                            0.0, lb::SpecularCoordinateSystem::MAX_ANGLE2);
        lb::Arrayf specPhiAngles    = lb::Arrayf::LinSpaced(ui_->specularCsNumAngle3SpinBox->value() + 1,
                                                            0.0, lb::SpecularCoordinateSystem::MAX_ANGLE3);

        // Create narrow intervals near specular directions.
        for (int i = 1; i < specThetaAngles.size() - 1; ++i) {
            lb::Arrayf::Scalar ratio = specThetaAngles[i] / lb::SpecularCoordinateSystem::MAX_ANGLE2;
            ratio = std::pow(ratio, static_cast<lb::Arrayf::Scalar>(2.0));
            specThetaAngles[i] = ratio * lb::SpecularCoordinateSystem::MAX_ANGLE2;
        }

        brdf = new lb::SpecularCoordinatesBrdf(inThetaAngles.size(),
                                               inPhiAngles.size(),
                                               specThetaAngles.size(),
                                               specPhiAngles.size(),
                                               lb::RGB_MODEL, 3, false);

        lb::SampleSet* ss = brdf->getSampleSet();
        ss->getAngles0() = inThetaAngles;
        ss->getAngles1() = inPhiAngles;
        ss->getAngles2() = specThetaAngles;
        ss->getAngles3() = specPhiAngles;
    }
    else if (coordinateSystemName == "Spherical") {
        int spinBox1Val = ui_->sphericalCsNumAngle1SpinBox->value();
        int numAngle1 = (isotropic || spinBox1Val <= 1) ? 1 : (spinBox1Val + 1);
        brdf = new lb::SphericalCoordinatesBrdf(ui_->sphericalCsNumAngle0SpinBox->value() + 1,
                                                numAngle1,
                                                ui_->sphericalCsNumAngle2SpinBox->value() + 1,
                                                ui_->sphericalCsNumAngle3SpinBox->value() + 1,
                                                lb::RGB_MODEL, 3, true);
    }
    else {
        std::cout
            << "[AnalyticBsdfDockWidget::initializeBrdf] Invalid coordinate system: "
            << coordinateSystemName << std::endl;
        return 0;
    }

    brdf->setSourceType(lb::GENERATED_SOURCE);

    return brdf;
}