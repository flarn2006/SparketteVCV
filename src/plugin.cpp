#include "plugin.hpp"


Plugin* pluginInstance;


void init(Plugin* p) {
	pluginInstance = p;

	// Add modules here
	// p->addModel(modelMyModule);
	p->addModel(modelEllie);
	p->addModel(modelNoteClassifier);
	p->addModel(modelPolyRepeater);
	p->addModel(modelPolySelector);
	p->addModel(modelRGBMatrix16);
	p->addModel(modelRGBMatrix);
	p->addModel(modelRGBMatrix64);
	p->addModel(modelHSV2RGB);
	p->addModel(modelFunctions);
	p->addModel(modelPolyCat);
	p->addModel(modelIntegrator);
	p->addModel(modelColorMixer);
	p->addModel(modelBusybox);

	// Any other plugin initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables when your module is created to reduce startup times of Rack.
}
