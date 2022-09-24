//
//  timerWrapper.cpp
//  video-mush
//
//  Created by Josh McNamee on 24/08/2016.
//
//

#include "timerWrapper.hpp"
#include <boost/timer/timer.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include "psnrProcess.hpp"
#include "ssimProcess.hpp"
#include "mushLog.hpp"

extern "C" void putLog(std::string s);

namespace mush {
    
    timerWrapper::timerWrapper(std::string tag) : _tag(tag) {
        
    }
    
    timerWrapper::~timerWrapper() {
        
    }
    
    void timerWrapper::register_node(std::shared_ptr<processNode> node, std::string name) {
		auto sz = _durations.size(); 
		auto sz2 = _reporters.size();
		_reporter_index.insert({ node.get(), sz2 });
		_reporters.push_back({  "%7fs | ", "%9s | ", name, type::timer, sz });
		_durations.push_back({});
        //_durations.insert({node.get(), {  }});
        
       // _names.insert({node.get(), name});
    }

	void timerWrapper::register_metric(std::shared_ptr<metricReporter> node, std::string name) {
		auto sz = _metric_results.size();
		auto sz2 = _reporters.size();
		_reporter_index.insert({ node.get(), sz2 });
		_reporters.push_back({ node->get_format_string(), node->get_title_format_string(), name, type::metric, sz });
		_metric_results.push_back({});
		//_metric_results.insert({ node.get(), {} });
		//_names.insert({ node.get(), name });
	}
    
	void timerWrapper::add_metric(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers, metrics m, std::string append) {
		std::shared_ptr<imageProcessMetric> p = nullptr;
		std::string name;
        switch (m) {
            case metrics::psnr:
                p = std::make_shared<mush::psnrProcess>(psnrProcess::type::linear, 1.0f);
                name = "PSNR ";
                break;
            case metrics::log_psnr:
                p = std::make_shared<mush::psnrProcess>(psnrProcess::type::log, 1e3f);
                name = "Log PSNR ";
                break;
            case metrics::pupsnr:
                p = std::make_shared<mush::psnrProcess>(psnrProcess::type::pu, 1.0f);
                name = "puPSNR ";
                break;
                
            case metrics::ssim:
                p = std::make_shared<mush::ssimProcess>(ssimProcess::type::linear);
                name = "SSIM ";
                break;
            case metrics::pussim:
                p = std::make_shared<mush::ssimProcess>(ssimProcess::type::pu);
                name = "puSSIM ";
                break;
        }
		p->init(context, buffers);
		register_metric(p, name + append);
		p->setTagInGuiName(name + append);
		_metric_storage.push_back(p);
	}

    void timerWrapper::process(std::shared_ptr<processNode> node) {
		
        auto now = _steady_clock.now();
        node->process();
        auto end = _steady_clock.now();
        
        std::chrono::duration<int64_t, std::ratio<1, 100000000000>> period = end - now;
        
		//std::lock_guard<std::mutex> lock(_durations_mutex);

        auto search = _reporter_index.find(node.get());
        if (search != _reporter_index.end()) {
			if (_reporters[search->second].t == type::timer) {
				_durations[_reporters[search->second].index].push_back(period);
			}
        }
    }

	void timerWrapper::process_metric(std::shared_ptr<imageProcessMetric> m) {
		m->process();
		metric(m);
	}

	void timerWrapper::process_metrics() {
		for (auto& m : _metric_storage) {
			process_metric(m);
		}
	}

	void timerWrapper::metric(std::shared_ptr<metricReporter> m) {
		auto met = m->get_last();
        if (met.type != metric_value_type::null) {
		auto search = _reporter_index.find(m.get());
		if (search != _reporter_index.end()) {
			if (_reporters[search->second].t == type::metric) {
				_metric_results[_reporters[search->second].index].push_back(met);
			}
		}
        }
	}

	void timerWrapper::print_header() {
		std::stringstream report;

		std::string format_string = " %5s  | ";

		std::vector<std::string> names = { "Frame" };

		for (auto& report : _reporters) {
			names.push_back(report.name);
			format_string += report.title_format;
		}

		if (log_output) {
			auto format = boost::format(format_string);
			for (auto& name : names) {
				format % name;
			}

			report << boost::str(format);
			putLog(report.str());
		}

		if (data_output) {
			setRowNames(names);
		}
	}

	void timerWrapper::print_report() {
		_print_report();
		_count++;
	}
    
    void timerWrapper::_print_report() {
        if (_first_go) {
            _first_go = false;
            print_header();
        }
        
			std::stringstream report;

			std::string format_string = " %6d | ";

			std::vector<metric_value> packaged_metrics;

			metric_value counter;
			counter.type = metric_value_type::i;
			counter.value.integer = _count;
			
			packaged_metrics.push_back(counter);

			{
				//std::lock_guard<std::mutex> lock(_durations_mutex);
				for (auto& report : _reporters) {

					format_string += report.format;
					if (report.t == type::timer) {

						if (_durations[report.index].size() > 0) {
							std::chrono::duration<int64_t, std::ratio<1, 100000000000>> total_duration = _durations[report.index].back();
							metric_value_union var;
							var.floating_point = total_duration.count() / 100000000000.0;
							packaged_metrics.push_back({ metric_value_type::f, var, _count, false, false });
						} else {
							metric_value_union var;
							var.floating_point = 1e9;
							packaged_metrics.push_back({ metric_value_type::f, var, _count, false, false });
						}
					} else if (report.t == type::metric) {
                        if (_metric_results[report.index].size() > 0) {
                            packaged_metrics.push_back(_metric_results[report.index].back());
                        }
					}
				}
			}

			if (log_output) {

				auto format = boost::format(format_string);

				for (auto& dur : packaged_metrics) {
					switch (dur.type) {
					case metric_value_type::f:
						format % dur.value.floating_point;
						break;
					case metric_value_type::i:
						format % dur.value.integer;
						break;
					}
				}

				//format % _tag;

				report << boost::str(format);
				putLog(report.str());
			}
			if (data_output) {
				addRow(packaged_metrics);
			}
    }

	void timerWrapper::print_metered_report() {
		bool do_report = false;

		if (_count > 5000) {
			if (_count % 1000 == 0) {
				do_report = true;
			}
		} else if (_count > 1000) {
			if (_count % 100 == 0) {
				do_report = true;
			}
		} else if (_count > 100) {
			if (_count % 10 == 0) {
				do_report = true;
			}
		} else {
			do_report = true;
		}

		if (do_report) {
			_print_report();
		}
		_count++;
	}
    
    void timerWrapper::print_final_report() {
		print_header();

        std::stringstream report;
        
        std::string format_string = " Avg. %6d | ";
    
        std::vector<metric_value> float_durations;

		metric_value var;
		var.type = metric_value_type::i;
		var.value.integer = (_count);
		var.is_average = true;
		var.frame = _count;

		float_durations.push_back(var);

		{
			//std::lock_guard<std::mutex> lock(_durations_mutex);
			for (auto& report : _reporters) {
				format_string += report.format;
				if (report.t == type::timer) {

					std::chrono::duration<int64_t, std::ratio<1, 100000000000>> total_duration = std::chrono::seconds(0);
					for (auto& dur : _durations[report.index]) {
						total_duration += dur;
					}
					metric_value_union val;
					val.floating_point = (std::chrono::duration_cast<std::chrono::microseconds>(total_duration).count() / 1e6) / (float)_durations[report.index].size();

					float_durations.push_back({ metric_value_type::f, val, _count, true, false });
				} else if (report.t == type::metric) {

					if (_metric_results[report.index].size() > 0) {
						auto met_first = _metric_results[report.index][0];

						metric_value ret;
						ret.type = met_first.type;
						ret.is_average = true;
						ret.frame = _count;

						switch (ret.type) {
						case metric_value_type::f:
						{

							float met = 0.0f;
							for (auto& m : _metric_results[report.index]) {
									met += m.value.floating_point;
							}
							ret.value.floating_point = met / (float)_metric_results[report.index].size();

						}
							break;
						case metric_value_type::i:
						{
							int64_t met = 0.0f;
							for (auto& m : _metric_results[report.index]) {
								met += m.value.integer;
							}
							ret.value.integer = met / (float)_metric_results[report.index].size();
						}
							break;
						}

						float_durations.push_back(ret);
					}

				}
			}
		}

		if (log_output) {

			auto format = boost::format(format_string);

			for (auto& dur : float_durations) {
				switch (dur.type) {
				case metric_value_type::f:
					format % dur.value.floating_point;
					break;
				case metric_value_type::i:
					format % dur.value.integer;
					break;
				}
			}

			report << boost::str(format);
			putLog(report.str());
		}
		if (data_output) {
			addRow(float_durations);
		}

    }
    
	std::vector<std::shared_ptr<mush::guiAccessible>> timerWrapper::get_metric_gui() const {
		std::vector<std::shared_ptr<mush::guiAccessible>> ret;
		for (auto &p : _metric_storage) {
			auto q = std::dynamic_pointer_cast<mush::guiAccessible>(p);
			if (q != nullptr) {
				ret.push_back(q);
			}
		}
		return ret;
	}
}
