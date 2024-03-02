#pragma once
#include <rack.hpp>


using namespace rack;

// Declare the Plugin, defined in plugin.cpp
extern Plugin* pluginInstance;

// Declare each Model, defined in each module source file
// extern Model* modelMyModule;
extern Model* modelEllie;
extern Model* modelNoteClassifier;
extern Model* modelPolyRepeater;
extern Model* modelPolySelector;
extern Model* modelRGBMatrix16;
extern Model* modelRGBMatrix;
extern Model* modelRGBMatrix64;
extern Model* modelHSV2RGB;
extern Model* modelFunctions;
extern Model* modelPolyCat;
extern Model* modelIntegrator;
