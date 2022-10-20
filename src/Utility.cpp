#include "Utility.hpp"

namespace sparkette {

	float applyScaleOffset(rack::engine::Input& input, rack::engine::Param& scale, rack::engine::Param& offset) {
		float cv = input.getVoltage() / 10;
		float sc = scale.getValue();
		float of = offset.getValue();
		return of + sc * cv;
	}

}
