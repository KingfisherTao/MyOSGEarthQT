#include "DrawLineCallback.h"
#include <osgEarth/GLUtils>
#include <qDebug>

DrawLineCallback::DrawLineCallback(osg::Vec3d start, double angle, double radius, double numSpokes, float losHeight, osgEarth::MapNode* mapNode):
	m_angle(angle),
	m_radius(radius),
	m_numSpokes(numSpokes), 
	m_losHeight(losHeight),
	m_mapNode(mapNode),
	m_spatRef(mapNode->getMapSRS()),
	m_numSegment(150),
	m_goodColor(0.0f, 1.0f, 0.0f, 1.0f),
	m_badColor(1.0f, 0.0f, 0.0f, 1.0f)
{
	// 基于分段数初始小段距离
	m_tempDis = m_radius / (double)m_numSegment;
	m_NodeCount = 0;

	m_pLs = new osgEarth::Symbology::LineString[m_numSpokes];
	m_lineStyle.getOrCreate<osgEarth::Symbology::LineSymbol>()->stroke()->width() = 2.0;
	m_lineStyle.getOrCreate<osgEarth::Symbology::AltitudeSymbol>()->clamping() = osgEarth::Symbology::AltitudeSymbol::CLAMP_TO_TERRAIN;
	m_lineStyle.getOrCreate<osgEarth::Symbology::AltitudeSymbol>()->technique() = osgEarth::Symbology::AltitudeSymbol::TECHNIQUE_DRAPE;
	m_lineStyle.getOrCreate<osgEarth::Symbology::RenderSymbol>()->depthOffset()->automatic() = true;

	// 构造视线
	m_LosNode = new osgEarth::Util::LinearLineOfSightNode(m_mapNode);
	m_LosNode->setTerrainOnly(true);
	setStart(start);

	// 初始化视线 bool 二维数组
	m_bLosArry = nullptr;
	m_bLosArry = new bool*[m_numSpokes];
	for (int i = 0; i < m_numSpokes; i++)
	{
		m_bLosArry[i] = new bool[m_numSegment];
	}

	// 计算每次旋转的弧度
	m_delta = osg::PI * 2.0 / m_numSpokes;

	// FeatureNode 添加的总group
	m_group = new osg::Group();
	osgEarth::GLUtils::setLighting(m_group->getOrCreateStateSet(), osg::StateAttribute::OFF);
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

	if (m_pLs)
	{
		m_pLs.release();
		m_pLs = nullptr;
	}

	if (m_bLosArry)
	{
		delete m_bLosArry;
		m_bLosArry = nullptr;
	}
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

	double _clat = 0.0, _clon = 0.0;
	double _angle = m_delta * (double)m_NodeCount + m_angle;
	for (unsigned int i = 0; i < m_numSegment; i++)
	{
		osgEarth::GeoMath::destination(m_lat, m_lon, _angle, m_tempDis * i, _clat, _clon);
		osgEarth::GeoPoint _end(m_spatRef->getGeographicSRS(), osg::RadiansToDegrees(_clon), osg::RadiansToDegrees(_clat), 1.0, osgEarth::AltitudeMode::ALTMODE_RELATIVE);

		m_pLs[m_NodeCount].push_back(_end.vec3d());

		m_LosNode->setEnd(_end);
		m_bLosArry[m_NodeCount][i] = m_LosNode->getHasLOS();
	}

	bool _curLos = m_bLosArry[m_NodeCount][0];
	osg::Vec3d _lastPoint(m_start.x(), m_start.y(), m_start.z());

	for (unsigned int i = 0; i < m_numSegment; i++)
	{
		if (_curLos != m_bLosArry[m_NodeCount][i])
		{
			osg::ref_ptr<osgEarth::Symbology::LineString> _ls = new osgEarth::Symbology::LineString();
			osg::ref_ptr<osgEarth::Features::Feature> _feature = new osgEarth::Features::Feature(_ls, m_spatRef);
			osg::ref_ptr<osgEarth::Annotation::FeatureNode> _featureNode = new osgEarth::Annotation::FeatureNode(_feature.get());
			m_lineStyle.getOrCreate<osgEarth::Symbology::LineSymbol>()->stroke()->color() = _curLos ? osgEarth::Symbology::Color::Green : osgEarth::Symbology::Color::Red;
			_featureNode->setStyle(m_lineStyle);
			_feature->getGeometry()->push_back(_lastPoint);
			_feature->getGeometry()->push_back(m_pLs[m_NodeCount].at(i)); 
			m_group->addChild(_featureNode.get());

			_lastPoint = m_pLs[m_NodeCount].at(i);
			_curLos = m_bLosArry[m_NodeCount][i];
		}
	}

	osg::ref_ptr<osgEarth::Symbology::LineString> _ls = new osgEarth::Symbology::LineString();
	osg::ref_ptr<osgEarth::Features::Feature> _feature = new osgEarth::Features::Feature(_ls, m_spatRef);
	osg::ref_ptr<osgEarth::Annotation::FeatureNode> _featureNode = new osgEarth::Annotation::FeatureNode(_feature.get());
	m_lineStyle.getOrCreate<osgEarth::Symbology::LineSymbol>()->stroke()->color() = _curLos ? osgEarth::Symbology::Color::Green : osgEarth::Symbology::Color::Red;
	_featureNode->setStyle(m_lineStyle);
	_feature->getGeometry()->push_back(_lastPoint);
	_feature->getGeometry()->push_back(m_pLs[m_NodeCount].at(m_numSegment - 1));
	m_group->addChild(_featureNode.get());

	m_NodeCount++;
	if (m_NodeCount == m_numSpokes)
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