//☀Rise☀
#include "Rise/rise_object.h"

#include "Rise/rise.h"

namespace Rise {

	void RiseObject::Log(Logger::Level level, const std::string& message) const {
		_core->Logger().Log(level, message);
	}

}
