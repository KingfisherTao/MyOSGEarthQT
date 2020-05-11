#pragma once
#include <QtWidgets/QMainWindow>

#include "ui_MyOSGEarthQT.h"
#include "SetLosHeight.h"

#include <osgEarth/Map>
#include <osgEarth/MapNode>
#include <osgEarthUtil/Sky>
#include <osgEarthUtil/AutoClipPlaneHandler>

#include <osgEarthUtil/MouseCoordsTool>
#include <osgEarthUtil/MeasureTool>

#include "PickEvent.h"

class MyOSGEarthQT : public QMainWindow
{
	Q_OBJECT

public:
	MyOSGEarthQT(QWidget *parent = Q_NULLPTR);
	~MyOSGEarthQT() {}
	void sendLosHeight(float height);

private slots:
	void on_VisibilityAnalysis(bool checked);
	void on_ViewshedAnalysis(bool checked);
	void on_RadarAnalysis(bool checked);
	void on_ClearAnalysis();
	void on_SetLosHeight();

private:
	Ui::MyOSGEarthQTClass ui;
	SetLosHeight* sub_setLosHeight;

	osg::ref_ptr<osgEarth::MapNode> m_mapNode;
	osg::ref_ptr<osg::Group> m_world;
	osg::ref_ptr<osg::Group> m_losGroup;

	PickEvent* m_PickEvent;
};
