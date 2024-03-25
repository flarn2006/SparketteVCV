#include "plugin.hpp"
#include "Knobs.hpp"

namespace sparkette {

Rogan1PYellow::Rogan1PYellow() {
	setSvg(Svg::load(asset::plugin(pluginInstance, "res/Rogan1PYellow.svg")));
	bg->setSvg(Svg::load(asset::system("res/ComponentLibrary/Rogan1P_bg.svg")));
	fg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/Rogan1PYellow_fg.svg")));
}

Rogan1PPurple::Rogan1PPurple() {
	setSvg(Svg::load(asset::plugin(pluginInstance, "res/Rogan1PPurple.svg")));
	bg->setSvg(Svg::load(asset::system("res/ComponentLibrary/Rogan1P_bg.svg")));
	fg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/Rogan1PPurple_fg.svg")));
}

}
