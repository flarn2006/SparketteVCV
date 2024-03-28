#pragma once
#include "plugin.hpp"

namespace sparkette {

struct GlowingLabel : Label {
	void drawLayer(const DrawArgs& args, int layer) override;
};

class SevenSegmentDisplay : public TransparentWidget {

	bool needsUpdate = false;
	int value, digits;
	std::string text, backtext;
	bool hexMode = false;
	std::shared_ptr<Font> font;

	void drawPart(const DrawArgs& args, bool isBack);

public:
	NVGcolor color;
	NVGcolor offColor;

	SevenSegmentDisplay();
	void draw(const DrawArgs& args) override;
	void drawLayer(const DrawArgs& args, int layer) override;
	void step() override;
	int getValue() const;
	void setValue(int newValue, int digits);
	void setHexMode(bool state);
	void setColor(const NVGcolor& newColor);

};

}
