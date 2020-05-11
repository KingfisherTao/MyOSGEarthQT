#include "MyOSGEarthQT.h"

#include <QDebug>
#include <QLabel>
#include <osg/Depth>
//===============================================================================
#include <osgEarth/GLUtils>
#include <osgEarthUtil/TerrainProfile>
#include <osgEarth/Registry>
#include <osgEarthFeatures/Feature>
#include <osgEarthAnnotation/FeatureNode>

//Creates a simple HUD camera
osg::Camera* createHud(double width, double height)
{
	osg::Camera* hud = new osg::Camera;
	hud->setProjectionMatrix(osg::Matrix::ortho2D(0, width, 0, height));
	hud->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
	hud->setViewMatrix(osg::Matrix::identity());
	hud->setClearMask(GL_DEPTH_BUFFER_BIT);
	hud->setRenderOrder(osg::Camera::POST_RENDER);
	hud->setAllowEventFocus(false);
	osg::StateSet* hudSS = hud->getOrCreateStateSet();
	osgEarth::GLUtils::setLighting(hudSS, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);
	hudSS->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
	hudSS->setMode(GL_BLEND, osg::StateAttribute::ON);

	return hud;
}
/**
 * Simple terrain profile display
 */
class TerrainProfileGraph : public osg::Group
{
public:
	/*
	 * Callback that is fired when the TerrainProfile changes
	 */
	struct GraphChangedCallback : public osgEarth::Util::TerrainProfileCalculator::ChangedCallback
	{
		GraphChangedCallback(TerrainProfileGraph* graph) :
			_graph(graph)
		{
		}

		virtual void onChanged(const osgEarth::Util::TerrainProfileCalculator* sender)
		{
			_graph->setTerrainProfile(sender->getProfile());
		}

		TerrainProfileGraph* _graph;
	};

	TerrainProfileGraph(osgEarth::Util::TerrainProfileCalculator* profileCalculator, double graphWidth = 200, double graphHeight = 200) :
		_profileCalculator(profileCalculator),
		_graphWidth(graphWidth),
		_graphHeight(graphHeight),
		_color(1.0f, 1.0f, 0.0f, 1.0f),
		_backcolor(0.0f, 0.0f, 0.0f, 0.5f)
	{
		_graphChangedCallback = new GraphChangedCallback(this);
		_profileCalculator->addChangedCallback(_graphChangedCallback.get());

		float textSize = 8;
		osg::ref_ptr< osgText::Font> font = osgEarth::Registry::instance()->getDefaultFont();
		osg::Vec4 textColor = osg::Vec4f(1, 0, 0, 1);

		_distanceMinLabel = new osgText::Text();
		_distanceMinLabel->setCharacterSize(textSize);
		_distanceMinLabel->setFont(font.get());
		_distanceMinLabel->setAlignment(osgText::TextBase::LEFT_BOTTOM);
		_distanceMinLabel->setColor(textColor);

		_distanceMaxLabel = new osgText::Text();
		_distanceMaxLabel->setCharacterSize(textSize);
		_distanceMaxLabel->setFont(font.get());
		_distanceMaxLabel->setAlignment(osgText::TextBase::RIGHT_BOTTOM);
		_distanceMaxLabel->setColor(textColor);

		_elevationMinLabel = new osgText::Text();
		_elevationMinLabel->setCharacterSize(textSize);
		_elevationMinLabel->setFont(font.get());
		_elevationMinLabel->setAlignment(osgText::TextBase::RIGHT_BOTTOM);
		_elevationMinLabel->setColor(textColor);

		_elevationMaxLabel = new osgText::Text();
		_elevationMaxLabel->setCharacterSize(textSize);
		_elevationMaxLabel->setFont(font.get());
		_elevationMaxLabel->setAlignment(osgText::TextBase::RIGHT_TOP);
		_elevationMaxLabel->setColor(textColor);
	}

	~TerrainProfileGraph()
	{
		_profileCalculator->removeChangedCallback(_graphChangedCallback.get());
	}

	void setTerrainProfile(const osgEarth::Util::TerrainProfile& profile)
	{
		_profile = profile;
		redraw();
	}

	//Redraws the graph
	void redraw()
	{
		removeChildren(0, getNumChildren());

		addChild(createBackground(_graphWidth, _graphHeight, _backcolor));

		osg::Geometry* geom = new osg::Geometry;
		geom->setUseVertexBufferObjects(true);

		osg::Vec3Array* verts = new osg::Vec3Array();
		verts->reserve(_profile.getNumElevations());
		geom->setVertexArray(verts);
		if (verts->getVertexBufferObject())
			verts->getVertexBufferObject()->setUsage(GL_STATIC_DRAW_ARB);

		osg::Vec4Array* colors = new osg::Vec4Array(osg::Array::BIND_OVERALL);
		colors->push_back(_color);
		geom->setColorArray(colors);

		double minElevation, maxElevation;
		_profile.getElevationRanges(minElevation, maxElevation);
		double elevationRange = maxElevation - minElevation;

		double totalDistance = _profile.getTotalDistance();

		for (unsigned int i = 0; i < _profile.getNumElevations(); i++)
		{
			double distance = _profile.getDistance(i);
			double elevation = _profile.getElevation(i);

			double x = (distance / totalDistance) * _graphWidth;
			double y = ((elevation - minElevation) / elevationRange) * _graphHeight;
			verts->push_back(osg::Vec3(x, y, 0));
		}

		geom->addPrimitiveSet(new osg::DrawArrays(GL_LINE_STRIP, 0, verts->size()));
		osg::Geode* graphGeode = new osg::Geode;
		graphGeode->addDrawable(geom);
		addChild(graphGeode);

		osg::Geode* labelGeode = new osg::Geode;
		labelGeode->addDrawable(_distanceMinLabel.get());
		labelGeode->addDrawable(_distanceMaxLabel.get());
		labelGeode->addDrawable(_elevationMinLabel.get());
		labelGeode->addDrawable(_elevationMaxLabel.get());

		_distanceMinLabel->setPosition(osg::Vec3(0, 0, 0));
		_distanceMaxLabel->setPosition(osg::Vec3(_graphWidth - 15, 0, 0));
		_elevationMinLabel->setPosition(osg::Vec3(_graphWidth - 5, 10, 0));
		_elevationMaxLabel->setPosition(osg::Vec3(_graphWidth - 5, _graphHeight, 0));

		_distanceMinLabel->setText("0m");
		_distanceMaxLabel->setText(osgEarth::toString<int>((int)totalDistance) + std::string("m"));

		_elevationMinLabel->setText(osgEarth::toString<int>((int)minElevation) + std::string("m"));
		_elevationMaxLabel->setText(osgEarth::toString<int>((int)maxElevation) + std::string("m"));

		addChild(labelGeode);

	}

	osg::Node* createBackground(double width, double height, const osg::Vec4f& backgroundColor)
	{
		//Create a background quad
		osg::Geometry* geometry = new osg::Geometry();
		geometry->setUseVertexBufferObjects(true);

		osg::Vec3Array* verts = new osg::Vec3Array();
		verts->reserve(4);
		verts->push_back(osg::Vec3(0, 0, 0));
		verts->push_back(osg::Vec3(width, 0, 0));
		verts->push_back(osg::Vec3(width, height, 0));
		verts->push_back(osg::Vec3(0, height, 0));
		geometry->setVertexArray(verts);
		if (verts->getVertexBufferObject())
			verts->getVertexBufferObject()->setUsage(GL_STATIC_DRAW_ARB);

		osg::Vec4Array* colors = new osg::Vec4Array(osg::Array::BIND_OVERALL);
		colors->push_back(backgroundColor);
		geometry->setColorArray(colors);

		geometry->addPrimitiveSet(new osg::DrawArrays(GL_QUADS, 0, 4));

		osg::Geode* geode = new osg::Geode;
		geode->addDrawable(geometry);

		return geode;

	}

	osg::ref_ptr<osgText::Text> _distanceMinLabel, _distanceMaxLabel, _elevationMinLabel, _elevationMaxLabel;

	osg::Vec4f _backcolor;
	osg::Vec4f _color;
	osgEarth::Util::TerrainProfile _profile;
	osg::ref_ptr< osgEarth::Util::TerrainProfileCalculator > _profileCalculator;
	double _graphWidth, _graphHeight;
	osg::ref_ptr< GraphChangedCallback > _graphChangedCallback;
};

/*
 * Simple event handler that draws a line when you click two points with the left mouse button
 */
class DrawProfileEventHandler : public osgGA::GUIEventHandler
{
public:
	DrawProfileEventHandler(osgEarth::MapNode* mapNode, osg::Group* root, osgEarth::Util::TerrainProfileCalculator* profileCalculator) :
		_mapNode(mapNode),
		_root(root),
		_startValid(false),
		_profileCalculator(profileCalculator)
	{
		_start = profileCalculator->getStart().vec3d();
		_end = profileCalculator->getEnd().vec3d();
		compute();
	}

	bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
	{
		if (ea.getEventType() == ea.PUSH && ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
		{
			osg::Vec3d world;
			if (_mapNode->getTerrain()->getWorldCoordsUnderMouse(aa.asView(), ea.getX(), ea.getY(), world))
			{
				osgEarth::GeoPoint mapPoint;
				mapPoint.fromWorld(_mapNode->getMapSRS(), world);
				//_mapNode->getMap()->worldPointToMapPoint( world, mapPoint );

				if (!_startValid)
				{
					_startValid = true;
					_start = mapPoint.vec3d();
					if (_featureNode.valid())
					{
						_root->removeChild(_featureNode.get());
						_featureNode = 0;
					}
				}
				else
				{
					_end = mapPoint.vec3d();
					compute();
					_startValid = false;
				}
			}
		}
		return false;
	}

	void compute()
	{
		//Tell the calculator about the new start/end points
		_profileCalculator->setStartEnd(osgEarth::GeoPoint(_mapNode->getMapSRS(), _start.x(), _start.y()),
			osgEarth::GeoPoint(_mapNode->getMapSRS(), _end.x(), _end.y()));

		if (_featureNode.valid())
		{
			_root->removeChild(_featureNode.get());
			_featureNode = 0;
		}

		osgEarth::LineString* line = new osgEarth::LineString();
		line->push_back(_start);
		line->push_back(_end);
		osgEarth::Features::Feature* feature = new osgEarth::Features::Feature(line, _mapNode->getMapSRS());
		feature->geoInterp() = osgEarth::Features::GEOINTERP_GREAT_CIRCLE;

		//Define a style for the line
		osgEarth::Symbology::Style style;
		osgEarth::Symbology::LineSymbol* ls = style.getOrCreateSymbol<osgEarth::Symbology::LineSymbol>();
		ls->stroke()->color() = osgEarth::Symbology::Color::Yellow;
		ls->stroke()->width() = 3.0f;
		ls->tessellationSize()->set(100.0, osgEarth::Symbology::Units::KILOMETERS);

		osgEarth::Symbology::AltitudeSymbol* alt = style.getOrCreate<osgEarth::Symbology::AltitudeSymbol>();
		alt->clamping() = alt->CLAMP_TO_TERRAIN;
		alt->technique() = alt->TECHNIQUE_DRAPE;

		osgEarth::Symbology::RenderSymbol* render = style.getOrCreate<osgEarth::Symbology::RenderSymbol>();
		render->lighting() = false;

		feature->style() = style;
		_featureNode = new osgEarth::Annotation::FeatureNode(feature);
		_featureNode->setMapNode(_mapNode);
		_root->addChild(_featureNode.get());

	}
	osgEarth::MapNode* _mapNode;
	osg::Group* _root;
	osgEarth::Util::TerrainProfileCalculator* _profileCalculator;
	osg::ref_ptr<osgEarth::Annotation::FeatureNode> _featureNode;
	bool _startValid;
	osg::Vec3d _start;
	osg::Vec3d _end;
};

//===============================================================================


MyOSGEarthQT::MyOSGEarthQT(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	setWindowState(Qt::WindowMinimized);
	setWindowState(Qt::WindowActive);

	// 初始化设置视高子窗口
	sub_setLosHeight = new SetLosHeight(this);

	// 设置状态栏
	auto _lab = new QLabel(this);
	_lab->setFrameStyle(QFrame::NoFrame);
	_lab->setText("");
	_lab->setMinimumSize(_lab->sizeHint());
	_lab->setFont(QFont("", 12, QFont::Normal));
	ui.statusBar->setStyleSheet(QString("QStatusBar::item{border: 0px}"));
	ui.statusBar->addPermanentWidget(_lab);

	// 世界 Group
	m_world = new osg::Group();

	m_mapNode = osgEarth::MapNode::findMapNode(osgDB::readNodeFile("../data/World.earth"));
	m_world->addChild(m_mapNode.get());

	// 近地面自动裁剪
	ui.openGLWidget->getCamera()->addCullCallback(new osgEarth::Util::AutoClipPlaneCullCallback(m_mapNode.get()));

	// 星空
	osg::ref_ptr<osgEarth::Util::SkyNode> skyNode = osgEarth::Util::SkyNode::create();
	osg::ref_ptr<osgEarth::Util::Ephemeris> ephemeris = new osgEarth::Util::Ephemeris;
	skyNode->setEphemeris(ephemeris);
	skyNode->setName("skyNode");
	skyNode->setDateTime(osgEarth::DateTime());
	skyNode->attach(ui.openGLWidget->getViewer(), 0);
	skyNode->setLighting(true);
	skyNode->addChild(m_mapNode);
	m_world->addChild(skyNode);

	// 绘制线的组
	m_losGroup = new osg::Group();
	m_losGroup->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
	m_losGroup->getOrCreateStateSet()->setAttributeAndModes(new osg::Depth(osg::Depth::Function::ALWAYS));
	m_mapNode->addChild(m_losGroup);

	// 左上角的UI
	osgEarth::Util::Controls::ControlCanvas* canvas = new osgEarth::Util::Controls::ControlCanvas();
	m_world->addChild(canvas);
	canvas->setNodeMask(0x1 << 1);

	osgEarth::Util::Controls::Grid* grid = new osgEarth::Util::Controls::Grid();
	grid->setBackColor(0, 0, 0, 0.2);
	grid->setAbsorbEvents(true);
	grid->setVertAlign(osgEarth::Util::Controls::Control::ALIGN_TOP);

	// 左上角 UI 中的 lab
	auto mouseLabel = new osgEarth::Util::Controls::LabelControl();
	grid->setControl(0, 0, new osgEarth::Util::Controls::LabelControl("Mouse:"));
	grid->setControl(1, 0, mouseLabel);

	canvas->addControl(grid);

	//double backgroundWidth = 500;
	//double backgroundHeight = 500;

	//double graphWidth = 200;
	//double graphHeight = 100;

	//Add the hud
	//osg::Camera* hud = createHud(backgroundWidth, backgroundHeight);
	//m_world->addChild(hud);

	//osg::ref_ptr< osgEarth::Util::TerrainProfileCalculator > calculator = new osgEarth::Util::TerrainProfileCalculator(m_mapNode,
	//	osgEarth::GeoPoint(m_mapNode->getMapSRS(), -124.0, 40.0),
	//	osgEarth::GeoPoint(m_mapNode->getMapSRS(), -75.1, 39.2)
	//);

	//osg::Group* profileNode = new TerrainProfileGraph(calculator.get(), graphWidth, graphHeight);
	//hud->addChild(profileNode);
	//ui.openGLWidget->getViewer()->addEventHandler(new DrawProfileEventHandler(m_mapNode, m_mapNode, calculator.get()));

	ui.openGLWidget->getViewer()->addEventHandler(new osgEarth::Util::MouseCoordsTool(m_mapNode, mouseLabel));
	ui.openGLWidget->getViewer()->setSceneData(m_world.get());

	connect(ui.VisibilityAnalysis, &QAction::triggered, this, &MyOSGEarthQT::on_VisibilityAnalysis);
	connect(ui.ViewshedAnalysis, &QAction::triggered, this, &MyOSGEarthQT::on_ViewshedAnalysis);
	connect(ui.RadarAnalysis, &QAction::triggered, this, &MyOSGEarthQT::on_RadarAnalysis);
	connect(ui.ClearAnalysis, &QAction::triggered, this, &MyOSGEarthQT::on_ClearAnalysis);
    connect(ui.SetLosHeight, &QAction::triggered, this, &MyOSGEarthQT::on_SetLosHeight);

	m_PickEvent = new PickEvent(_lab, m_mapNode.get(), m_losGroup);
	ui.openGLWidget->getViewer()->addEventHandler(m_PickEvent);

	osg::ref_ptr<osgEarth::Util::EarthManipulator> em = dynamic_cast<osgEarth::Util::EarthManipulator*>(ui.openGLWidget->getViewer()->getCameraManipulator());
	em->setViewpoint(osgEarth::Viewpoint(NULL, 87.43, 27.18, 2060.66, -2.5, -10, 20000), 2);
}

void MyOSGEarthQT::on_VisibilityAnalysis(bool checked)
{
	
	if (checked)
	{
		ui.openGLWidget->getViewer()->getEventHandlers();
		m_PickEvent->setActionEvent(EnumActionEvent::VisibilityAnalysis);
	}
	else
	{
		m_PickEvent->setActionEvent(EnumActionEvent::ActionNull);
		ui.VisibilityAnalysis->setChecked(false);
		return;
	}
	ui.ViewshedAnalysis->setChecked(false);
	ui.RadarAnalysis->setChecked(false);
}
void MyOSGEarthQT::on_ViewshedAnalysis(bool checked)
{
	if (checked)
	{
		m_PickEvent->setActionEvent(EnumActionEvent::ViewshedAnalysis);
	}
	else
	{
		m_PickEvent->setActionEvent(EnumActionEvent::ActionNull);
		ui.ViewshedAnalysis->setChecked(false);
		return;
	}
	ui.VisibilityAnalysis->setChecked(false);
	ui.RadarAnalysis->setChecked(false);
}
void MyOSGEarthQT::on_RadarAnalysis(bool checked)
{
	
	if (checked)
	{
		m_PickEvent->setActionEvent(EnumActionEvent::RadarAnalysis);
	}
	else
	{
		m_PickEvent->setActionEvent(EnumActionEvent::ActionNull);
		ui.RadarAnalysis->setChecked(false);
		return;
	}
	ui.VisibilityAnalysis->setChecked(false);
	ui.ViewshedAnalysis->setChecked(false);
}
void MyOSGEarthQT::on_ClearAnalysis()
{
	ui.VisibilityAnalysis->setChecked(false);
	ui.ViewshedAnalysis->setChecked(false);
	ui.RadarAnalysis->setChecked(false);
	m_PickEvent->setActionEvent(EnumActionEvent::ActionNull);

	m_PickEvent->RemoveAnalysis();

}
void MyOSGEarthQT::on_SetLosHeight()
{
	sub_setLosHeight->show();
	sub_setLosHeight->move(this->pos());
}

void MyOSGEarthQT::sendLosHeight(float height)
{
	m_PickEvent->setLosHeight(height);
}
