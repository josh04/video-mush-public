//
//  motionProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 12/08/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef video_mush_gmotionProcess_hpp
#define video_mush_gmotionProcess_hpp

#include <Mush Core/registerContainer.hpp>
#include <Mush Core/integerMapProcess.hpp>
#include <Mush Core/tagInGui.hpp>


class getDiscontinuities : public mush::integerMapProcess {
public:
	getDiscontinuities();
    
	~getDiscontinuities();
    
	void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
    
	void process() override;
    
	void inUnlock() override;
    
private:
    cl::CommandQueue * queue = nullptr;
    cl::Kernel * discontinuities = nullptr;
    cl::Kernel * copyImage = nullptr;

    mush::registerContainer<mush::imageBuffer> motionVectors;
    mush::registerContainer<mush::imageBuffer> depth;
};
#endif
