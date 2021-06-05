#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>

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
#include <OgreVector.h>
#include <OgreMeshSerializer.h>

#include <random>

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
	const Ogre::uint32 width;
	const Ogre::uint32 height;
	const std::function< Ogre::Vector3 (const Ogre::Vector3& point) > randomnessProviderPosition = [] (const Ogre::Vector3& point) { return point; };
	const std::function< Ogre::Radian () > randomnessProviderRotation = [] () { return Ogre::Radian(); };
	const std::function< cv::Mat (cv::Mat& image) > postProcessing = [] (cv::Mat& image) { return image; };
};

struct ProcessConfigurator {
	std::vector<SingleImageConfiguration> imagesConfigurations;
};

cv::Mat getTextureMat(const Ogre::uint32 width, const Ogre::uint32 height, Ogre::RenderTexture *texture) {
	Ogre::PixelFormat format = Ogre::PF_BYTE_BGRA;
	size_t outBytesPerPixel = Ogre::PixelUtil::getNumElemBytes(format);
	auto data = new unsigned char [width*height*outBytesPerPixel];

	auto sourceBox = Ogre::Box(0, 0, texture->getWidth(), texture->getHeight());
	auto destinationBox = Ogre::PixelBox(Ogre::Box(0, 0, width, height), format, data);
	auto mat = cv::Mat(height, width, CV_8UC4, data);

	texture->copyContentsToMemory(sourceBox, destinationBox);

	return mat;
}

void create(const ProcessConfigurator& configurator) {
	const auto initialCameraPosition = Ogre::Vector3(0, 0, 15);
	const auto lookAtVector = Ogre::Vector3(0, 0, -1);

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
	camNode->setPosition(initialCameraPosition);
	camNode->lookAt(lookAtVector, Ogre::Node::TS_PARENT);

	// create the camera
	Ogre::Camera *cam = scnMgr->createCamera("myCam");
	cam->setNearClipDistance(5); // specific to this sample
	cam->setAutoAspectRatio(true);
	camNode->attachObject(cam);

	// finally something to render
	Ogre::SceneNode *node = scnMgr->getRootSceneNode()->createChildSceneNode();

	Ogre::TexturePtr rttTexture;
	Ogre::RenderTexture *renderTexture;

	for(const auto& image : configurator.imagesConfigurations) {
		bool rttChanged = false;

		rttChanged = rttTexture.get() == nullptr;

		if(!rttChanged && rttTexture->getWidth() != image.width) {
			rttTexture->setWidth(image.width);
			rttChanged = true;
		}

		if(!rttChanged && rttTexture->getHeight() != image.height) {
			rttTexture->setWidth(image.height);
			rttChanged = true;
		}

		if(rttChanged) {
			if(rttTexture.get() != nullptr) {
				Ogre::TextureManager::getSingleton().remove(rttTexture);
			}

			rttTexture = Ogre::TextureManager::getSingleton().createManual(
				"RttTex",
				Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
				Ogre::TEX_TYPE_2D,
				image.width, image.height,
				0,
				Ogre::PF_R8G8B8A8,
				Ogre::TU_RENDERTARGET);

			renderTexture = rttTexture->getBuffer()->getRenderTarget();

			renderTexture->addViewport(cam);
			renderTexture->getViewport(0)->setClearEveryFrame(true);
			renderTexture->getViewport(0)->setBackgroundColour(Ogre::ColourValue(0, 0, 0, 0));
			renderTexture->getViewport(0)->setOverlaysEnabled(false);
		}

		auto meshName = image.inputMesh;

		if(!Ogre::MeshManager::getSingleton().resourceExists(meshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME)) {
			loadLocalMesh(meshName, image.inputMesh);
		}

		auto newPos = image.randomnessProviderPosition(camNode->getPosition());
		camNode->setPosition(newPos);

		auto newRot = image.randomnessProviderRotation();
		camNode->rotate(camNode->getPosition() - lookAtVector, newRot);

		camNode->lookAt(lookAtVector, Ogre::Node::TS_PARENT);

		Ogre::Entity *ent = scnMgr->createEntity(meshName);
		node->attachObject(ent);


		renderTexture->update();
		auto mat = getTextureMat(image.width, image.height, renderTexture);
		cv::imwrite(image.outputPath, image.postProcessing(mat));

		node->detachObject(ent);
		scnMgr->destroyEntity(ent);

		camNode->setPosition(initialCameraPosition);
		camNode->resetOrientation();
		camNode->lookAt(lookAtVector, Ogre::Node::TS_PARENT);
	}

	app.closeApp();
}

int main() {

	std::default_random_engine generator(42);

	ProcessConfigurator configurator;
	configurator.imagesConfigurations.emplace_back(SingleImageConfiguration {
		.inputMesh = "../fish.mesh",
		.outputPath = "../rendered.png",
		.width = 250,
		.height = 250,
		.randomnessProviderPosition = [&generator] (const Ogre::Vector3& point) {
			std::normal_distribution<Ogre::Real> distributionX(point.x,25.0);
			std::normal_distribution<Ogre::Real> distributionY(point.y,5.0);
			std::normal_distribution<Ogre::Real> distributionZ(point.z,5.0);
			return Ogre::Vector3(
				distributionX(generator),
				distributionY(generator),
				distributionZ(generator)
			);
		},
		.randomnessProviderRotation = [&generator] () {
			std::normal_distribution<Ogre::Real> distributionAngle(0, 10);
			return Ogre::Degree(distributionAngle(generator));
		},
		.postProcessing = [] (cv::Mat& image) {
			cv::Mat mask;
			inRange(image, cv::Scalar(0, 0, 0, 0), cv::Scalar(255, 255, 255, 0), mask);
			image.setTo(cv::Scalar(0, 0, 0, 255), mask);
			return image;
		},
	});
	configurator.imagesConfigurations.emplace_back(SingleImageConfiguration {
		.inputMesh = "../fish.mesh",
		.outputPath = "../rendered2.png",
		.width = 300,
		.height = 300,
	});
	configurator.imagesConfigurations.emplace_back(SingleImageConfiguration {
		.inputMesh = "../Sinbad.mesh",
		.outputPath = "../rendered3.png",
		.width = 250,
		.height = 250,
		.postProcessing = [] (cv::Mat& image) {
			cv::Mat mask;
			inRange(image, cv::Scalar(0, 0, 0, 0), cv::Scalar(255, 255, 255, 0), mask);
			image.setTo(cv::Scalar(0, 0, 255, 255), mask);
			return image;
		},
	});

	create(configurator);
	return 0;
}