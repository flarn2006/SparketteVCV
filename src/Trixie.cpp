#include "plugin.hpp"
#include <cmath>


struct Trixie : Module {
	enum ParamId {
		PARAMS_LEN
	};
	enum InputId {
		INPUTS_LEN
	};
	enum OutputId {
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	Trixie() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
	}

	float phase = 0.f;

	void onReset(const ResetEvent &e) override {
		phase = 0.f;
	}

	void process(const ProcessArgs& args) override {
		phase += args.sampleTime;
		while (phase > 1.f)
			phase -= 1.f;
	}
};


struct TrixieWidget : ModuleWidget {
	float *pPhase = nullptr;

	TrixieWidget(Trixie* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Trixie.svg")));
		if (module)
			pPhase = &module->phase;

		/*addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));*/
	}

	void drawLayer(const DrawArgs &args, int layer) override {
		if (pPhase && layer == 1 && module) {
			double left = (module->leftExpander.module && module->leftExpander.module->model == modelEllie) ? -RACK_GRID_WIDTH*8 : 0.0;
			double right = box.size.x + ((module->rightExpander.module && module->rightExpander.module->model == modelEllie) ? RACK_GRID_WIDTH*8 : 0.0);
			float sine = std::sin(*pPhase * M_PI) + 1.f / 2;
			double aura_width = 7.0 - (double)sine * 2;
			nvgFillColor(args.vg, nvgRGBA(0x51, 0xff, 0xff, 0x60 + (int)(sine * 0x60)));

			nvgBeginPath(args.vg);
			nvgRect(args.vg, left-aura_width, -aura_width, right-left+2*aura_width, aura_width);
			nvgFill(args.vg);
			nvgClosePath(args.vg);

			nvgBeginPath(args.vg);
			nvgRect(args.vg, left-aura_width, box.size.y, right-left+2*aura_width, aura_width);
			nvgFill(args.vg);
			nvgClosePath(args.vg);

			nvgBeginPath(args.vg);
			nvgRect(args.vg, left-aura_width, 0.0, aura_width, box.size.y);
			nvgFill(args.vg);
			nvgClosePath(args.vg);

			nvgBeginPath(args.vg);
			nvgRect(args.vg, right, 0.0, aura_width, box.size.y);
			nvgFill(args.vg);
			nvgClosePath(args.vg);
		}
		ModuleWidget::drawLayer(args, layer);
	}
};


Model* modelTrixie = createModel<Trixie, TrixieWidget>("Trixie");
