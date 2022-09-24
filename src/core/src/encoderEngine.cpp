

#include "encoderEngine.hpp"

extern void SetThreadName(const char * threadName);

void encoderEngine::startThread() {
	SetThreadName("encoderEngine");
	gather();
}