#include "Widgets.hpp"

namespace sparkette {

CKSSWithLine::CKSSWithLine() {
	shadow->opacity = 0.0;
	addFrame(Svg::load(asset::system("res/ComponentLibrary/CKSS_0.svg")));
	addFrame(Svg::load(asset::plugin(pluginInstance, "res/CKSS_1_line.svg")));
}

}
