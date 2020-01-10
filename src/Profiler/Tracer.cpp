#include "Tracer.hpp"
#include <assert.h>
#include "OS/Process.hpp"

Tracer::Tracer() noexcept {}

void Tracer::begin_session(std::string name) noexcept {
	assert(!session_running);
	current_session_name = name;
	current_session = dyn_struct::array_t{};
	session_running = true;
	timers.push_back(new Scoped_Timer("session", name));
}

void Tracer::end_session(std::filesystem::path path) noexcept {
	assert(session_running);

	for (uint32_t i = timers.size() - 1; i + 1 > 0; --i) {
		delete timers[i];
	}
	timers.clear();

	dyn_struct obj = dyn_struct::structure_t{};
	obj["traceEvents"] = std::move(current_session);
	save_to_json_file(obj, path / (current_session_name + ".json"));

	session_running = false;
}

void Tracer::push(dyn_struct str) noexcept {
	assert(session_running);

	current_session.push_back(std::move(str));
}

void Tracer::begin(std::string name) noexcept {
	timers.push_back(new Scoped_Timer("timer", name));
}

void Tracer::end() noexcept {
	delete timers.back();
	timers.pop_back();
}

Scoped_Timer::Scoped_Timer(std::string cat, std::string name) noexcept {
	this->cat = cat;
	this->name = name;
	this->ph = Phase::X;
	pid = get_process_id();
	tid = get_thread_id();
	ts = std::chrono::steady_clock::now();
}

Scoped_Timer::~Scoped_Timer() noexcept {
	auto end = std::chrono::steady_clock::now();
	auto dur = std::chrono::duration_cast<std::chrono::microseconds>(end - ts).count();

	dyn_struct str = dyn_struct::structure_t{};
	str["cat"] = cat;
	str["name"] = name;
	str["dur"] = dur;
	str["ph"] = (char)ph;
	str["pid"] = pid;
	str["tid"] = tid;
	str["ts"] = ts.time_since_epoch().count() / 1000;
	str["ph"] = ph;

	Tracer::get().push(std::move(str));
}


