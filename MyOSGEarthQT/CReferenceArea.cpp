#include "CReferenceArea.h"

#include <osg/Depth>
#include <osgEarth/GLUtils>

CReferenceArea::CReferenceArea(osg::ref_ptr<osgEarth::MapNode> mapNode):
	m_mapNode(mapNode),
	m_spatRef(mapNode->getMapSRS()),
	m_numSpokes(0.0),
	m_radius(0.0)
{
	// ³õÊ¼»¯ ·¶Î§È¦ feature
	m_CircleFeature = new osgEarth::Features::Feature(new osgEarth::Symbology::LineString(), m_spatRef);
	m_CircleFeature->geoInterp() = osgEarth::Features::GEOINTERP_GREAT_CIRCLE;

	osgEarth::Symbology::AltitudeSymbol* alt_Circle = m_CircleFeature->style()->getOrCreate<osgEarth::Symbology::AltitudeSymbol>();
	alt_Circle->clamping() = alt_Circle->CLAMP_TO_TERRAIN;
	alt_Circle->technique() = alt_Circle->TECHNIQUE_GPU;

	osgEarth::Symbology::RenderSymbol* render_Circle = m_CircleFeature->style()->getOrCreate<osgEarth::Symbology::RenderSymbol>();
	render_Circle->depthOffset()->enabled() = true;
	render_Circle->depthOffset()->automatic() = true;

	osgEarth::Symbology::LineSymbol* ls_Circle = m_CircleFeature->style()->getOrCreate<osgEarth::Symbology::LineSymbol>();
	ls_Circle->stroke()->color() = osgEarth::Color(osgEarth::Color::Yellow, 0.2f);
	ls_Circle->stroke()->width() = 2.0f;
	ls_Circle->tessellation() = 150;

	m_CircleNode = new osgEarth::Annotation::FeatureNode(m_CircleFeature.get());
	osgEarth::GLUtils::setLighting(m_CircleNode->getOrCreateStateSet(), osg::StateAttribute::OFF);

}

void CReferenceArea::init()
{
	const osgEarth::Distance radius = m_radius;
	if (m_numSpokes == 0)
	{
		// automatically calculate
		double segLen = radius.as(osgEarth::Units::METERS) / 8.0;
		double circumference = 2 * osg::PI* radius.as(osgEarth::Units::METERS);
		m_numSpokes = (unsigned)::ceil(circumference / segLen);
	}

	// add 360 Ring
	m_CircleFeature->getGeometry()->clear();
	double _delta = osg::PI * 2.0 / m_numSpokes;
	double earthRadius = m_spatRef->getEllipsoid()->getRadiusEquator();
	double lat = osg::DegreesToRadians(m_start.y());
	double lon = osg::DegreesToRadians(m_start.x());
	for (int i = (unsigned int)m_numSpokes - 1; i >= 0; --i)
	{
		double angle = _delta * (double)i;
		double clat, clon;
		osgEarth::GeoMath::destination(lat, lon, angle, m_radius, clat, clon, earthRadius);
		m_CircleFeature->getGeometry()->push_back(osg::Vec3d(osg::RadiansToDegrees(clon), osg::RadiansToDegrees(clat), m_start.z()));
	}
	m_CircleFeature->getGeometry()->push_back(m_CircleFeature->getGeometry()->front());
	m_CircleNode->init();
}
void CReferenceArea::setStart(osg::Vec3d start)
{
	m_start = start;
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