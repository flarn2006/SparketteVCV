#pragma once
#include "plugin.hpp"

namespace sparkette {

	float applyScaleOffset(float voltage, rack::engine::Param& scale, rack::engine::Param& offset);

}
