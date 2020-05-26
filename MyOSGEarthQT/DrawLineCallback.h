#pragma once
#include <osgEarthFeatures/Feature>
#include <osgEarthAnnotation/FeatureNode>
#include <osgEarthUtil/LinearLineOfSight>

// 这是小陈起的名字 DrawLineThread 后面 由于重写成了回调形式，更名为 DrawLineCallback
class DrawLineCallback : public osg::Callback
{
public:
	DrawLineCallback(osg::Vec3d start, double angle, double radius, double numSpokes, float losHeight, osgEarth::MapNode* mapNode);
	virtual ~DrawLineCallback();
	inline osg::Group* get() { return m_group; }
	inline void setStart(osg::Vec3d start);
	void creatNode();
	virtual bool run(osg::Object* object, osg::Object* data);
	void clear();
private:
	osg::ref_ptr<osgEarth::MapNode>							m_mapNode;
	const osgEarth::SpatialReference*						m_spatRef;
	osg::ref_ptr<osg::Group>								m_group;
	osg::ref_ptr<osgEarth::Util::LinearLineOfSightNode>		m_LosNode;

	osg::Vec3d	m_start;
	osg::Vec4	m_goodColor;
	osg::Vec4	m_badColor;

	int				m_NodeCount;
	double			m_delta;
	double			m_lon;
	double			m_lat;
	double			m_angle;
	float			m_losHeight;
	double			m_numSpokes;
	double			m_radius;
	double			m_tempDis;
	unsigned int	m_numSegment;
};
