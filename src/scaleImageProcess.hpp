#ifndef SCALEIMAGEPROCESS_HPP
#define SCALEIMAGEPROCESS_HPP

#include <Mush Core/singleKernelProcess.hpp>

namespace mush {

	class scaleImageProcess : public singleKernelProcess {
	public:
		scaleImageProcess(unsigned int width, unsigned int height);
		~scaleImageProcess();

		void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;

	private:

	};

}


#endif
