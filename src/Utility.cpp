#include "Utility.hpp"

namespace sparkette {

	float applyScaleOffset(float voltage, rack::engine::Param& scale, rack::engine::Param& offset) {
		float cv = voltage / 10;
		float sc = scale.getValue();
		float of = offset.getValue();
		return of + sc * cv;
	}

	void applyPolyScaleOffset(float* voltages, int nchan, rack::engine::Param& scale, rack::engine::Param& offset) {
		float sc = scale.getValue();
		float of = offset.getValue();
		for (int i=0; i<nchan; ++i) {
			float cv = voltages[i] / 10;
			voltages[i] = of + sc * cv;
		}
	}

	void hsvToRgb(float h, float s, float v, float& r, float& g, float& b) {
		r = g = b = 0;
		int sector = (int)(h * 6) % 6;
		float c = v * s;
		float x = c * (1 - std::fabs(std::fmod(h * 6, 2) - 1));
		float m = v - c;

		switch (sector) {
			case 0: r=c; g=x; break;
			case 1: g=c; r=x; break;
			case 2: g=c; b=x; break;
			case 3: b=c; g=x; break;
			case 4: b=c; r=x; break;
			case 5: r=c; b=x; break;
		}
		r = (r + m) * 10;
		g = (g + m) * 10;
		b = (b + m) * 10;
	}

}
