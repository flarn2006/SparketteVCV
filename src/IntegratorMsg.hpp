#pragma once

namespace sparkette {

	struct IntegratorMsg {
		struct {
			bool max = false;
			bool min = false;
			bool wrap_pulse = false;
		} a, b;
		int side = -1;
		bool wrap_enabled = false;
	};

}
