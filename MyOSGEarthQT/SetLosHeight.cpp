#include "SetLosHeight.h"
#include "MyOSGEarthQT.h"

#include <QDoubleValidator>

SetLosHeight::SetLosHeight(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	ui.lineEdit->setValidator(new QDoubleValidator(0.0,8848,6,this));
	ui.lineEdit->setText("2.0");
	connect(ui.buttonOK, &QPushButton::clicked, this, &SetLosHeight::on_btnOK);
	connect(ui.buttonCancel, &QPushButton::clicked, this, &SetLosHeight::on_btnCancel);
}

void SetLosHeight::on_btnOK()
{
	close();
	MyOSGEarthQT* parent = (MyOSGEarthQT*)parentWidget();
	parent->sendLosHeight(ui.lineEdit->text().toFloat());
}

void SetLosHeight::on_btnCancel()
{
	close();
}