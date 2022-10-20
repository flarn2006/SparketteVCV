#pragma once

namespace sparkette {

template <typename TBase = GrayModuleLightWidget>
struct TTrueRGBLight : TBase {
	TTrueRGBLight() {
		this->addBaseColor(color::RED);
		this->addBaseColor(color::GREEN);
		this->addBaseColor(color::BLUE);
	}
};
using TrueRGBLight = TTrueRGBLight<>;

template <typename TLight = SmallLight<TrueRGBLight>>
Widget* addLightMatrix(Vec topLeft, Vec size, Module* module, int firstID, int width, int height) {
	double x_increment = size.x / (width - 1);
	double y_increment = size.y / (height - 1);

	auto widget = new Widget;
	for (int y=0; y<height; ++y) {
		for (int x=0; x<width; ++x) {
			auto light = createLightCentered<TLight>( topLeft + Vec(x_increment*x, y_increment*y), module, firstID);
			firstID += light->getNumColors();
			widget->addChild(light);
		}
	}
	return widget;
}

}
