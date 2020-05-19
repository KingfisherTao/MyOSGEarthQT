#pragma once
#include "DrawLineThread.h"


class DrawCircleThread : public OpenThreads::Thread
{
public:
	DrawCircleThread(osg::Vec3d start, double radius, double numSpokes, osg::Group* losGroup, const osgEarth::SpatialReference* mapSRS);
	~DrawCircleThread();
	inline osg::Group* get() { return m_group.get(); }
	inline void setStart(osg::Vec3d start) { m_start = start; }
	inline void setLT(DrawLineThread* plt) { m_pLT = plt; }
	void creatNode();
	virtual void run();
	void clear();

private:
	osg::ref_ptr<osg::Group>			m_group;
	osg::ref_ptr<osg::Group>			m_losGroup;

	const osgEarth::SpatialReference*	m_spatRef;
	DrawLineThread*						m_pLT;

	osg::Vec3d	m_start;
	osg::Vec4	m_goodColor;
	osg::Vec4	m_badColor;

	double		m_numSpokes;
	double		m_radius;
};