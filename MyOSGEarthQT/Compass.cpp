#include "Compass.h"
#include <osg/MatrixTransform>
#include <osg/Texture2D>
#include <osgEarth/ShaderGenerator>

Compass::Compass(const std::string& platPath, const std::string& needlePath) :
	_plateTransform(nullptr),
	_needleTransform(nullptr),
	_mainCamera(nullptr)
{
	setViewport(0.0, 0.0, 128, 128);
	setProjectionMatrix(osg::Matrixd::ortho(-1.5, 1.5, -1.5, 1.5, -10.0, 10.0));

	setRenderOrder(osg::Camera::POST_RENDER);
	setClearMask(GL_DEPTH_BUFFER_BIT);
	setAllowEventFocus(false);
	setReferenceFrame(osg::Transform::ABSOLUTE_RF);
	getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
	getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);

	_plateTransform = createCompassPart(platPath, 1.5f, -1.0f);
	_needleTransform = createCompassPart(needlePath, 1.5f, 0.0f);

	osgEarth::ShaderGenerator SG;
	_plateTransform->accept(SG);
	_needleTransform->accept(SG);
}

void Compass::traverse(osg::NodeVisitor& nv)
{
	if (_mainCamera.valid() && nv.getVisitorType() == osg::NodeVisitor::CULL_VISITOR)
	{
		osg::Matrix matrix = _mainCamera->getViewMatrix();
		matrix.setTrans(osg::Vec3());

		osg::Vec3 northVec = osg::Z_AXIS * matrix;
		northVec.z() = 0.0f;
		northVec.normalize();
		osg::Vec3 axis = osg::Y_AXIS ^ northVec;
		float angle = atan2(axis.length(), osg::Y_AXIS*northVec);
		axis.normalize();

		if (_plateTransform.valid())
		{
			_plateTransform->setMatrix(osg::Matrix::rotate(angle, axis));
		}
	}
	_plateTransform->accept(nv);
	_needleTransform->accept(nv);
	osg::Camera::traverse(nv);
}

osg::MatrixTransform* Compass::createCompassPart(const std::string& image, float radius, float height)
{
	osg::Vec3 center(-radius, -radius, height);
	osg::ref_ptr<osg::Geode> geode = new osg::Geode;
	geode->addDrawable(createTexturedQuadGeometry(center, osg::Vec3(radius*2.0f, 0.0f,0.0f),osg::Vec3(0.0f, radius*2.0f, 0.0f)));
	osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
	texture->setImage(osgDB::readImageFile(image));
	osg::ref_ptr<osg::MatrixTransform> part = new osg::MatrixTransform;
	part->getOrCreateStateSet()->setTextureAttributeAndModes(0, texture.get());
	part->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
	part->addChild(geode.get());
	return part.release();
}
