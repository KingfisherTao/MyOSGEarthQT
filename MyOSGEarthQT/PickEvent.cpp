#include "PickEvent.h"
#include <osgEarth/GLUtils>

PickEvent::PickEvent(QLabel* label, osgEarth::MapNode* mapNode, osg::Group* losGroup) :
	m_ActionEvent(EnumActionEvent::ActionNull),
	m_Label(label),
	m_mapNode(mapNode),
	m_losGroup(losGroup),
	m_bFirstClick(false),
	m_bLastPoint(false),
	m_ui_losHeight(2.0),
	m_ui_numSpokes(200),
	m_ui_numSegment(200),
	m_mapName(QString::fromUtf8(mapNode->getMapSRS()->getName().c_str())),
	m_csysTitle(QString::fromLocal8Bit("坐标:"))
{
	m_spatRef = m_mapNode->getMapSRS();
	// 剖面计算器
	//m_Calculator = new osgEarth::Util::TerrainProfileCalculator(m_mapNode);

	m_Group = new osg::Group();
	m_mapNode->addChild(m_Group);

	// 贴地的 圆 style
	m_circleStyle.getOrCreate<osgEarth::Symbology::PolygonSymbol>()->fill()->color() = osgEarth::Color(osgEarth::Color::Red, 0.3f);
	m_circleStyle.getOrCreate<osgEarth::Symbology::AltitudeSymbol>()->clamping() = osgEarth::Symbology::AltitudeSymbol::CLAMP_TO_TERRAIN;
	m_circleStyle.getOrCreate<osgEarth::Symbology::AltitudeSymbol>()->technique() = osgEarth::Symbology::AltitudeSymbol::TECHNIQUE_DRAPE;
	m_circleStyle.getOrCreate<osgEarth::Symbology::AltitudeSymbol>()->verticalOffset() = 0.1;
	m_circleStyle.getOrCreate<osgEarth::Symbology::RenderSymbol>()->order();

	// 贴地的 outline style
	m_circleOutLineStyle.getOrCreateSymbol<osgEarth::Symbology::LineSymbol>()->stroke()->color() = osgEarth::Color(osgEarth::Color::Yellow, 1.0f);
	m_circleOutLineStyle.getOrCreateSymbol<osgEarth::Symbology::LineSymbol>()->stroke()->width() = 2.0f;
	m_circleOutLineStyle.getOrCreate<osgEarth::Symbology::PolygonSymbol>()->fill()->color() = osgEarth::Symbology::Color(osgEarth::Color::White, 0.0); // transparent fill
	m_circleOutLineStyle.getOrCreate<osgEarth::Symbology::AltitudeSymbol>()->clamping() = osgEarth::Symbology::AltitudeSymbol::CLAMP_TO_TERRAIN;
	m_circleOutLineStyle.getOrCreate<osgEarth::Symbology::AltitudeSymbol>()->technique() = osgEarth::Symbology::AltitudeSymbol::TECHNIQUE_DRAPE;

	// 贴地的线:
	m_feature = new osgEarth::Features::Feature(new osgEarth::Symbology::LineString(), m_spatRef);
	m_feature->geoInterp() = osgEarth::Features::GEOINTERP_GREAT_CIRCLE;

	osgEarth::Symbology::AltitudeSymbol* _alt = m_feature->style()->getOrCreate<osgEarth::Symbology::AltitudeSymbol>();
	_alt->clamping() = _alt->CLAMP_TO_TERRAIN;
	_alt->technique() = _alt->TECHNIQUE_DRAPE;

	osgEarth::Symbology::RenderSymbol* _render = m_feature->style()->getOrCreate<osgEarth::Symbology::RenderSymbol>();
	_render->depthOffset()->enabled() = true;
	_render->depthOffset()->automatic() = true;

	osgEarth::Symbology::LineSymbol* _ls = m_feature->style()->getOrCreate<osgEarth::Symbology::LineSymbol>();
	_ls->stroke()->color() = osgEarth::Color(osgEarth::Color::Yellow, 1.0f);
	_ls->stroke()->width() = 1.0f;

	m_featureNode = new osgEarth::Annotation::FeatureNode(m_feature);
	m_Group->addChild(m_featureNode.get());

	//m_pFA = NULL;
	//m_pFA = new CReferenceArea(m_mapNode);
	//m_pFA->setNumSpokes(100.0);
	//m_Group->addChild(m_pFA->get());
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
				auto _start = osgEarth::GeoPoint(m_spatRef->getGeographicSRS(), Point.x(), Point.y(), m_ui_losHeight, osgEarth::AltitudeMode::ALTMODE_RELATIVE);
				m_curLosNode = new osgEarth::Util::LinearLineOfSightNode(m_mapNode);
				m_curLosNode->setDisplayMode(osgEarth::Util::LineOfSight::DisplayMode::MODE_SPLIT);
				m_curLosNode->setStart(_start);
				m_losGroup->addChild(m_curLosNode);

				m_bFirstClick = false;
			}
			else
			{
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

				m_curCircleOutLine = new osgEarth::Annotation::CircleNode;
				m_curCircleOutLine->setStyle(m_circleOutLineStyle);
				osgEarth::GLUtils::setLighting(m_curCircleOutLine->getOrCreateStateSet(), osg::StateAttribute::OFF);
				m_curCircleOutLine->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);

				auto _centerPoint = osgEarth::GeoPoint(m_spatRef->getGeographicSRS(), Point.x(), Point.y(), 0, osgEarth::AltitudeMode::ALTMODE_RELATIVE);

				m_curCircleNode->setPosition(_centerPoint);
				m_curCircleOutLine->setPosition(_centerPoint);
				m_losGroup->addChild(m_curCircleNode.get());
				m_losGroup->addChild(m_curCircleOutLine.get());

				m_feature->getGeometry()->clear();
				m_feature->getGeometry()->push_back(osg::Vec3d(Point.x(), Point.y(), 10));
				//m_pFA->setStart(Point);

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

				m_feature->getGeometry()->clear();
				m_featureNode->dirty();

				// 地形剖面 dis 两次鼠标点击距离， elevNum 变化的高程count
				//auto _profile = m_Calculator->getProfile();
				//m_Calculator->computeTerrainProfile(m_mapNode, _start, _end, _profile);
				//auto dis = _profile.getTotalDistance();
				//auto elevNum = _profile.getNumElevations();

				// Analysis
				auto _start = osgEarth::GeoPoint(m_spatRef->getGeographicSRS(), LastPoint, osgEarth::AltitudeMode::ALTMODE_ABSOLUTE);
				auto _end = osgEarth::GeoPoint(m_spatRef->getGeographicSRS(), Point, osgEarth::AltitudeMode::ALTMODE_ABSOLUTE);
				auto _angle = osgEarth::GeoMath::bearing(osg::DegreesToRadians(_start.y()), osg::DegreesToRadians(_start.x()),
														osg::DegreesToRadians(_end.y()), osg::DegreesToRadians(_end.x()));

				m_pLT = new DrawLineCallback(LastPoint, 
											_angle, 
											osgEarth::GeoMath::distance(LastPoint, Point, m_spatRef), 
											(double)m_ui_numSpokes,
											(double)m_ui_numSegment, 
											m_ui_losHeight, m_mapNode);
				m_losGroup->addChild(m_pLT->get());
				m_vLT.push_back(m_pLT);

				m_bFirstClick = true;
			}
		}break;
		// 雷达分析
		case EnumActionEvent::RadarAnalysis:
		{
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
				osgEarth::GeoPoint _end(m_spatRef->getGeographicSRS(), Point.x(), Point.y(), 0.0, osgEarth::AltitudeMode::ALTMODE_RELATIVE);
				m_curLosNode->setEnd(_end);
			}
		}break;
		// 视域分析
		case EnumActionEvent::ViewshedAnalysis:
		{
			if (!m_bFirstClick)
			{
				auto _dis = osgEarth::GeoMath::distance(FirstPoint, Point, m_spatRef);
				m_curCircleNode->setRadius(_dis);
				m_curCircleOutLine->setRadius(_dis);

				//m_pFA->setRadius(_dis);
				if (!m_bLastPoint)
				{
					m_feature->getGeometry()->push_back(osg::Vec3d(Point.x(), Point.y(), 0));
					m_bLastPoint = true;
				}
				else
				{
					m_feature->getGeometry()->back() = osg::Vec3d(Point.x(), Point.y(), 0);
				}

				m_featureNode->dirty();
			}

		}break;
		// 雷达分析
		case EnumActionEvent::RadarAnalysis:
		{
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
	m_feature->getGeometry()->clear();
	m_featureNode->dirty();
	//m_pFA->clear();

	m_vLT.clear();
//	m_vLT.swap(std::vector<DrawLineCallback*>());

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
	m_ui_losHeight = height;
}

void PickEvent::setViewshedPara(int numSpokes, int numSegment)
{
	m_ui_numSpokes = numSpokes;
	m_ui_numSegment = numSegment;
}