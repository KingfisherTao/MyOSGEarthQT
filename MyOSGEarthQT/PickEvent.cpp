#include "PickEvent.h"

PickEvent::PickEvent(QLabel* label, osgEarth::MapNode* mapNode, osg::Group* losGroup) :
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
	// 剖面计算器
	m_Calculator = new osgEarth::Util::TerrainProfileCalculator(m_mapNode);

	// m_curRosNode = NULL;

	m_Group = new osg::Group();
	m_mapNode->addChild(m_Group);

	// 贴地的 圆
	m_circleStyle.getOrCreate<osgEarth::Symbology::PolygonSymbol>()->fill()->color() = osgEarth::Color(osgEarth::Color::Red, 0.3f);
	m_circleStyle.getOrCreate<osgEarth::Symbology::AltitudeSymbol>()->clamping() = osgEarth::Symbology::AltitudeSymbol::CLAMP_TO_TERRAIN;
	m_circleStyle.getOrCreate<osgEarth::Symbology::AltitudeSymbol>()->technique() = osgEarth::Symbology::AltitudeSymbol::TECHNIQUE_DRAPE;
	m_circleStyle.getOrCreate<osgEarth::Symbology::AltitudeSymbol>()->verticalOffset() = 0.1;
	m_circleStyle.getOrCreate<osgEarth::Symbology::RenderSymbol>()->order();

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
	ls->stroke()->color() = osgEarth::Color(osgEarth::Color::Yellow, 1.0f);
	ls->stroke()->width() = 2.0f;
	//ls->stroke()->stipple() = 255;
	ls->tessellation() = 150;

	m_featureNode = new osgEarth::Annotation::FeatureNode(m_feature.get());
	m_Group->addChild(m_featureNode.get());

	m_pFA = NULL;
	m_pFA = new CReferenceArea(m_mapNode);
	m_pFA->setNumSpokes(100.0);
	m_Group->addChild(m_pFA->get());
}

PickEvent::~PickEvent()
{
	if (m_pFA)
	{
		delete m_pFA;
		m_pFA = nullptr;
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
				auto _start = osgEarth::GeoPoint(m_spatRef->getGeographicSRS(), Point.x(), Point.y(), m_losHeight, osgEarth::AltitudeMode::ALTMODE_RELATIVE);
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
				auto centerPoint = osgEarth::GeoPoint(m_spatRef->getGeographicSRS(), Point.x(), Point.y(), 0, osgEarth::AltitudeMode::ALTMODE_RELATIVE);
				m_curCircleNode->setPosition(centerPoint);
				m_losGroup->addChild(m_curCircleNode.get());

				//m_curRosNode = new osgEarth::Util::RadialLineOfSightNode(m_mapNode);
				//m_curRosNode->setCenter(centerPoint);
				//m_curRosNode->setRadius(0);
				//m_curRosNode->setNumSpokes(100);
				//m_curRosNode->setDisplayMode(osgEarth::Util::LineOfSight::DisplayMode::MODE_SINGLE);
				//m_losGroup->addChild(m_curRosNode);

				m_feature->getGeometry()->clear();
				m_feature->getGeometry()->push_back(osg::Vec3d(Point.x(), Point.y(), 10));
				m_pFA->setStart(Point);

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

				// 地形剖面 dis 两次鼠标点击距离， elevNum 变化的高程count
				//auto _start = osgEarth::GeoPoint(m_spatRef->getGeographicSRS(), LastPoint, osgEarth::AltitudeMode::ALTMODE_ABSOLUTE);
				//auto _end = osgEarth::GeoPoint(m_spatRef->getGeographicSRS(), Point, osgEarth::AltitudeMode::ALTMODE_ABSOLUTE);
				//auto _profile = m_Calculator->getProfile();
				//m_Calculator->computeTerrainProfile(m_mapNode, _start, _end, _profile);
				//auto dis = _profile.getTotalDistance();
				//auto elevNum = _profile.getNumElevations();

				// Analysis
				m_pLT = new DrawLineThread(LastPoint, osgEarth::GeoMath::distance(LastPoint, Point, m_spatRef), 150.0, m_spatRef);
				m_losGroup->addChild(m_pLT->get());
				m_vLT.push_back(m_pLT);

				m_pCT = new DrawCircleThread(LastPoint, osgEarth::GeoMath::distance(LastPoint, Point, m_spatRef), 50.0, m_losGroup, m_spatRef);
				m_pCT->setLT(m_pLT);
				//m_losGroup->addChild(m_pCT->get());
				m_vCT.push_back(m_pCT);

				m_feature->getGeometry()->clear();
				m_featureNode->dirty();
				
				m_pLT->start();
				m_pCT->start();

				m_bFirstClick = true;
			}
		}break;
		// 雷达分析
		case EnumActionEvent::RadarAnalysis:
		{
			// OE_INFO << "pickLeft RadarAnalysis" << std::endl;
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
				m_pFA->setRadius(_dis);
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
			// OE_INFO << "pickMove RadarAnalysis" << std::endl;
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
	m_pFA->clear();

	
	for (std::vector<DrawLineThread*>::iterator ite = m_vLT.begin(); ite != m_vLT.end(); ++ite)
	{
		(*ite)->clear();
		delete *ite;
	}
	m_vLT.clear();
	m_vLT.swap(std::vector<DrawLineThread*>());

	for (std::vector<DrawCircleThread*>::iterator ite = m_vCT.begin(); ite != m_vCT.end(); ++ite)
	{
		(*ite)->clear();
		delete *ite;
	}
	m_vCT.clear();
	m_vCT.swap(std::vector<DrawCircleThread*>());
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
