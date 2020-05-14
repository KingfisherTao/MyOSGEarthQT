#pragma once

#include <osgEarthFeatures/Feature>
#include <osgEarthAnnotation/FeatureNode>

// 鼠标事件中，点击地面后产生的测试区域选框 包括一条距离辅助线 以及 圆形选框
class CReferenceArea
{

public:
	CReferenceArea(osg::ref_ptr<osgEarth::MapNode> MapNode);
	virtual ~CReferenceArea() {}

	inline osg::Group* get() { return m_CircleNode.get(); }
	inline void setNumSpokes(double numSpokes) { m_numSpokes = numSpokes; }
	void setStart(osg::Vec3d point);
	void setRadius(double radius);
	void init();
	void clear();

private:

	osg::ref_ptr<osgEarth::Annotation::FeatureNode>		m_CircleNode;
	osg::ref_ptr<osgEarth::Features::Feature>			m_CircleFeature;

	osg::ref_ptr<osgEarth::MapNode>						m_mapNode;
	const osgEarth::SpatialReference*					m_spatRef;

	osg::Vec3d	m_start;
	osg::Vec3d	m_end;
	double		m_radius;
	double		m_numSpokes;
};

