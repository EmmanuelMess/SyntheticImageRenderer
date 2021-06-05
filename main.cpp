#include <OgreRoot.h>
#include <OgreRenderWindow.h>
#include <OgreViewport.h>
#include <OgreRenderTexture.h>
#include <OgreTextureManager.h>
#include <OgreEntity.h>
#include <OgreApplicationContextBase.h>
#include <OgrePrerequisites.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreWindowEventUtilities.h>

class SyntheticImageGenerator: public OgreBites::ApplicationContextBase {
public:
	SyntheticImageGenerator() : OgreBites::ApplicationContextBase("SyntheticImageGenerator") {};

	void setup() override;
};

void SyntheticImageGenerator::setup()
{
	mRoot->initialise(false);

	createWindow(mAppName, 0, 0, Ogre::NameValuePairList {{"hidden", "true"}});

	locateResources();
	initialiseRTShaderSystem();
	loadResources();

	// adds context as listener to process context-level (above the sample level) events
	mRoot->addFrameListener(this);
}

int main() {
	auto app = SyntheticImageGenerator();
	app.initApp();

	// get a pointer to the already created root
	Ogre::Root* root = app.getRoot();
	Ogre::SceneManager* scnMgr = root->createSceneManager();

	// register our scene with the RTSS
	Ogre::RTShader::ShaderGenerator* shadergen = Ogre::RTShader::ShaderGenerator::getSingletonPtr();
	shadergen->addSceneManager(scnMgr);

	// without light we would just get a black screen
	Ogre::Light* light = scnMgr->createLight("MainLight");
	Ogre::SceneNode* lightNode = scnMgr->getRootSceneNode()->createChildSceneNode();
	lightNode->setPosition(0, 10, 15);
	lightNode->attachObject(light);

	// also need to tell where we are
	Ogre::SceneNode* camNode = scnMgr->getRootSceneNode()->createChildSceneNode();
	camNode->setPosition(0, 0, 15);
	camNode->lookAt(Ogre::Vector3(0, 0, -1), Ogre::Node::TS_PARENT);

	// create the camera
	Ogre::Camera* cam = scnMgr->createCamera("myCam");
	cam->setNearClipDistance(5); // specific to this sample
	cam->setAutoAspectRatio(true);
	camNode->attachObject(cam);

	// finally something to render
	Ogre::Entity* ent = scnMgr->createEntity("Sinbad.mesh");
	Ogre::SceneNode* node = scnMgr->getRootSceneNode()->createChildSceneNode();
	node->attachObject(ent);

	Ogre::TexturePtr rttTexture = Ogre::TextureManager::getSingleton().createManual(
			"RttTex",
			Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
			Ogre::TEX_TYPE_2D,
			600, 600,
			0,
			Ogre::PF_R8G8B8,
			Ogre::TU_RENDERTARGET);

	Ogre::RenderTexture* renderTexture = rttTexture->getBuffer()->getRenderTarget();

	renderTexture->addViewport(cam);
	renderTexture->getViewport(0)->setClearEveryFrame(true);
	renderTexture->getViewport(0)->setBackgroundColour(Ogre::ColourValue::Black);
	renderTexture->getViewport(0)->setOverlaysEnabled(false);

	renderTexture->update();

	renderTexture->writeContentsToFile("../rendered.png");

	app.closeApp();
	return 0;
}