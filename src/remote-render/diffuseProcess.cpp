//
//  diffuseProcess.cpp
//  parcore
//
//  Created by Josh McNamee on 25/06/2015.
//  Copyright (c) 2015 Josh McNamee. All rights reserved.
//

#include <Mush Core/opencl.hpp>

#include "diffuseProcess.hpp"

void diffuseProcess::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
    assert(buffers.size() == 2);
    
    copy = context->getKernel("copyImage");
    regular_samples = context->getKernel("regular_sample_expand");
    push = context->getKernel("diffuse_push");
    pull_copy = context->getKernel("diffuse_pull_copy");
    pull_diag = context->getKernel("diffuse_pull_diag");
    pull_horiz = context->getKernel("diffuse_pull_horiz");
    
    final_correct = context->getKernel("diffuse_final_correct");
    
    buffer = castToImage(buffers.begin()[0]);
    _regular_samples = castToImage(buffers.begin()[1]);
    
    buffer->getParams(_width, _height, _size);
    _regular_samples->getParams(_reg_width, _reg_height, _size);
    
    addItem(context->floatImage(_width, _height));
    
    
    scratch = context->redGreenImage(_width, _height);;
    
    first_width = (_width + 2 - (_width % 2)) / 2;
    first_height = (_height + 2 - (_height % 2)) / 2;
    first = context->redGreenImage(first_width, first_height);
    second_width = (first_width + 2 - (first_width % 2)) / 2;
    second_height = (first_height + 2 - (first_height % 2)) / 2;
    second = context->redGreenImage(second_width, second_height);
    third_width = (second_width + 2 - (second_width % 2)) / 2;
    third_height = (second_height + 2 - (second_height % 2)) / 2;
    third = context->redGreenImage(third_width, third_height);
    fourth_width = (third_width + 2 - (third_width % 2)) / 2;
    fourth_height = (third_height + 2 - (third_height % 2)) / 2;
    fourth = context->redGreenImage(fourth_width, fourth_height);
    fifth_width = (fourth_width + 2 - (fourth_width % 2)) / 2;
    fifth_height = (fourth_height + 2 - (fourth_height % 2)) / 2;
    fifth = context->redGreenImage(fifth_width, fifth_height);
    
    queue = context->getQueue();
}

void diffuseProcess::process() {
    cl::Event event;
    inLock();
    auto input = buffer->outLock();
    if (input == nullptr) {
        release();
        return;
    }
    auto reg_s = _regular_samples->outLock();
    if (reg_s == nullptr) {
        release();
        return;
    }
    copy->setArg(0, input.get_image());
    copy->setArg(1, _getImageMem(0));
    queue->enqueueNDRangeKernel(*copy, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
    event.wait();
    
    regular_samples->setArg(0, reg_s.get_image());
    regular_samples->setArg(1, _getImageMem(0));
    queue->enqueueNDRangeKernel(*regular_samples, cl::NullRange, cl::NDRange(_reg_width, _reg_height), cl::NullRange, NULL, &event);
    event.wait();
    
    buffer->outUnlock();
    _regular_samples->outUnlock();
    
	cl::Image2D image_mem = _getImageMem(0);

    push_seq(&image_mem, first, first_width, first_height);
    push_seq(first, second, second_width, second_height);
    push_seq(second, third, third_width, third_height);
    push_seq(third, fourth, fourth_width, fourth_height);
    push_seq(fourth, fifth, fifth_width, fifth_height);
    
    pull_seq(fifth, fourth, fifth_width, fifth_height, fourth_width, fourth_height);
    pull_seq(fourth, third, fourth_width, fourth_height, third_width, third_height);
    pull_seq(third, second, third_width, third_height, second_width, second_height);
    pull_seq(second, first, second_width, second_height, first_width, first_height);
    pull_seq(first, &image_mem, first_width, first_height, _width, _height);
    
    
    final_correct->setArg(0, _getImageMem(0));
    final_correct->setArg(1, _getImageMem(0));
    
    queue->enqueueNDRangeKernel(*final_correct, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
    event.wait();
    
    inUnlock();
}

void diffuseProcess::push_seq(cl::Image2D * top, cl::Image2D * bottom, const unsigned int &width, const unsigned int &height) {
    cl::Event event;
    push->setArg(0, *top);
    push->setArg(1, *bottom);
    queue->enqueueNDRangeKernel(*push, cl::NullRange, cl::NDRange(width, height), cl::NullRange, NULL, &event);
    event.wait();
}

void diffuseProcess::pull_seq(cl::Image2D * bottom, cl::Image2D * top, const unsigned int &bottom_width, const unsigned int &bottom_height, const unsigned int &top_width, const unsigned int &top_height) {
    cl::Event event;
    pull_copy->setArg(0, *bottom);
    pull_copy->setArg(1, *top);
    pull_copy->setArg(2, *top);
    queue->enqueueNDRangeKernel(*pull_copy, cl::NullRange, cl::NDRange(bottom_width, bottom_height), cl::NullRange, NULL, &event);
    event.wait();
    
    copy->setArg(0, *top);
    copy->setArg(1, *scratch);
    queue->enqueueNDRangeKernel(*copy, cl::NullRange, cl::NDRange(top_width, top_height), cl::NullRange, NULL, &event);
    event.wait();
    
    pull_diag->setArg(0, *top);
    pull_diag->setArg(1, *scratch);
    queue->enqueueNDRangeKernel(*pull_diag, cl::NullRange, cl::NDRange(bottom_width, bottom_height), cl::NullRange, NULL, &event);
    event.wait();
    
    copy->setArg(0, *scratch);
    copy->setArg(1, *top);
    queue->enqueueNDRangeKernel(*copy, cl::NullRange, cl::NDRange(top_width, top_height), cl::NullRange, NULL, &event);
    event.wait();
    
    pull_horiz->setArg(0, *scratch);
    pull_horiz->setArg(1, *top);
    queue->enqueueNDRangeKernel(*pull_horiz, cl::NullRange, cl::NDRange(top_width, top_height), cl::NullRange, NULL, &event);
    event.wait();
}
