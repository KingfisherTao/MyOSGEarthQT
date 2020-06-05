#include "CReferenceArea.h"

#include <osg/Depth>
#include <osgEarth/GLUtils>

CReferenceArea::CReferenceArea(osgEarth::MapNode* mapNode):
	m_mapNode(mapNode),
	m_spatRef(mapNode->getMapSRS()),
	m_numSpokes(0.0),
	m_start_lat(0.0),
	m_start_lon(0.0),
	m_start_delta(0.0),
	m_radius(0.0)
{
	m_CircleFeature = new osgEarth::Features::Feature(new osgEarth::Symbology::LineString(), m_spatRef);
	m_CircleFeature->geoInterp() = osgEarth::Features::GEOINTERP_GREAT_CIRCLE;

	osgEarth::Symbology::AltitudeSymbol* alt_Circle = m_CircleFeature->style()->getOrCreate<osgEarth::Symbology::AltitudeSymbol>();
	alt_Circle->clamping() = alt_Circle->CLAMP_TO_TERRAIN;
	alt_Circle->technique() = alt_Circle->TECHNIQUE_DRAPE;

	osgEarth::Symbology::RenderSymbol* render_Circle = m_CircleFeature->style()->getOrCreate<osgEarth::Symbology::RenderSymbol>();
	render_Circle->depthOffset()->enabled() = true;
	render_Circle->depthOffset()->automatic() = true;

	osgEarth::Symbology::LineSymbol* ls_Circle = m_CircleFeature->style()->getOrCreate<osgEarth::Symbology::LineSymbol>();
	ls_Circle->stroke()->color() = osgEarth::Color(osgEarth::Color::Yellow, 1.0f);
	ls_Circle->stroke()->width() = 1.0f;

	m_CircleNode = new osgEarth::Annotation::FeatureNode(m_CircleFeature.get());
	osgEarth::GLUtils::setLighting(m_CircleNode->getOrCreateStateSet(), osg::StateAttribute::OFF);
}

void CReferenceArea::init()
{
	if (m_numSpokes == 0.0)
	{
		// automatically calculate
		const osgEarth::Distance radius = m_radius;
		double segLen = radius.as(osgEarth::Units::METERS) / 8.0;
		double circumference = 2 * osg::PI* radius.as(osgEarth::Units::METERS);
		m_numSpokes = (unsigned)::ceil(circumference / segLen);
		m_start_delta = osg::PI * 2.0 / m_numSpokes;
	}

	double _angle = 0.0;
	double _clat = 0.0, _clon = 0.0;

	clear();
	for (int i = (unsigned int)m_numSpokes - 1; i >= 0; --i)
	{
		_angle = m_start_delta * (double)i;
		osgEarth::GeoMath::destination(m_start_lat, m_start_lon, _angle, m_radius, _clat, _clon, m_spatRef->getEllipsoid()->getRadiusEquator());
		m_CircleFeature->getGeometry()->push_back(osg::Vec3d(osg::RadiansToDegrees(_clon), osg::RadiansToDegrees(_clat), 0.0));
	}
	m_CircleFeature->getGeometry()->push_back(m_CircleFeature->getGeometry()->front());
	m_CircleNode->dirty();
}
void CReferenceArea::setStart(osg::Vec3d start)
{
	m_start = start;
	m_start_lat = osg::DegreesToRadians(m_start.y());
	m_start_lon = osg::DegreesToRadians(m_start.x());
	clear();
}

void CReferenceArea::setRadius(double radius)
{
	m_radius = radius;
	init();
}

void CReferenceArea::clear()
{
	m_CircleFeature->getGeometry()->clear();
	m_CircleNode->dirty();
}