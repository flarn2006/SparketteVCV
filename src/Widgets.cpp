#include "Widgets.hpp"

namespace sparkette {

SevenSegmentDisplay::SevenSegmentDisplay() {
	font = APP->window->loadFont(asset::plugin(pluginInstance, "res/DSEG7ClassicMini-BoldItalic.ttf"));
}

void SevenSegmentDisplay::drawPart(const DrawArgs& args, bool isBack) {
	bndSetFont(font->handle);
	if (isBack)
		bndIconLabelValue(args.vg, 0.0, 9.0, box.size.x+16.0, box.size.y, -1, offColor, BND_LEFT, 17.0, backtext.c_str(), nullptr);
	else
		bndIconLabelValue(args.vg, 0.0, 9.0, box.size.x+16.0, box.size.y, -1, color, BND_LEFT, 17.0, text.c_str(), nullptr);
	bndSetFont(APP->window->uiFont->handle);
}

void SevenSegmentDisplay::draw(const DrawArgs& args) {
	drawPart(args, true);
}

void SevenSegmentDisplay::drawLayer(const DrawArgs& args, int layer) {
	drawPart(args, false);
	TransparentWidget::drawLayer(args, layer);
}

void SevenSegmentDisplay::step() {
	TransparentWidget::step();
	if (needsUpdate) {
		text = string::f(hexMode ? "%0*x" : "%0*d", digits, value);
		needsUpdate = false;
	}
}

int SevenSegmentDisplay::getValue() const {
	return value;
}

void SevenSegmentDisplay::setValue(int newValue, int digits) {
	value = newValue;
	needsUpdate = true;
	if (digits != this->digits) {
		this->digits = digits;
		backtext = "";
		for (int i=0; i<digits; ++i)
			backtext += "8";
	}
}

void SevenSegmentDisplay::setHexMode(bool state) {
	hexMode = state;
	needsUpdate = true;
}

void SevenSegmentDisplay::setColor(const NVGcolor& newColor) {
	color = offColor = newColor;
	offColor.r /= 4;
	offColor.g /= 4;
	offColor.b /= 4;
}

}
