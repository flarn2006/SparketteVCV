#pragma once
#include "plugin.hpp"

namespace sparkette {

template <typename TWidget>
struct GlowingWidget : TWidget {
	void drawLayer(const typename TWidget::DrawArgs& args, int layer) override {
		static_cast<TWidget*>(this)->draw(args);
	}
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
