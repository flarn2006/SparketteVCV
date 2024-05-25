#include "plugin.hpp"
#include "DMA.hpp"
#include "Segments.hpp"
#include <vector>
#include <string>

using namespace sparkette;

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
		IN_HEIGHT_A0 = IN_WIDTH_A0 + 7*4,
		OUT_WIDTH_A0 = IN_HEIGHT_A0 + 7*4,
		OUT_HEIGHT_A0 = OUT_WIDTH_A0 + 7*4,
		LIGHTS_LEN = OUT_HEIGHT_A0 + 7*4
	};
	enum DisplayId {
		IN_WIDTH,
		IN_HEIGHT,
		OUT_WIDTH,
		OUT_HEIGHT,
		DISPLAYS_LEN
	};

	const DMAChannelBase *topDataSource = nullptr;
	const DMAChannelBase *bottomDataSource = nullptr;
	SegmentStringDisplay *displays[DISPLAYS_LEN];

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

		for (int i=0; i<DISPLAYS_LEN; ++i)
			displays[i] = nullptr;
	}

	void process(const ProcessArgs& args) override {
		DMAExpanderModule<float, bool>::process(args);
		for (int i=0; i<DISPLAYS_LEN; ++i)
			if (displays[i])
				displays[i]->refresh(args.sampleTime);
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

		auto ssdw = new SegmentStringDisplayWidget(module, Reshape::IN_WIDTH_A0);
		for (int i=0; i<4; ++i)
			ssdw->createDigit<SevenSegments<TinySimpleSegmentH, TinySimpleSegmentV, GreenLight>>();
		if (module)
			module->displays[Reshape::IN_WIDTH] = ssdw;
		ssdw->setPosition(mm2px(Vec(3.54, 37.0)));
		addChild(ssdw);

		ssdw = new SegmentStringDisplayWidget(module, Reshape::IN_HEIGHT_A0);
		for (int i=0; i<4; ++i)
			ssdw->createDigit<SevenSegments<TinySimpleSegmentH, TinySimpleSegmentV, GreenLight>>();
		if (module)
			module->displays[Reshape::IN_HEIGHT] = ssdw;
		ssdw->setPosition(mm2px(Vec(3.54, 47.8)));
		addChild(ssdw);

		ssdw = new SegmentStringDisplayWidget(module, Reshape::OUT_WIDTH_A0);
		for (int i=0; i<4; ++i)
			ssdw->createDigit<SevenSegments<TinySimpleSegmentH, TinySimpleSegmentV, GreenLight>>();
		if (module)
			module->displays[Reshape::OUT_WIDTH] = ssdw;
		ssdw->setPosition(mm2px(Vec(3.54, 96.9)));
		addChild(ssdw);

		ssdw = new SegmentStringDisplayWidget(module, Reshape::OUT_HEIGHT_A0);
		for (int i=0; i<4; ++i)
			ssdw->createDigit<SevenSegments<TinySimpleSegmentH, TinySimpleSegmentV, GreenLight>>();
		if (module)
			module->displays[Reshape::OUT_HEIGHT] = ssdw;
		ssdw->setPosition(mm2px(Vec(3.54, 107.7)));
		addChild(ssdw);
	}
};


Model* modelReshape = createModel<Reshape, ReshapeWidget>("Reshape");
