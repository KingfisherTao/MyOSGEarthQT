#include "DrawLineThread.h"
#include <osgEarth/GLUtils>

DrawLineThread::DrawLineThread(osg::Vec3d start, double radius, double	numSpokes, const osgEarth::SpatialReference* mapSRS):
	m_start(start),
	m_radius(radius),
	m_numSpokes(numSpokes), 
	m_spatRef(mapSRS),
	m_goodColor(0.0f, 1.0f, 0.0f, 1.0f),
	m_badColor(1.0f, 0.0f, 0.0f, 1.0f)
{
	m_group = new osg::Group();
	creatNode();
}

void DrawLineThread::creatNode()
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
		_featureNode->setName("fNode_"+ std::to_string(i));
		m_group->addChild(_featureNode.get());
	}
}

DrawLineThread::~DrawLineThread()
{

}

void DrawLineThread::cloneGeometry(osgEarth::Geometry* geometrySide)
{ 
	m_GeometrySide = geometrySide->clone(); 
}

void DrawLineThread::run()
{
	osgEarth::GeoPoint geoPoint;
	osg::Vec3d up, tempOpint;
	unsigned int num = m_group->getNumChildren();
	osgEarth::Annotation::FeatureNode* _fNode;

	//for (osgEarth::Geometry::iterator ite = m_GeometrySide->begin(); ite != m_GeometrySide->end(); ++ite)
	//{
	//	up = (*ite - m_start) / num;
	//	for (unsigned int i = 0; i < num; i++)
	//	{
	//		tempOpint = up * i + m_start;
	//		_fNode = dynamic_cast<osgEarth::Annotation::FeatureNode*>(m_group->getChild(i));
	//		_fNode->getFeature()->getGeometry()->push_back(tempOpint);
	//	}
	//	_fNode->init();
	//}

	double earthRadius = m_spatRef->getEllipsoid()->getRadiusEquator();
	double lat = osg::DegreesToRadians(m_start.y());
	double lon = osg::DegreesToRadians(m_start.x());
	double _tempDis = m_radius / 100.0;

	double _delta = osg::PI * 2.0 / num;
	for (unsigned int i = 0; i < num; i++)
	{
		double angle = _delta * (double)i;
		double clat, clon;

		_fNode = dynamic_cast<osgEarth::Annotation::FeatureNode*>(m_group->getChild(i));
		for (unsigned int j = 0; j <= (unsigned int)100.0; j++)
		{
			osgEarth::GeoMath::destination(lat, lon, angle, _tempDis * j, clat, clon, earthRadius);
			_fNode->getFeature()->getGeometry()->push_back(osg::Vec3d(osg::RadiansToDegrees(clon), osg::RadiansToDegrees(clat), m_start.z()));
		}
		_fNode->init();
		OpenThreads::Thread::microSleep(24000);
	}
	cancel();
}

void DrawLineThread::clear()
{
	osgEarth::Annotation::FeatureNode* _fNode;
	for (unsigned int i = 0; i < m_group->getNumChildren(); i++)
	{
		_fNode = dynamic_cast<osgEarth::Annotation::FeatureNode*>(m_group->getChild(i));
		_fNode->getFeature()->getGeometry()->clear();
	}

	m_group->removeChild(0, m_group->getNumChildren());
}