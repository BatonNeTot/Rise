//☀Rise☀
#include "loader.h"

namespace Rise {

	Loader::Loader(uint32_t threadCount) {
		for (uint32_t i = 0; i < threadCount; ++i) {
			_jobThreads.emplace_back(std::bind(&Loader::ThreadLoop, this));
		}
	}
	Loader::~Loader() {
		{
			std::unique_lock<std::mutex> ul(_queueLock);
			_destroying = true;
			_queueCond.notify_all();
		}
		for (auto& thread : _jobThreads) {
			thread.join();
		}
	}

	void Loader::ThreadLoop() {
		while (true) {
			std::unique_lock<std::mutex> ul(_queueLock);
			_queueCond.wait(ul, [=]() { return !_workQueue.empty() || _destroying; });
			if (_workQueue.empty()) {
				return;
			}

			auto job = std::move(_workQueue.front());
			_workQueue.pop();
			ul.unlock();

			job();
		}
	}

}
