#include "PickEvent.h"
#include <QDebug>
#include <osgEarth/GeoMath>

#include <osgEarthFeatures/Feature>
#include <osgEarthAnnotation/FeatureNode>

PickEvent::PickEvent(QLabel* label, osg::ref_ptr<osgEarth::MapNode> mapNode, osg::Group* losGroup) :
	m_ActionEvent(EnumActionEvent::ActionNull),
	m_Label(label),
	m_mapNode(mapNode),
	m_losGroup(losGroup),
	m_bFirstClick(false),
	m_losHeight(2.0),
	m_mapName(QString::fromUtf8(mapNode->getMapSRS()->getName().c_str())),
	m_csysTitle(QString::fromLocal8Bit("坐标:"))
{
	m_spatRef = m_mapNode->getMapSRS();
	m_Calculator = new osgEarth::Util::TerrainProfileCalculator(m_mapNode);

	// 贴地的 圆
	m_circleStyle.getOrCreate<osgEarth::Symbology::PolygonSymbol>()->fill()->color() = osgEarth::Color(osgEarth::Color::Red, 0.4f);
	m_circleStyle.getOrCreate<osgEarth::Symbology::AltitudeSymbol>()->clamping() = osgEarth::Symbology::AltitudeSymbol::CLAMP_TO_TERRAIN;
	m_circleStyle.getOrCreate<osgEarth::Symbology::AltitudeSymbol>()->technique() = osgEarth::Symbology::AltitudeSymbol::TECHNIQUE_DRAPE;
	m_circleStyle.getOrCreate<osgEarth::Symbology::AltitudeSymbol>()->verticalOffset() = 0.1;
	m_circleStyle.getOrCreate<osgEarth::Symbology::RenderSymbol>()->order();

	osgEarth::Annotation::FeatureNode* pathNode = 0;
	{
		osgEarth::Symbology::Geometry* path = new osgEarth::Symbology::LineString();
		path->push_back(osg::Vec3d(-74, 40.714, 0));   // New York
		path->push_back(osg::Vec3d(139.75, 35.68, 0)); // Tokyo

		osgEarth::Features::Feature* pathFeature = new osgEarth::Features::Feature(path, m_spatRef->getGeographicSRS());
		pathFeature->geoInterp() = osgEarth::Features::GEOINTERP_GREAT_CIRCLE;

		osgEarth::Symbology::Style pathStyle;
		pathStyle.getOrCreate<osgEarth::Symbology::LineSymbol>()->stroke()->color() = osgEarth::Symbology::Color::Red;
		pathStyle.getOrCreate<osgEarth::Symbology::LineSymbol>()->stroke()->width() = 2.0f;
		pathStyle.getOrCreate<osgEarth::Symbology::LineSymbol>()->stroke()->smooth() = true;
		pathStyle.getOrCreate<osgEarth::Symbology::LineSymbol>()->tessellationSize() = 75000;
		pathStyle.getOrCreate<osgEarth::Symbology::PointSymbol>()->size() = 14;
		pathStyle.getOrCreate<osgEarth::Symbology::PointSymbol>()->fill()->color() = osgEarth::Symbology::Color::Red;
		pathStyle.getOrCreate<osgEarth::Symbology::PointSymbol>()->smooth() = true;
		pathStyle.getOrCreate<osgEarth::Symbology::AltitudeSymbol>()->clamping() = osgEarth::Symbology::AltitudeSymbol::CLAMP_TO_TERRAIN;
		pathStyle.getOrCreate<osgEarth::Symbology::AltitudeSymbol>()->technique() = osgEarth::Symbology::AltitudeSymbol::TECHNIQUE_GPU;
		pathStyle.getOrCreate<osgEarth::Symbology::RenderSymbol>()->depthOffset()->enabled() = true;

		pathNode = new osgEarth::Annotation::FeatureNode(pathFeature, pathStyle);
		m_losGroup->addChild(pathNode);

	}


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
				osgEarth::GeoPoint start(m_spatRef->getGeographicSRS(), Point.x(), Point.y(), m_losHeight, osgEarth::AltitudeMode::ALTMODE_RELATIVE);
				m_curLosNode = new osgEarth::Util::LinearLineOfSightNode(m_mapNode.get());
				m_curLosNode->setDisplayMode(osgEarth::Util::LineOfSight::DisplayMode::MODE_SPLIT);
				m_curLosNode->setStart(start);
				m_losGroup->addChild(m_curLosNode);
				m_bFirstClick = false;

				
			}
			else
			{
				auto start = osgEarth::GeoPoint(m_spatRef->getGeographicSRS(), LastPoint, osgEarth::AltitudeMode::ALTMODE_ABSOLUTE);
				auto end = osgEarth::GeoPoint(m_spatRef->getGeographicSRS(), Point, osgEarth::AltitudeMode::ALTMODE_ABSOLUTE);
				auto _profile = m_Calculator->getProfile();
				m_Calculator->computeTerrainProfile(m_mapNode, start, end, _profile);
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

				m_bFirstClick = false;
			}
			else
			{
				

				m_bFirstClick = true;
			}
		}break;
		// 雷达分析
		case EnumActionEvent::RadarAnalysis:
		{
			OE_WARN << "pickLeft RadarAnalysis" << std::endl;
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
				auto dis = osgEarth::GeoMath::distance(FirstPoint, Point, m_mapNode->getMapSRS());
				m_curCircleNode->setRadius(dis);


				//m_curRosNode->setRadius(dis);
			}

		}break;
		// 雷达分析
		case EnumActionEvent::RadarAnalysis:
		{
			OE_WARN << "pickMove RadarAnalysis" << std::endl;
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