#pragma once
#include <osgEarthUtil/EarthManipulator>

class Compass : public osg::Camera
{
public:
	Compass(const std::string& platPath, const std::string& needlePath);
	inline void setMainCamera(osg::Camera* camera) {_mainCamera = camera;}
	virtual void traverse(osg::NodeVisitor& nv);

private:
	osg::MatrixTransform* createCompassPart(const std::string& image, float radius, float height);
	osg::ref_ptr<osg::MatrixTransform> _plateTransform;
	osg::ref_ptr<osg::MatrixTransform> _needleTransform;
	osg::observer_ptr<osg::Camera> _mainCamera;
};