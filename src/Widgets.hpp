#pragma once
#include "plugin.hpp"

namespace sparkette {

template <typename TWidget>
struct GlowingWidget : TWidget {
	void drawLayer(const typename TWidget::DrawArgs& args, int layer) override {
		if (layer == 1)
			static_cast<TWidget*>(this)->draw(args);
		TWidget::drawLayer(args, layer);
	}
};

struct CKSSWithLine : app::SvgSwitch {
	CKSSWithLine();
};

template <typename TSwitch, bool Momentary = true>
struct MomentaryVariant : TSwitch {
	MomentaryVariant() {
		this->momentary = Momentary;
	}
};

}
