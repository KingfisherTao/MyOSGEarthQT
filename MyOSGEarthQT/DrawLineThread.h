#pragma once

#include <osgEarth/ThreadingUtils>
#include <osgEarthFeatures/Feature>
#include <osgEarthAnnotation/FeatureNode>

// 这是小陈起的名字 
class DrawLineThread : public OpenThreads::Thread
{
public:
	DrawLineThread(osg::Vec3d start, double radius, double numSpokes, const osgEarth::SpatialReference* mapSRS);
	~DrawLineThread();
	inline osg::Group* get() { return m_group.get(); }
	void cloneGeometry(osgEarth::Geometry* geometrySide);
	inline void setStart(osg::Vec3d start) { m_start = start; }
	void creatNode();
	virtual void run();
	void clear();
private:

	osg::ref_ptr<osgEarth::Geometry>					m_GeometrySide;
	const osgEarth::SpatialReference*					m_spatRef;
	osg::ref_ptr<osg::Group>							m_group;

	osg::Vec3d	m_start;
	osg::Vec4	m_goodColor;
	osg::Vec4	m_badColor;

	double		m_numSpokes;
	double		m_radius;
};

