//☀Rise☀
#include "rise_object.h"

#include "rise.h"

namespace Rise {

	void RiseObject::Log(Logger::Level level, const std::string& message) const {
		_core->Logger().Log(level, message);
	}

}
