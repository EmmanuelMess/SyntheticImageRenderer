# SyntheticImageRenderer
A synthetic image renderer made with Ogre

The code in main generates these files, without opening any windows:
<img src="rendered.png"/> <img src="rendered2.png"/> <img src="rendered3.png"/>

To change the configuration for any image use:
```c++
	ProcessConfigurator configurator;
	configurator.imagesConfigurations.emplace_back(SingleImageConfiguration {
		.inputMesh = "../fish.mesh", //path to mesh
		.outputPath = "../rendered.png" //image name
	});
```