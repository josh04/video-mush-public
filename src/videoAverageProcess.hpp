#ifndef VIDEOAVERAGEPROCESS_HPP
#define VIDEOAVERAGEPROCESS_HPP

#include <Mush Core/singleKernelProcess.hpp>

namespace mush {
	class videoAverageProcess : public singleKernelProcess {
	public:
		videoAverageProcess(int cap = 1000);
		~videoAverageProcess();

		void setArgs() override;
	private:
		int _count = 0;
		int _cap = 0;
	};

}

#endif

