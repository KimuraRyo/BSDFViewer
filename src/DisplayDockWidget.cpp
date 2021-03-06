// =================================================================== //
// Copyright (C) 2018-2019 Kimura Ryo                                  //
//                                                                     //
// This Source Code Form is subject to the terms of the Mozilla Public //
// License, v. 2.0. If a copy of the MPL was not distributed with this //
// file, You can obtain one at http://mozilla.org/MPL/2.0/.            //
// =================================================================== //

#include "DisplayDockWidget.h"

DisplayDockWidget::DisplayDockWidget(QWidget* parent)
                                     : QDockWidget(parent),
                                       graphScene_(0),
                                       data_(0),
                                       ui_(new Ui::DisplayDockWidgetBase)
{
    ui_->setupUi(this);
}

DisplayDockWidget::~DisplayDockWidget()
{
    delete ui_;
}

void DisplayDockWidget::setGraphScene(GraphScene* scene)
{
    graphScene_ = scene;
    ui_->logPlotGroupBox->setChecked(graphScene_->isLogPlot());
}

float DisplayDockWidget::getGamma()
{
    if (graphScene_->isLogPlotAcceptable() &&
        graphScene_->isLogPlot()) {
        return ui_->logPlotBaseSlider->value();
    }
    else {
        return 1.0f;
    }
}

void DisplayDockWidget::updateScene()
{
    if (!graphScene_) return;

    graphScene_->createAxisAndScale();
    graphScene_->showScaleInPlaneOfIncidence(ui_->planeOfIncidenceCheckBox->isChecked());
    graphScene_->updateInOutDirLine();

    emit redrawGraphRequested();
}

void DisplayDockWidget::updateUi()
{
    if (!data_) return;

    if (data_->getSpecularReflectances() || data_->getSpecularTransmittances()) {
        ui_->logPlotGroupBox->setDisabled(true);
    }
    else {
        ui_->logPlotGroupBox->setEnabled(true);
    }
}

void DisplayDockWidget::useLogPlot(bool on)
{
    graphScene_->useLogPlot(on);
    updateBaseOfLogarithm(ui_->logPlotBaseSlider->value());
}

void DisplayDockWidget::updateBaseOfLogarithm(int index)
{
    graphScene_->setBaseOfLogarithm(index);

    graphScene_->updateGraphGeometry();
    graphScene_->createAxisAndScale();
    graphScene_->showScaleInPlaneOfIncidence(ui_->planeOfIncidenceCheckBox->isChecked());
    graphScene_->updateInOutDirLine();

    emit redrawGraphRequested();
    emit redrawTableRequested();

    ui_->logPlotBaseLineEdit->setText(QString::number(index));
}

void DisplayDockWidget::toggleLogPlotCheckBox(bool on)
{
    ui_->logPlotGroupBox->setChecked(on);
}

void DisplayDockWidget::setScaleLength1(double length)
{
    graphScene_->setScaleLength1(length);
    updateScene();
}

void DisplayDockWidget::setScaleLength2(double length)
{
    graphScene_->setScaleLength2(length);
    updateScene();
}

void DisplayDockWidget::showScaleInPlaneOfIncidence(bool on)
{
    graphScene_->showScaleInPlaneOfIncidence(on);
    emit redrawGraphRequested();
}
