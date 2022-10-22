#include "Utility.hpp"

namespace sparkette {

	float applyScaleOffset(float voltage, rack::engine::Param& scale, rack::engine::Param& offset) {
		float cv = voltage / 10;
		float sc = scale.getValue();
		float of = offset.getValue();
		return of + sc * cv;
	}

}
