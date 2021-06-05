#include <OgreRoot.h>
#include <OgreRenderWindow.h>
#include <OgreViewport.h>
#include <OgreRenderTexture.h>
#include <OgreTextureManager.h>
#include <OgreEntity.h>
#include <OgreApplicationContextBase.h>
#include <OgrePrerequisites.h>
#include <OgreMeshManager.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreWindowEventUtilities.h>
#include <OgreMeshSerializer.h>

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

void loadLocalMesh(const std::string& name, const std::string& path) {
	std::ifstream file (path.c_str(), std::ifstream::in | std::ifstream::binary);
	if (!file) {
		OGRE_EXCEPT(Ogre::Exception::ERR_FILE_NOT_FOUND, "File " + path + " not found.", "LocalMeshLoader");
	}

	Ogre::DataStreamPtr stream (OGRE_NEW Ogre::FileStreamDataStream (&file, false));
	Ogre::MeshPtr mesh = Ogre::MeshManager::getSingleton().createManual(name, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

	Ogre::MeshSerializer meshSerializer;
	meshSerializer.importMesh(stream, mesh.get());
}

struct SingleImageConfiguration {
	const std::string inputMesh = "../fish.mesh";
	const std::string outputPath = "../rendered.png";
};

struct ProcessConfigurator {
	std::vector<SingleImageConfiguration> imagesConfigurations;
};

void create(const ProcessConfigurator& configurator) {
	auto app = SyntheticImageGenerator();
	app.initApp();

	// get a pointer to the already created root
	Ogre::Root *root = app.getRoot();
	Ogre::SceneManager *scnMgr = root->createSceneManager();

	// register our scene with the RTSS
	Ogre::RTShader::ShaderGenerator *shadergen = Ogre::RTShader::ShaderGenerator::getSingletonPtr();
	shadergen->addSceneManager(scnMgr);

	// without light we would just get a black screen
	Ogre::Light *light = scnMgr->createLight("MainLight");
	Ogre::SceneNode *lightNode = scnMgr->getRootSceneNode()->createChildSceneNode();
	lightNode->setPosition(0, 10, 15);
	lightNode->attachObject(light);

	// also need to tell where we are
	Ogre::SceneNode *camNode = scnMgr->getRootSceneNode()->createChildSceneNode();
	camNode->setPosition(0, 0, 15);
	camNode->lookAt(Ogre::Vector3(0, 0, -1), Ogre::Node::TS_PARENT);

	// create the camera
	Ogre::Camera *cam = scnMgr->createCamera("myCam");
	cam->setNearClipDistance(5); // specific to this sample
	cam->setAutoAspectRatio(true);
	camNode->attachObject(cam);

	// finally something to render
	Ogre::SceneNode *node = scnMgr->getRootSceneNode()->createChildSceneNode();

	Ogre::TexturePtr rttTexture = Ogre::TextureManager::getSingleton().createManual(
		"RttTex",
		Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		Ogre::TEX_TYPE_2D,
		600, 600,
		0,
		Ogre::PF_R8G8B8,
		Ogre::TU_RENDERTARGET);

	Ogre::RenderTexture *renderTexture = rttTexture->getBuffer()->getRenderTarget();

	renderTexture->addViewport(cam);
	renderTexture->getViewport(0)->setClearEveryFrame(true);
	renderTexture->getViewport(0)->setBackgroundColour(Ogre::ColourValue::Black);
	renderTexture->getViewport(0)->setOverlaysEnabled(false);

	for(const auto& image : configurator.imagesConfigurations) {
		auto meshName = image.inputMesh;

		if(!Ogre::MeshManager::getSingleton().resourceExists(meshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME)) {
			loadLocalMesh(meshName, image.inputMesh);
		}

		Ogre::Entity *ent = scnMgr->createEntity(meshName);
		node->attachObject(ent);

		renderTexture->update();

		renderTexture->writeContentsToFile(image.outputPath);

		node->detachObject(ent);
		scnMgr->destroyEntity(ent);
	}

	app.closeApp();
}

int main() {
	ProcessConfigurator configurator;
	configurator.imagesConfigurations.emplace_back(SingleImageConfiguration {
		.inputMesh = "../fish.mesh",
		.outputPath = "../rendered.png"
	});
	configurator.imagesConfigurations.emplace_back(SingleImageConfiguration {
		.inputMesh = "../Sword.mesh",
		.outputPath = "../rendered2.png"
	});
	configurator.imagesConfigurations.emplace_back(SingleImageConfiguration {
		.inputMesh = "../Sinbad.mesh",
		.outputPath = "../rendered3.png"
	});

	create(configurator);
	return 0;
}