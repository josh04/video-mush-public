#ifndef IMAGEPROCESSMETRIC_HPP
#define IMAGEPROCESSMETRIC_HPP

#include "imageProcess.hpp"
#include "metricReporter.hpp"

namespace mush {
	class imageProcessMetric : public imageProcess, public metricReporter {
	public:
		imageProcessMetric() : imageProcess(), metricReporter() {}
		~imageProcessMetric() {}
	};

}

#endif
