//
//  lateralTonemapProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 14/10/2014.
//
//

#ifndef video_mush_lateralTonemapProcess_hpp
#define video_mush_lateralTonemapProcess_hpp

#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>

class lateralTonemapProcess : public mush::imageProcess {
public:
    lateralTonemapProcess() : mush::imageProcess() {
        
    }
    
    ~lateralTonemapProcess() {
        
    }
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
        assert(buffers.size() == 1);
        inputBuffer = castToImage(buffers.begin()[0]);
        
        inputBuffer->getParams(_width, _height, _size);
        
        addItem(context->floatImage(_width, _height));
        
        readBuffer = (cl_float4 *)context->hostReadBuffer(_width*_height*sizeof(cl_float4));
        
        queue = context->getQueue();
        
        copyImage = context->getKernel("copyImage");
    }
    
    void process() {
        cl::size_t<3> origin;
        cl::size_t<3> region;
        origin[0] = 0; origin[1] = 0; origin[2] = 0;
        region[0] = _width; region[1] = _height; region[2] = 1;
        
        inLock();
        
        auto input = inputBuffer->outLock();
        
        if (input == nullptr) {
            release();
            return;
        }
        
        cl::Event event;
        
        copyImage->setArg(0, input.get_image());
        copyImage->setArg(1, _getMem(0).get_buffer());
        queue->enqueueNDRangeKernel(*copyImage, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        queue->enqueueReadImage(_getImageMem(0), CL_TRUE, origin, region, 0, 0, readBuffer, NULL, &event);
        event.wait();
        
        std::vector<float> max_vals, max_vals2;
        
        for (unsigned int i = 0; i < _width; ++i) {
            float max_val = 0.0f;
            for (unsigned int j = 0; j < _height; ++j) {
                for (unsigned int k = 0; k < 3; ++k) {
                    max_val = fmax(max_val, readBuffer[(j*_width+i)].s[k]);
                }
            }
            max_vals.push_back(max_val);
            max_vals2.push_back(max_val);
        }
        
        for (int64_t i = 0; i < _width; ++i) {
            //const float sigma = 300.0f;
            
            float max_vals_filter = max_vals[i];
            
            float weights = 1.0f;
            
            for (int64_t j = 0; j < 3; ++j) {
                if (i-j > 0) {
                    float weight = pow(0.99f, (float)(j+1));
                    max_vals_filter += max_vals[i-j-1]*weight;
                    weights += weight;
                }
                
                if (i + j + 1 < _height) {
                    //float weight = (exp(-((float)j + 1.0f)*((float)j + 1.0f)/(2.0f*sigma*sigma))) / sqrt(2*M_PI*sigma*sigma);
                    
                    float weight = pow(0.95f, (float)(j+1));
                    
                    max_vals_filter += max_vals[i+j+1]*weight;
                    weights += weight;
                }
            }
                        
            max_vals2[i] = max_vals_filter / weights;
        }
        
        for (unsigned int i = 0; i < _width; ++i) {
            for (unsigned int j = 0; j < _height; ++j) {
                for (unsigned int k =0; k < 3; ++k) {
                    readBuffer[(j*_width+i)].s[k] = readBuffer[(j*_width+i)].s[k] / max_vals2[i];
                }
            }
        }
        
        queue->enqueueWriteImage(_getImageMem(0), CL_TRUE, origin, region, 0, 0, (cl_float4 *)readBuffer, NULL, &event);
        event.wait();
        
        
        inputBuffer->outUnlock();
        inUnlock();
    }
    
private:
    cl::CommandQueue * queue;
    mush::registerContainer<mush::imageBuffer> inputBuffer;
    cl_float4 * readBuffer = nullptr;
    cl::Kernel * copyImage = nullptr;
    
};

#endif
