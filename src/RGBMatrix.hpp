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

struct RGBMatrixRAXMessage {
	bool active;
	float x, y, r, g, b;
};

}
