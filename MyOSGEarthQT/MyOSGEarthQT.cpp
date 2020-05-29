#include "MyOSGEarthQT.h"
#include <QLabel>
#include <osg/Depth>

//===============================================================================


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

//===============================================================================

MyOSGEarthQT::MyOSGEarthQT(QWidget *parent)
	: QMainWindow(parent)
{
	m_pDrawProfile = nullptr;
	ui.setupUi(this);
	//setWindowState(Qt::WindowMinimized);
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
	//osgEarth::Util::Controls::ControlCanvas* canvas = new osgEarth::Util::Controls::ControlCanvas();
	//m_world->addChild(canvas);
	//canvas->setNodeMask(0x1 << 1);

	//osgEarth::Util::Controls::Grid* grid = new osgEarth::Util::Controls::Grid();
	//grid->setBackColor(0, 0, 0, 0.2);
	//grid->setAbsorbEvents(true);
	//grid->setVertAlign(osgEarth::Util::Controls::Control::ALIGN_TOP);

	//// 左上角 UI 中的 lab
	//auto mouseLabel = new osgEarth::Util::Controls::LabelControl();
	//grid->setControl(0, 0, new osgEarth::Util::Controls::LabelControl("Mouse:"));
	//grid->setControl(1, 0, mouseLabel);

	//double backgroundWidth = 500;
	//double backgroundHeight = 500;

	//double graphWidth = 200;
	//double graphHeight = 100;

	//// Add the hud
	//m_hud = createHud(backgroundWidth, backgroundHeight);
	//m_world->addChild(m_hud);

	//osg::ref_ptr< osgEarth::Util::TerrainProfileCalculator > calculator = new osgEarth::Util::TerrainProfileCalculator(m_mapNode,
	//	osgEarth::GeoPoint(m_mapNode->getMapSRS(), -124.0, 40.0),
	//	osgEarth::GeoPoint(m_mapNode->getMapSRS(), -75.1, 39.2)
	//);

	//osg::Group* profileNode = new TerrainProfileGraph(calculator.get(), graphWidth, graphHeight);
	//m_hud->addChild(profileNode);

	//m_pDrawProfile = new DrawProfileEventHandler(m_mapNode, m_mapNode, calculator.get());
	//ui.openGLWidget->getViewer()->addEventHandler(m_pDrawProfile);

	//ui.openGLWidget->getViewer()->addEventHandler(new osgEarth::Util::MouseCoordsTool(m_mapNode, mouseLabel));

	//canvas->addControl(grid);

	ui.openGLWidget->getViewer()->setSceneData(m_world.get());

	connect(ui.VisibilityAnalysis, &QAction::triggered, this, &MyOSGEarthQT::on_VisibilityAnalysis);
	connect(ui.ViewshedAnalysis, &QAction::triggered, this, &MyOSGEarthQT::on_ViewshedAnalysis);
	connect(ui.RadarAnalysis, &QAction::triggered, this, &MyOSGEarthQT::on_RadarAnalysis);
	connect(ui.ClearAnalysis, &QAction::triggered, this, &MyOSGEarthQT::on_ClearAnalysis);
    connect(ui.SetLosHeight, &QAction::triggered, this, &MyOSGEarthQT::on_SetLosHeight);

	m_PickEvent = new PickEvent(_lab, m_mapNode, m_losGroup);
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

	ui.openGLWidget->getViewer()->removeEventHandler(m_pDrawProfile);
	m_world->removeChild(m_hud);

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
