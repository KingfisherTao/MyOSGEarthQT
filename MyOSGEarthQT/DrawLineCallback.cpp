#include "DrawLineCallback.h"
#include <osgEarth/GLUtils>

DrawLineCallback::DrawLineCallback(osg::Vec3d start, double angle, double radius, double numSpokes, float losHeight, osgEarth::MapNode* mapNode):
	m_angle(angle),
	m_radius(radius),
	m_numSpokes(numSpokes), 
	m_losHeight(losHeight),
	m_mapNode(mapNode),
	m_spatRef(mapNode->getMapSRS()),
	m_numSegment(100),
	m_goodColor(0.0f, 1.0f, 0.0f, 1.0f),
	m_badColor(1.0f, 0.0f, 0.0f, 1.0f)
{
	m_tempDis = m_radius / (double)m_numSegment;
	m_NodeCount = 0;

	m_group = new osg::Group();
	m_LosNode = new osgEarth::Util::LinearLineOfSightNode(m_mapNode);
	setStart(start);

	creatNode();

	m_group->setUpdateCallback(this);
}

DrawLineCallback::~DrawLineCallback()
{
	if (m_group)
	{
		m_group.release();
		m_group = nullptr;
	}

	if (m_LosNode)
	{
		m_LosNode.release();
		m_LosNode = nullptr;
	}
}
void DrawLineCallback::creatNode()
{
	for (int i = 0; i < (int)m_numSpokes; i++)
	{
		osg::ref_ptr<osgEarth::Features::Feature> _feature = new osgEarth::Features::Feature(new osgEarth::Symbology::LineString(), m_spatRef);
		_feature->geoInterp() = osgEarth::Features::GEOINTERP_GREAT_CIRCLE;

		osgEarth::Symbology::AltitudeSymbol* alt = _feature->style()->getOrCreate<osgEarth::Symbology::AltitudeSymbol>();
		alt->clamping() = alt->CLAMP_TO_TERRAIN;
		alt->technique() = alt->TECHNIQUE_GPU;

		osgEarth::Symbology::RenderSymbol* render = _feature->style()->getOrCreate<osgEarth::Symbology::RenderSymbol>();
		render->depthOffset()->enabled() = true;
		render->depthOffset()->automatic() = true;

		osgEarth::Symbology::LineSymbol* ls = _feature->style()->getOrCreate<osgEarth::Symbology::LineSymbol>();
		ls->stroke()->color() = osgEarth::Color(osgEarth::Color::Red, 0.2f);
		ls->stroke()->width() = 2.0f;
		ls->tessellation() = 150;

		osg::ref_ptr<osgEarth::Annotation::FeatureNode> _featureNode = new osgEarth::Annotation::FeatureNode(_feature.get());
		osgEarth::GLUtils::setLighting(_featureNode->getOrCreateStateSet(), osg::StateAttribute::OFF);
		m_group->addChild(_featureNode.get());
	}

	m_delta = osg::PI * 2.0 / m_group->getNumChildren();
}

void DrawLineCallback::setStart(osg::Vec3d start)
{
	m_start = start; 
	m_lat = osg::DegreesToRadians(m_start.y()); 
	m_lon = osg::DegreesToRadians(m_start.x());

	auto _start = osgEarth::GeoPoint(m_spatRef->getGeographicSRS(), m_start.x(), m_start.y(), m_losHeight, osgEarth::AltitudeMode::ALTMODE_RELATIVE);
	m_LosNode->setStart(_start);
}

bool DrawLineCallback::run(osg::Object* object, osg::Object* data)
{

	osg::Group* _group = dynamic_cast<osg::Group*>(object);
	osgEarth::Annotation::FeatureNode* _fNode = dynamic_cast<osgEarth::Annotation::FeatureNode*>(_group->getChild(m_NodeCount));

	double _clat = 0.0, _clon = 0.0;
	double _angle = m_delta * (double)m_NodeCount + m_angle;

	for (unsigned int i = 0; i <= m_numSegment; i++)
	{
		osgEarth::GeoMath::destination(m_lat, m_lon, _angle, m_tempDis * i, _clat, _clon);
		osgEarth::GeoPoint _end(m_spatRef->getGeographicSRS(), osg::RadiansToDegrees(_clon), osg::RadiansToDegrees(_clat), 0.0, osgEarth::AltitudeMode::ALTMODE_RELATIVE);
		// ��������
		m_LosNode->setEnd(_end);
		_fNode->getFeature()->getGeometry()->push_back(_end.vec3d());	
		// ���ϰ�
		if (m_LosNode->getHasLOS())
		{

		}
		// ���ϰ�
		else
		{

		}
	}
	_fNode->dirty();
	m_NodeCount++;
	if (m_NodeCount >= _group->getNumChildren())
	{
		_group->removeUpdateCallback(this);
		return false;
	}
	return traverse(object, data);
}

void DrawLineCallback::clear()
{
	osgEarth::Annotation::FeatureNode* _fNode;
	for (unsigned int i = 0; i < m_group->getNumChildren(); i++)
	{
		_fNode = dynamic_cast<osgEarth::Annotation::FeatureNode*>(m_group->getChild(i));
		_fNode->getFeature()->getGeometry()->clear();
		_fNode->dirty();
	}

	m_group->removeChild(0, m_group->getNumChildren());
}