#include "MyOSGEarthQT.h"
#include "Compass.h"

#include <osg/Depth>
#include <osgEarthUtil/MouseCoordsTool>
#include <osgEarthUtil/Controls>

MyOSGEarthQT::MyOSGEarthQT(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	setWindowState(Qt::WindowActive);

	// 初始化设置视高子窗口
	sub_setLosHeight = new SetLosHeight(this);
	sub_viewshedPara = new ViewshedPara(this);

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

    m_mapNode = osgEarth::MapNode::findMapNode(osgDB::readNodeFile("../data/myWorld.earth"));
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

	// 创建部分UI
	osgEarth::Util::ControlCanvas* canvas = osgEarth::Util::ControlCanvas::getOrCreate(ui.openGLWidget->getViewer());
	osgEarth::Util::LabelControl* readout = new osgEarth::Util::LabelControl();
	readout->setBackColor(osgEarth::Color(osgEarth::Color::Black, 0.8));
	readout->setHorizAlign(osgEarth::Util::Control::ALIGN_RIGHT);
	readout->setVertAlign(osgEarth::Util::Control::ALIGN_BOTTOM);

	osgEarth::Util::MouseCoordsTool* mcTool = new osgEarth::Util::MouseCoordsTool(m_mapNode);
	mcTool->addCallback(new osgEarth::Util::MouseCoordsLabelCallback(readout));
	ui.openGLWidget->getViewer()->addEventHandler(mcTool);
	canvas->addControl(readout);
	m_world->addChild(canvas);

	ui.openGLWidget->getViewer()->setSceneData(m_world.get());

	connect(ui.VisibilityAnalysis, &QAction::triggered, this, &MyOSGEarthQT::on_VisibilityAnalysis);
	connect(ui.ViewshedAnalysis, &QAction::triggered, this, &MyOSGEarthQT::on_ViewshedAnalysis);
	connect(ui.RadarAnalysis, &QAction::triggered, this, &MyOSGEarthQT::on_RadarAnalysis);
	connect(ui.ClearAnalysis, &QAction::triggered, this, &MyOSGEarthQT::on_ClearAnalysis);
    connect(ui.SetLosHeight, &QAction::triggered, this, &MyOSGEarthQT::on_SetLosHeight);
	connect(ui.ViewshedPara, &QAction::triggered, this, &MyOSGEarthQT::on_SetViewshedPara);

	m_PickEvent = new PickEvent(_lab, m_mapNode, m_losGroup);
	ui.openGLWidget->getViewer()->addEventHandler(m_PickEvent);

	// 设置视点 测试用
	//osg::ref_ptr<osgEarth::Util::EarthManipulator> em = dynamic_cast<osgEarth::Util::EarthManipulator*>(ui.openGLWidget->getViewer()->getCameraManipulator());
	//em->setViewpoint(osgEarth::Viewpoint(NULL, 87.43, 27.18, 2060.66, -2.5, -10, 20000), 2);

	// 添加一个指北针
	osg::ref_ptr<Compass> compass = new Compass("../data/Compass/plate.png", "../data/Compass/needle.png");
	compass->setMainCamera(ui.openGLWidget->getViewer()->getCamera());
	m_world->addChild(compass.get());

}

MyOSGEarthQT::~MyOSGEarthQT()
{

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

void MyOSGEarthQT::on_SetViewshedPara()
{
	sub_viewshedPara->show();
	sub_viewshedPara->move(this->pos());
}

void MyOSGEarthQT::sendViewshedPara(int numSpokes, int numSegment)
{
	m_PickEvent->setViewshedPara(numSpokes, numSegment);
}
