//
//  timerWrapper.hpp
//  video-mush
//
//  Created by Josh McNamee on 24/08/2016.
//
//

#ifndef timerWrapper_hpp
#define timerWrapper_hpp

#include <memory>
#include <mutex>
#include <chrono>
#include <vector>
#include <string>
#include <unordered_map>
#include "metricConfig.hpp"
#include "processNode.hpp"
#include "metricReporter.hpp"
#include "imageProcessMetric.hpp"
#include "mush-core-dll.hpp"

namespace mush {
class MUSHEXPORTS_API timerWrapper {
public:
    timerWrapper(std::string tag);
    ~timerWrapper();
    
    void register_node(std::shared_ptr<processNode> node, std::string name);

	void register_metric(std::shared_ptr<metricReporter> node, std::string name);

	void add_metric(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers, metrics m, std::string append);
    
    void process(std::shared_ptr<processNode> node);

	void process_metric(std::shared_ptr<imageProcessMetric> m);

	void process_metrics();

	void metric(std::shared_ptr<metricReporter> m);

	void print_header();

    void print_report();
    
    void print_metered_report();
    
    void print_final_report();
    
	std::vector<std::shared_ptr<mush::guiAccessible>> get_metric_gui() const;

private:
	void _print_report();
    
    std::chrono::steady_clock _steady_clock;
    
    std::unordered_map<void *, std::string> _names;

	enum class type {
		timer,
		metric
	};

	struct reporter {
		std::string format;
		std::string title_format;
		std::string name;
		type t;
		size_t index;
	};

	std::unordered_map<void *, size_t> _reporter_index;
	std::vector<reporter> _reporters;
	std::vector<std::vector<std::chrono::duration<int64_t, std::ratio<1, 100000000000>>>> _durations;
	std::vector<std::vector<metric_value>> _metric_results;

	std::vector<std::shared_ptr<imageProcessMetric>> _metric_storage;

	bool _first_go = true;

	std::mutex _durations_mutex;

	uint32_t _count = 0;

	std::string _tag;

	bool log_output = true;
	bool data_output = true;
};

}

#endif /* timerWrapper_hpp */
