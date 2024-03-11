#pragma once
#include "plugin.hpp"

namespace sparkette {

	float applyScaleOffset(float voltage, rack::engine::Param& scale, rack::engine::Param& offset);
	void applyPolyScaleOffset(float* voltages, int nchan, rack::engine::Param& scale, rack::engine::Param& offset);
	void hsvToRgb(float h, float s, float v, float& r, float& g, float& b);

}
