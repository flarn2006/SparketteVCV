#pragma once
#include "plugin.hpp"
#include <string>
#include <vector>
#include <cstdarg>

namespace sparkette {

	float applyScaleOffset(float voltage, rack::engine::Param& scale, rack::engine::Param& offset);
	void applyPolyScaleOffset(float* voltages, int nchan, rack::engine::Param& scale, rack::engine::Param& offset);
	void hsvToRgb(float h, float s, float v, float& r, float& g, float& b);
	int transform2DByMatrixInput(int nchan, const float* channels, float& x, float& y);
	int transform2DByMatrixInput(Input& input, float& x, float& y);
	int transform2DByMatrixInput(int matrix_nchan, const float* matrix_channels, std::size_t nchan, float* x, float* y);
	int transform2DByMatrixInput(Input& input, std::size_t nchan, float* x, float* y);

	template <class TSwitchQuantity = SwitchQuantity>
	TSwitchQuantity* configSwitchWithLabels(Module* module, int paramId, float minValue, float maxValue, float defaultValue, const std::string& name, ...) {
		va_list args;
		const char* arg;
		std::vector<std::string> labels;

		va_start(args, name);
		while ((arg = va_arg(args, const char*)))
			labels.emplace_back(arg);
		va_end(args);

		return module->configSwitch<TSwitchQuantity>(paramId, minValue, maxValue, defaultValue, name, labels);
	}

	template <typename TFirst, typename... TRest>
	struct FirstParameter {
		typedef TFirst type;
	};

}
