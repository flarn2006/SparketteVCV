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

	template <typename TFirst, typename... TRest>
	struct FirstParameter {
		typedef TFirst type;
	};

	void fillAddressArray(int xoff, int yoff, int x_nchan, int y_nchan, const float *x_array, const float *y_array, int *addresses, int poly_increment, int matrix_width, int matrix_height);

}
