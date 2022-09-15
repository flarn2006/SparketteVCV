#include "plugin.hpp"


Plugin* pluginInstance;


void init(Plugin* p) {
	pluginInstance = p;

	// Add modules here
	// p->addModel(modelMyModule);
	p->addModel(modelEllie);
	p->addModel(modelNoteClassifier);
	p->addModel(modelPolyRepeater);
	p->addModel(modelFastClock);
	p->addModel(modelLEDMatrix);

	// Any other plugin initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables when your module is created to reduce startup times of Rack.
}
