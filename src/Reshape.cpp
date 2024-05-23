#include "plugin.hpp"
#include "DMA.hpp"
#include "Segments.hpp"
#include <vector>
#include <string>

using namespace sparkette;

struct ReshapeDisplay : TransparentWidget {
	const DMAChannelBase *const *pDataSource = nullptr;

	void drawLayer(const DrawArgs &args, int layer) override {
		const double fontsize = 8.0;
		if (layer == 1) {
			const auto color = componentlibrary::SCHEME_BLUE;
			if (pDataSource && *pDataSource) {
				bndIconLabelValue(args.vg, 0.0, 0.0, box.size.x, box.size.y, -1, color, BND_CENTER, fontsize, string::f("%zu", (*pDataSource)->width()).c_str(), nullptr);
			} else {
				bndIconLabelValue(args.vg, 0.0, 0.0, box.size.x, box.size.y, -1, componentlibrary::SCHEME_RED, BND_CENTER, fontsize, pDataSource ? "CHANNEL\nNOT\nFOUND" : "NO HOST\nCONNECTED", nullptr);
			}
		}
		TransparentWidget::drawLayer(args, layer);
	}
};

struct Reshape : DMAExpanderModule<float, bool> {
	enum ParamId {
		CHANNEL_PARAM,
		SHAPE_PARAM,
		SHAPE_CV_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		SHAPE_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUTPUTS_LEN
	};
	enum LightId {
		CHANNEL_LIGHT_R,
		CHANNEL_LIGHT_G,
		CHANNEL_LIGHT_B,
		DMA_CLIENT_LIGHT,
		DMA_HOST_LIGHT_G,
		DMA_HOST_LIGHT_R,
		IN_WIDTH_A0,
		LIGHTS_LEN = IN_WIDTH_A0 + 7*4
	};

	const DMAChannelBase *topDataSource = nullptr;
	const DMAChannelBase *bottomDataSource = nullptr;
	SegmentStringDisplay *in_width_disp = nullptr;

	Reshape() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		std::vector<std::string> ch_labels;
		for (int i=0; i<PORT_MAX_CHANNELS; ++i)
			ch_labels.push_back(string::f("%d", i));
		ch_labels.push_back("All");
		configSwitch(CHANNEL_PARAM, 0.f, static_cast<float>(PORT_MAX_CHANNELS), 0.f, "DMA channel", ch_labels);
		paramQuantities[CHANNEL_PARAM]->snapEnabled = true;

		configParam(SHAPE_PARAM, -1.f, 1.f, 0.f, "Shape");
		configParam(SHAPE_CV_PARAM, 0.f, 1.f, 0.f, "Shape CV amount");
		configInput(SHAPE_INPUT, "Shape");
		dmaClientLightID = DMA_CLIENT_LIGHT;
		dmaHostLightID = DMA_HOST_LIGHT_G;
	}

	void process(const ProcessArgs& args) override {
		DMAExpanderModule<float, bool>::process(args);
		if (in_width_disp)
			in_width_disp->setString(string::f("%-4d", (int)(params[SHAPE_PARAM].getValue() * 1000)), args.sampleTime);
	}
};


struct ReshapeWidget : ModuleWidget {
	ReshapeWidget(Reshape* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Reshape.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<SynthTechAlco>(mm2px(Vec(10.16, 19.482)), module, Reshape::CHANNEL_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.16, 65.831)), module, Reshape::SHAPE_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(10.16, 76.54)), module, Reshape::SHAPE_CV_PARAM));

		addInput(createInputCentered<CL1362Port>(mm2px(Vec(10.16, 87.249)), module, Reshape::SHAPE_INPUT));

		addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(2.688, 29.813)), module, Reshape::CHANNEL_LIGHT_R));
		addChild(createLightCentered<SmallLight<BlueLight>>(Vec(8.0, 8.0), module, Reshape::DMA_CLIENT_LIGHT));
		addChild(createLightCentered<SmallLight<GreenRedLight>>(Vec(box.size.x - 8.0, 8.0), module, Reshape::DMA_HOST_LIGHT_G));

		// mm2px(Vec(15.24, 17.78))
		/*auto widget = createWidget<ReshapeDisplay>(mm2px(Vec(2.54, 26.31)));
		widget->box.size = mm2px(Vec(15.24, 17.78));
		if (module)
			widget->pDataSource = &module->topDataSource;
		addChild(widget);*/

		auto ssdw = new SegmentStringDisplayWidget(module, Reshape::IN_WIDTH_A0);
		for (int i=0; i<4; ++i)
			ssdw->createDigit<SevenSegments<TinySimpleSegmentH, TinySimpleSegmentV, GreenLight>>();
		if (module)
			module->in_width_disp = ssdw;
		ssdw->setPosition(mm2px(Vec(3.54, 37.0)));
		addChild(ssdw);

		auto widget = createWidget<ReshapeDisplay>(mm2px(Vec(2.54, 96.212)));
		widget->box.size = mm2px(Vec(15.24, 17.78));
		if (module)
			widget->pDataSource = &module->bottomDataSource;
		addChild(widget);
	}
};


Model* modelReshape = createModel<Reshape, ReshapeWidget>("Reshape");
