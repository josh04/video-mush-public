#ifndef MSEPROCESS_HPP
#define MSEPROCESS_HPP

#include <Mush Core/singleKernelProcess.hpp>

class mseProcess : public mush::singleKernelProcess {
public:
	mseProcess() : mush::singleKernelProcess("psnr_diff_mse") {

	}

	~mseProcess() {};
};

#endif
