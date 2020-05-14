#pragma once

#include <osgEarthFeatures/Feature>
#include <osgEarthAnnotation/FeatureNode>

// ����¼��У�������������Ĳ�������ѡ�� ����һ�����븨���� �Լ� Բ��ѡ��
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

