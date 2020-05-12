#pragma once
#include <osgGA/GUIEventHandler>
#include <osgEarth/MapNode>
#include <osgViewer/Viewer>

#include <osgEarthUtil/LinearLineOfSight>
#include <osgEarthUtil/RadialLineOfSight>
#include <osgEarthAnnotation/CircleNode>
#include <osgEarthUtil/TerrainProfile>

#include <osgEarthFeatures/Feature>
#include <osgEarthAnnotation/FeatureNode>

#include <QLabel>

enum EnumActionEvent
{
	ActionNull = 0,
	VisibilityAnalysis,
	ViewshedAnalysis,
	RadarAnalysis
};

class PickEvent : public osgGA::GUIEventHandler
{
public:
	PickEvent(QLabel* label, osg::ref_ptr<osgEarth::MapNode> MapNode, osg::Group* losGroup);
	~PickEvent() {}
	virtual bool handle(const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &aa) override;
	void setActionEvent(const EnumActionEvent &ae);
	void setLosHeight(float height);
	void RemoveAnalysis();

protected:
	virtual void pickLeft(osg::Vec3d Point);
	virtual void pickMove(osg::Vec3d Point);

	osg::Vec3d Screen2Geo(float x, float y);
private:
	osg::ref_ptr<osgEarth::MapNode>				m_mapNode;
	osg::ref_ptr<osgViewer::Viewer>				m_viewer;
	osg::ref_ptr<osg::Group>					m_losGroup;
	const osgEarth::SpatialReference*			m_spatRef;

	osg::ref_ptr<osgEarth::Util::LinearLineOfSightNode>			m_curLosNode;
	osg::ref_ptr<osgEarth::Util::RadialLineOfSightNode>			m_curRosNode;
	osg::ref_ptr<osgEarth::Annotation::CircleNode>				m_curCircleNode;
	osgEarth::Symbology::Style									m_circleStyle;
	osg::ref_ptr<osgEarth::Util::TerrainProfileCalculator>		m_Calculator;

	osgEarth::Symbology::Style									m_pathStyle;
	osg::ref_ptr<osgEarth::Annotation::FeatureNode>				m_featureNode;
	//osg::ref_ptr<osgEarth::Symbology::Geometry>					m_featureGeometry;
	osg::ref_ptr<osgEarth::Features::Feature>					m_feature;

	osg::Vec3d FirstPoint;
	osg::Vec3d LastPoint;

	bool m_bFirstClick;
	float m_losHeight;
	float m_last_mouseX , m_last_mouseY;

	QLabel* m_Label;
	QString m_mapName;
	QString m_csysTitle;
	EnumActionEvent m_ActionEvent;
};