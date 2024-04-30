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
		r += m; g += m; b += m;
	}

	int transform2DByMatrixInput(int nchan, const float* channels, float& x, float& y) {
		if (nchan >= 6) {
			float xt = x * channels[0] + y * channels[1] + channels[2];
			y *= channels[4];
			y += x * channels[3] + channels[5];
			x = xt;
			return 1 | ((nchan > 6) << 1);
		} else if (nchan >= 4) {
			float xt = x * channels[0] + y * channels[1];
			y *= channels[3];
			y += x * channels[2];
			x = xt;
			return 1 | ((nchan > 4) << 1);
		} else {
			return 2;
		}
	}

	int transform2DByMatrixInput(Input& input, float& x, float& y) {
		int nchan = input.getChannels();
		if (nchan > 0) {
			float channels[PORT_MAX_CHANNELS];
			input.readVoltages(channels);
			return transform2DByMatrixInput(nchan, channels, x, y);
		} else {
			return 0;
		}
	}

	int transform2DByMatrixInput(int matrix_nchan, const float* matrix_channels, std::size_t nchan, float* x, float* y) {
		int result = 0;
		for (std::size_t i=0; i<nchan; ++i)
			result = transform2DByMatrixInput(matrix_nchan, matrix_channels, x[i], y[i]);
		return result;
	}

	int transform2DByMatrixInput(Input& input, std::size_t nchan, float* x, float* y) {
		int matrix_nchan = input.getChannels();
		float channels[PORT_MAX_CHANNELS];
		input.readVoltages(channels);
		return transform2DByMatrixInput(matrix_nchan, channels, nchan, x, y);
	}

	void fillAddressArray(int xoff, int yoff, int x_nchan, int y_nchan, const float *x_array, const float *y_array, int *addresses, int poly_increment, int matrix_width, int matrix_height) {
		for (int i=0; i<PORT_MAX_CHANNELS; ++i) {
			int x = xoff, y = yoff;
			if (i < y_nchan) {
				y += y_array[i] / 10 * matrix_height;
				if (i < x_nchan)
					x += (int)(x_array[i] / 10 * matrix_width);
				addresses[i] = matrix_width * y + x;
			} else if (i < x_nchan) {
				x += (int)(x_array[i] / 10 * (matrix_width*matrix_height-1));
				addresses[i] = matrix_width * y + x;
			} else if (i > 0) {
				addresses[i] = (addresses[i-1] + poly_increment) % (matrix_width*matrix_height);
			} else {
				addresses[i] = matrix_width * y + x;
			}
			if (addresses[i] < 0)
				addresses[i] = 0;
			else
				addresses[i] %= matrix_width*matrix_height;
		}
	}
}
