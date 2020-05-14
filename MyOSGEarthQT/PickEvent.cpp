#include "PickEvent.h"
#include <QDebug>
#include <osgEarth/GeoMath>
#include <osgEarth/GLUtils>

PickEvent::PickEvent(QLabel* label, osg::ref_ptr<osgEarth::MapNode> mapNode, osg::Group* losGroup) :
	m_ActionEvent(EnumActionEvent::ActionNull),
	m_Label(label),
	m_mapNode(mapNode),
	m_losGroup(losGroup),
	m_bFirstClick(false),
	m_bLastPoint(false),
	m_losHeight(2.0),
	m_mapName(QString::fromUtf8(mapNode->getMapSRS()->getName().c_str())),
	m_csysTitle(QString::fromLocal8Bit("坐标:"))
{
	m_spatRef = m_mapNode->getMapSRS();
	m_Calculator = new osgEarth::Util::TerrainProfileCalculator(m_mapNode);
	// m_curRosNode = NULL;

	// 贴地的 圆
	m_circleStyle.getOrCreate<osgEarth::Symbology::PolygonSymbol>()->fill()->color() = osgEarth::Color(osgEarth::Color::Blue, 0.4f);
	m_circleStyle.getOrCreate<osgEarth::Symbology::AltitudeSymbol>()->clamping() = osgEarth::Symbology::AltitudeSymbol::CLAMP_TO_TERRAIN;
	m_circleStyle.getOrCreate<osgEarth::Symbology::AltitudeSymbol>()->technique() = osgEarth::Symbology::AltitudeSymbol::TECHNIQUE_DRAPE;
	m_circleStyle.getOrCreate<osgEarth::Symbology::AltitudeSymbol>()->verticalOffset() = 0.1;
	m_circleStyle.getOrCreate<osgEarth::Symbology::RenderSymbol>()->order();
}

bool PickEvent::handle(const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &aa)
{
	m_viewer = dynamic_cast<osgViewer::Viewer*>(&aa);
	if (!m_viewer)
	{
		return false;
	}
	
	// 鼠标点击事件
	if (ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
	{
		if (ea.getEventType() == osgGA::GUIEventAdapter::PUSH)
		{
			m_last_mouseX = ea.getX();
			m_last_mouseY = ea.getY();
		}
		else if (ea.getEventType() == osgGA::GUIEventAdapter::RELEASE)
		{
			if (m_last_mouseX == ea.getX() && m_last_mouseY == ea.getY())
			{
				FirstPoint = Screen2Geo(ea.getX(), ea.getY());
				pickLeft(FirstPoint);
				LastPoint = FirstPoint;
			}
		}
	}
	
	// 鼠标滑动事件
	if (ea.getEventType() == osgGA::GUIEventAdapter::MOVE)
	{
		auto MovePoint = Screen2Geo(ea.getX(), ea.getY());
		pickMove(MovePoint);
	}

	return false;
}

void PickEvent::pickLeft(osg::Vec3d Point)
{
	switch (m_ActionEvent)
	{
		// 通视分析
		case EnumActionEvent::VisibilityAnalysis:
		{
			if (m_bFirstClick)
			{
				auto _start = osgEarth::GeoPoint(m_spatRef->getGeographicSRS(), Point.x(), Point.y(), m_losHeight, osgEarth::AltitudeMode::ALTMODE_RELATIVE);
				m_curLosNode = new osgEarth::Util::LinearLineOfSightNode(m_mapNode);
				m_curLosNode->setDisplayMode(osgEarth::Util::LineOfSight::DisplayMode::MODE_SPLIT);
				m_curLosNode->setStart(_start);
				m_losGroup->addChild(m_curLosNode);

				m_bFirstClick = false;
			}
			else
			{
				// 地形剖面 dis 两次鼠标点击距离， elevNum 变化的高程count
				auto _start = osgEarth::GeoPoint(m_spatRef->getGeographicSRS(), LastPoint, osgEarth::AltitudeMode::ALTMODE_ABSOLUTE);
				auto _end = osgEarth::GeoPoint(m_spatRef->getGeographicSRS(), Point, osgEarth::AltitudeMode::ALTMODE_ABSOLUTE);
				auto _profile = m_Calculator->getProfile();
				m_Calculator->computeTerrainProfile(m_mapNode, _start, _end, _profile);
				auto dis = _profile.getTotalDistance();
				auto elevNum = _profile.getNumElevations();

				
				m_bFirstClick = true;
			}
		}break;
		// 视域分析
		case EnumActionEvent::ViewshedAnalysis:
		{
			if (m_bFirstClick)
			{
				m_curCircleNode = new osgEarth::Annotation::CircleNode;
				m_curCircleNode->setStyle(m_circleStyle);
				m_curCircleNode->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
				auto centerPoint = osgEarth::GeoPoint(m_spatRef->getGeographicSRS(), Point.x(), Point.y(), 0, osgEarth::AltitudeMode::ALTMODE_RELATIVE);
				m_curCircleNode->setPosition(centerPoint);
				m_losGroup->addChild(m_curCircleNode.get());

				//m_curRosNode = new osgEarth::Util::RadialLineOfSightNode(m_mapNode);
				//m_curRosNode->setCenter(centerPoint);
				//m_curRosNode->setRadius(0);
				//m_curRosNode->setNumSpokes(100);
				//m_curRosNode->setDisplayMode(osgEarth::Util::LineOfSight::DisplayMode::MODE_SINGLE);
				//m_losGroup->addChild(m_curRosNode);

				// feature:
				m_feature = new osgEarth::Features::Feature(new osgEarth::Symbology::LineString(), m_spatRef);
				m_feature->geoInterp() = osgEarth::Features::GEOINTERP_GREAT_CIRCLE;

				osgEarth::Symbology::AltitudeSymbol* alt = m_feature->style()->getOrCreate<osgEarth::Symbology::AltitudeSymbol>();
				alt->clamping() = alt->CLAMP_TO_TERRAIN;
				alt->technique() = alt->TECHNIQUE_GPU;

				osgEarth::Symbology::RenderSymbol* render = m_feature->style()->getOrCreate<osgEarth::Symbology::RenderSymbol>();
				render->depthOffset()->enabled() = true;
				render->depthOffset()->automatic() = true;

				osgEarth::Symbology::LineSymbol* ls = m_feature->style()->getOrCreate<osgEarth::Symbology::LineSymbol>();
				ls->stroke()->color() = osgEarth::Symbology::Color::Red;
				ls->stroke()->width() = 2.0f;
				ls->tessellation() = 150;
				//ls->stroke()->stipple() = 255;

				m_featureNode = new osgEarth::Annotation::FeatureNode(m_feature.get());
				osgEarth::GLUtils::setLighting(m_featureNode->getOrCreateStateSet(), osg::StateAttribute::OFF);

				m_losGroup->addChild(m_featureNode.get());

				m_feature->getGeometry()->push_back(osg::Vec3d(Point.x(), Point.y(), 10));
				
				m_bFirstClick = false;
			}
			else
			{
				if (m_bLastPoint)
				{
					m_feature->getGeometry()->back() = osg::Vec3d(Point.x(), Point.y(), 10);
					m_bLastPoint = false;
				}
				else
				{
					m_feature->getGeometry()->push_back(osg::Vec3d(Point.x(), Point.y(), 10));
				}
				m_featureNode->init();

				m_bFirstClick = true;

				// add 360 Ring
				osg::Vec3d _end = m_feature->getGeometry()->back();
				osg::Vec3d _start = m_feature->getGeometry()->front();

				double _numSpokes = 50.0;
				double delta = osg::PI * 2.0 / _numSpokes;

				auto _dis = osgEarth::GeoMath::distance(_start, _end, m_spatRef);
				_dis = osg::clampAbove(_dis, 1.0);
				auto _centerPoint = osgEarth::GeoPoint(m_spatRef->getGeographicSRS(), _start.x(), _start.y(), m_losHeight, osgEarth::AltitudeMode::ALTMODE_RELATIVE);
				
				osg::Vec3d			_centerWorld;

				_centerPoint.toWorld(_centerWorld, m_mapNode->getTerrain());

				osg::Vec3d up = osg::Vec3d(_centerWorld);
				up.normalize();

				osg::Vec3d side = up ^ osg::Vec3d(0, 0, 1);

				m_feature->getGeometry()->clear();
				for (unsigned int i = 0; i < (unsigned int)_numSpokes; i++)
				{
					double angle = delta * (double)i;
					osg::Quat quat(angle, up);
					osg::Vec3d spoke = quat * (side * _dis);
					osg::Vec3d end = _centerWorld + spoke;
					_centerPoint.fromWorld(m_spatRef->getGeographicSRS(), end);
					m_feature->getGeometry()->push_back(_centerPoint.vec3d());
				}
				m_feature->getGeometry()->push_back(m_feature->getGeometry()->front());
				m_featureNode->init();

			}
		}break;
		// 雷达分析
		case EnumActionEvent::RadarAnalysis:
		{
			// OE_WARN << "pickLeft RadarAnalysis" << std::endl;
			if (m_bFirstClick)
			{
				m_bFirstClick = false;
			}
			else
			{
				m_bFirstClick = true;
			}
				
		}break;
		default:break;
	}
}

void PickEvent::pickMove(osg::Vec3d Point)
{
	// 更新右下角的坐标
	auto tempStr = QString(m_csysTitle + " x: " + QString::number(Point.x(), 'f', 6) + " | y: " + QString::number(Point.y(), 'f', 6) + " | z: " + QString::number(Point.z(), 'f', 6) + " |  " + m_mapName);
	m_Label->setText(tempStr);

	switch (m_ActionEvent)
	{
		// 通视分析
		case EnumActionEvent::VisibilityAnalysis:
		{
			if (!m_bFirstClick)
			{
				osgEarth::GeoPoint end(m_spatRef->getGeographicSRS(), Point.x(), Point.y(), 0, osgEarth::AltitudeMode::ALTMODE_RELATIVE);
				m_curLosNode->setEnd(end);

			}
		}break;
		// 视域分析
		case EnumActionEvent::ViewshedAnalysis:
		{
			if (!m_bFirstClick)
			{
				auto _dis = osgEarth::GeoMath::distance(FirstPoint, Point, m_spatRef);
				m_curCircleNode->setRadius(_dis);
				//m_curRosNode->setRadius(dis);

				if (!m_bLastPoint)
				{
					m_feature->getGeometry()->push_back(osg::Vec3d(Point.x(), Point.y(), 0));
					m_bLastPoint = true;
				}
				else
				{
					m_feature->getGeometry()->back() = osg::Vec3d(Point.x(), Point.y(), 0);
				}

				m_featureNode->init();
			}

		}break;
		// 雷达分析
		case EnumActionEvent::RadarAnalysis:
		{
			// OE_WARN << "pickMove RadarAnalysis" << std::endl;
			if (!m_bFirstClick)
			{

			}
			else
			{

			}

		}break;
		default:break;
	}
}

void PickEvent::setActionEvent(const EnumActionEvent &ae)
{
	m_ActionEvent = ae;
	m_bFirstClick = true;
}

void PickEvent::RemoveAnalysis()
{
	// 通视分析结果清理
	m_losGroup->removeChildren(0, m_losGroup->getNumChildren());
}

osg::Vec3d PickEvent::Screen2Geo(float x, float y)
{
	osg::Vec3d world;
	osgEarth::GeoPoint geoPoint;
	if (m_mapNode->getTerrain()->getWorldCoordsUnderMouse(m_viewer, x, y, world))
	{
		geoPoint.fromWorld(m_spatRef, world);
	}
	return geoPoint.vec3d();
}

void PickEvent::setLosHeight(float height)
{
	m_losHeight = height;
}
