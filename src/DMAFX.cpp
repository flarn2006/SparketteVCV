#include "plugin.hpp"
#include "DMA.hpp"
#include "Widgets.hpp"
#include <utility>
#include <functional>
#include <vector>

using namespace sparkette;

struct DMAFX : DMAExpanderModule<float, bool> {
	enum ParamId {
		SCROLL_AMOUNT_CV_PARAM,
		SCROLL_AMOUNT_PARAM,
		INVERT_PARAM,
		RAND_MAX_PARAM,
		RANDOMIZE_PARAM,
		RAND_MIN_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		SCROLL_NW_INPUT,
		SCROLL_N_INPUT,
		SCROLL_NE_INPUT,
		SCROLL_W_INPUT,
		SCROLL_E_INPUT,
		SCROLL_SW_INPUT,
		SCROLL_S_INPUT,
		SCROLL_SE_INPUT,
		SCROLL_AMOUNT_INPUT,
		ROTATE_CW_INPUT,
		ROTATE_CCW_INPUT,
		FLIP_V_INPUT,
		FLIP_H_INPUT,
		INVERT_INPUT,
		RANDOMIZE_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUTPUTS_LEN
	};
	enum LightId {
		DMA_CLIENT_LIGHT,
		DMA_HOST_LIGHT_G,
		DMA_HOST_LIGHT_R,
		LIGHTS_LEN
	};

	dsp::SchmittTrigger tr_scroll[8][PORT_MAX_CHANNELS];
	dsp::SchmittTrigger tr_rotate_cw[PORT_MAX_CHANNELS];
	dsp::SchmittTrigger tr_rotate_ccw[PORT_MAX_CHANNELS];
	dsp::SchmittTrigger tr_flip_v[PORT_MAX_CHANNELS];
	dsp::SchmittTrigger tr_flip_h[PORT_MAX_CHANNELS];
	dsp::SchmittTrigger tr_invert[PORT_MAX_CHANNELS];
	dsp::SchmittTrigger tr_random[PORT_MAX_CHANNELS];

	std::vector<float> scratch;

	void onTrigger(int input, dsp::SchmittTrigger triggers[], int dma_nchan, const std::function<void(int)> &func) {
		int nchan = inputs[input].getChannels();
		if (nchan == 1) {
			float v = inputs[input].getVoltage();
			for (int i=0; i<dma_nchan; ++i) {
				if (triggers[i].process(v))
					func(i);
			}
		} else if (nchan > 1) {
			nchan = std::min(nchan, dma_nchan);
			float voltages[PORT_MAX_CHANNELS];
			inputs[input].readVoltages(voltages);
			for (int i=0; i<nchan; ++i) {
				if (triggers[i].process(voltages[i]))
					func(i);
			}
		}
	}

	DMAFX() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(SCROLL_AMOUNT_CV_PARAM, -32.f, 32.f, 0.f, "Scroll amount CV");
		configParam(SCROLL_AMOUNT_PARAM, 1.f, 32.f, 1.f, "Scroll amount");
		paramQuantities[SCROLL_AMOUNT_PARAM]->snapEnabled = true;
		configButton(INVERT_PARAM, "Invert");
		configParam(RAND_MAX_PARAM, -10.f, 10.f, 10.f, "Max random value");
		configButton(RANDOMIZE_PARAM, "Randomize");
		configParam(RAND_MIN_PARAM, -10.f, 10.f, 0.f, "Min random value");
		configInput(SCROLL_NW_INPUT, "Scroll NW");
		configInput(SCROLL_N_INPUT, "Scroll N");
		configInput(SCROLL_NE_INPUT, "Scroll NE");
		configInput(SCROLL_W_INPUT, "Scroll W");
		configInput(SCROLL_E_INPUT, "Scroll E");
		configInput(SCROLL_SW_INPUT, "Scroll SW");
		configInput(SCROLL_S_INPUT, "Scroll S");
		configInput(SCROLL_SE_INPUT, "Scroll SE");
		configInput(SCROLL_AMOUNT_INPUT, "Scroll amount");
		configInput(ROTATE_CW_INPUT, "Rotate clockwise");
		configInput(ROTATE_CCW_INPUT, "Rotate counterclockwise");
		configInput(FLIP_V_INPUT, "Vertical flip");
		configInput(FLIP_H_INPUT, "Horizontal flip");
		configInput(INVERT_INPUT, "Invert");
		configInput(RANDOMIZE_INPUT, "Randomize");

		dmaClientLightID = DMA_CLIENT_LIGHT;
		dmaHostLightID = DMA_HOST_LIGHT_G;
	}

	template <typename T>
	void scroll(DMAChannel<T> &dma, int dx, int dy) {
		int cols = dma.width();
		int rows = dma.height();
		if (dx < 0) dx += cols;
		if (dy < 0) dy += rows;
		scratch.resize(std::max(cols, rows));

		if (dx != 0) {
			for (int y=0; y<rows; ++y) {
				for (int x=0; x<cols; ++x)
					scratch[x] = dma.read(x, y);
				for (int x=0; x<cols; ++x)
					dma.write((x + dx) % cols, y, scratch[x]);
			}
		}

		if (dy != 0) {
			for (int x=0; x<cols; ++x) {
				for (int y=0; y<rows; ++y)
					scratch[y] = dma.read(x, y);
				for (int y=0; y<rows; ++y)
					dma.write(x, (y + dy) % rows, scratch[y]);
			}
		}
	}

	constexpr void getScrollOffsets(int input, int &dx, int &dy) {
		int n = input - SCROLL_NW_INPUT;
		if (n < 3) {
			dy = -1;
			switch (n) {
				case 0: dx = -1; break;
				case 1: dx = 0; break;
				case 2: dx = 1; break;
			}
		} else if (n > 4) {
			dy = 1;
			switch (n) {
				case 5: dx = -1; break;
				case 6: dx = 0; break;
				case 7: dx = 1; break;
			}
		} else {
			dy = 0;
			switch (n) {
				case 3: dx = -1; break;
				case 4: dx = 1; break;
			}
		}
	}

	template <typename T>
	void flipV(DMAChannel<T> &dma) {
		int cols = dma.width();
		int rows = dma.height();
		for (int x=0; x<cols; ++x) {
			for (int y=0; y<rows/2; ++y) {
				T temp = dma.read(x, y);
				dma.write(x, y, dma.read(x, rows-1-y));
				dma.write(x, rows-1-y, temp);
			}
		}
	}

	template <typename T>
	void flipH(DMAChannel<T> &dma) {
		int cols = dma.width();
		int rows = dma.height();
		for (int y=0; y<rows; ++y) {
			for (int x=0; x<cols/2; ++x) {
				T temp = dma.read(x, y);
				dma.write(x, y, dma.read(cols-1-x, y));
				dma.write(cols-1-x, y, temp);
			}
		}
	}

	void process(const ProcessArgs& args) override {
		DMAExpanderModule<float, bool>::process(args);
		int dma_nchan = std::min(getDMAChannelCount(), PORT_MAX_CHANNELS);
		if (dma_nchan == 0) return;

		DMAChannel<float> *dmaF[PORT_MAX_CHANNELS];
		DMAChannel<bool> *dmaB[PORT_MAX_CHANNELS];
		for (int i=0; i<dma_nchan; ++i) {
			dmaF[i] = DMAClient<float>::getDMAChannel(i);
			dmaB[i] = DMAClient<bool>::getDMAChannel(i);
		}

		int scroll_amount_nchan = inputs[SCROLL_AMOUNT_INPUT].getChannels();
		float scroll_amount[PORT_MAX_CHANNELS];
		float amount_offset = params[SCROLL_AMOUNT_PARAM].getValue();
		if (scroll_amount_nchan) {
			inputs[SCROLL_AMOUNT_INPUT].readVoltages(scroll_amount);
			float amount_scale = params[SCROLL_AMOUNT_CV_PARAM].getValue();
			for (int i=0; i<PORT_MAX_CHANNELS; ++i) {
				scroll_amount[i] *= amount_scale / 10.f;
				scroll_amount[i] += amount_offset;
				scroll_amount[i] = std::max(-64.f, std::min(64.f, scroll_amount[i]));
			}
		} else {
			scroll_amount[0] = amount_offset;
			scroll_amount_nchan = 1;
		}

		for (int i=0; i<8; ++i) {
			int dx, dy;
			getScrollOffsets(SCROLL_NW_INPUT+i, dx, dy);
			onTrigger(SCROLL_NW_INPUT+i, tr_scroll[i], dma_nchan, [this, dx, dy, scroll_amount_nchan, &scroll_amount, &dmaF, &dmaB](int ch) {
				float amount = scroll_amount[ch % scroll_amount_nchan];
				int DX = (int)((float)dx * amount);
				int DY = (int)((float)dy * amount);
				if (dmaF[ch])
					scroll(*dmaF[ch], DX, DY);
				else if (dmaB[ch])
					scroll(*dmaB[ch], DX, DY);
			});
		}

		onTrigger(FLIP_V_INPUT, tr_flip_v, dma_nchan, [&](int ch) {
			if (dmaF[ch])
				flipV(*dmaF[ch]);
			else if (dmaB[ch])
				flipV(*dmaB[ch]);
		});

		onTrigger(FLIP_H_INPUT, tr_flip_h, dma_nchan, [&](int ch) {
			if (dmaF[ch])
				flipH(*dmaF[ch]);
			else if (dmaB[ch])
				flipH(*dmaB[ch]);
		});
		
		onTrigger(INVERT_INPUT, tr_invert, dma_nchan, [&](int ch) {
			if (dmaF[ch]) {
				DMAChannel<float> &dma = *dmaF[ch];
				std::size_t count = dma.size();
				for (std::size_t i=0; i<count; ++i)
					dma[ch] = -dma[ch];
			} else if (dmaB[ch]) {
				DMAChannel<bool> &dma = *dmaB[ch];
				std::size_t count = dma.size();
				for (std::size_t i=0; i<count; ++i)
					dma[ch] = !dma[ch];
			}
		});
	}
};


struct DMAFXWidget : ModuleWidget {
	Label *channel_disp;
	DMAFXWidget(DMAFX* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/DMAFX.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Trimpot>(mm2px(Vec(15.24, 51.55)), module, DMAFX::SCROLL_AMOUNT_CV_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(23.918, 51.55)), module, DMAFX::SCROLL_AMOUNT_PARAM));
		addParam(createParamCentered<VCVButton>(mm2px(Vec(15.293, 98.54)), module, DMAFX::INVERT_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(20.214, 106.901)), module, DMAFX::RAND_MAX_PARAM));
		addParam(createParamCentered<VCVButton>(mm2px(Vec(14.182, 109.441)), module, DMAFX::RANDOMIZE_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(20.214, 111.981)), module, DMAFX::RAND_MIN_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.35, 24.88)), module, DMAFX::SCROLL_NW_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.24, 24.88)), module, DMAFX::SCROLL_N_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(24.13, 24.88)), module, DMAFX::SCROLL_NE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.35, 33.77)), module, DMAFX::SCROLL_W_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(24.13, 33.77)), module, DMAFX::SCROLL_E_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.35, 42.66)), module, DMAFX::SCROLL_SW_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.24, 42.66)), module, DMAFX::SCROLL_S_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(24.13, 42.66)), module, DMAFX::SCROLL_SE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.562, 51.55)), module, DMAFX::SCROLL_AMOUNT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.89, 70.6)), module, DMAFX::ROTATE_CW_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.59, 70.6)), module, DMAFX::ROTATE_CCW_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(11.007, 81.289)), module, DMAFX::FLIP_V_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.59, 82.348)), module, DMAFX::FLIP_H_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.35, 98.54)), module, DMAFX::INVERT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.821, 109.441)), module, DMAFX::RANDOMIZE_INPUT));

		addChild(createLightCentered<SmallLight<BlueLight>>(Vec(8.0, 8.0), module, DMAFX::DMA_CLIENT_LIGHT));
		addChild(createLightCentered<SmallLight<GreenRedLight>>(Vec(box.size.x - 8.0, 8.0), module, DMAFX::DMA_HOST_LIGHT_G));

		channel_disp = createWidget<GlowingWidget<Label>>(mm2px(Vec(20.805, 9.381)));
		channel_disp->text = "#";
		channel_disp->box.size = mm2px(Vec(6.135, 5.08));
		channel_disp->color = componentlibrary::SCHEME_BLUE;
		channel_disp->fontSize = 12.f;
		addChild(channel_disp);
	}

	void step() override {
		ModuleWidget::step();
		if (module) {
			auto m = dynamic_cast<DMAFX*>(module);
			channel_disp->text = string::f("%d", m->getDMAChannelCount());
		}
	}
};


Model* modelDMAFX = createModel<DMAFX, DMAFXWidget>("DMAFX");
