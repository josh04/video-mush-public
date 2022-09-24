//
//  edgeEncodingProcessor.hpp
//  parcore
//
//  Created by Josh McNamee on 26/06/2015.
//  Copyright (c) 2015 Josh McNamee. All rights reserved.
//

#ifndef parcore_edgeEncodingProcessor_hpp
#define parcore_edgeEncodingProcessor_hpp

#include <Mush Core/timerWrapper.hpp>
#include <Mush Core/imageProcessor.hpp>

#include "jbig2EncodeProcess.hpp"
#include "jbig2DecodeProcess.hpp"
#include "edgeEncodingExtractBinaryMap.hpp"
#include "edgeSamplePacker.hpp"
#include "regularEdgeSamples.hpp"
#include "lzmaEncode.hpp"

class edgeEncodingProcessor : public mush::imageProcessor {
public:
    edgeEncodingProcessor(float threshold, std::shared_ptr<mush::timerWrapper> timer_wrapper, std::string name);
    
    ~edgeEncodingProcessor();
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
    
    void process() override;
    
    const std::vector<std::shared_ptr<mush::ringBuffer>> getBuffers() const override;
    
    std::vector<std::shared_ptr<mush::guiAccessible>> getGuiBuffers() override;
    
    void go() override;
    
    void release();
    
    shared_ptr <mush::imageProcess> getLaplace() { return laplace; }
    shared_ptr <mush::imageProcess> getEdgeSamples() { return edgeSamples; }
    shared_ptr <mush::imageProcess> getReconstruction() { return reconstruction; }
    
    private:
    shared_ptr <mush::imageProcess> laplace  = nullptr;
    shared_ptr <mush::imageProcess> edgeSamples = nullptr;
    //shared_ptr <mush::imageProcess> regularSamples = nullptr;
    shared_ptr <mush::imageProcess> reconstruction = nullptr;
    
    shared_ptr <jbig2EncodeProcess> jbig2 = nullptr;
    shared_ptr <edgeEncodingExtractBinaryMap> edge_extract_binary = nullptr;
    shared_ptr <jbig2DecodeProcess> jbig2_dec = nullptr;
    shared_ptr <edgeSamplePacker> sample_packer = nullptr;
    shared_ptr <regularEdgeSamples> regular_samples = nullptr;
    shared_ptr <lzmaEncode> lzma_encode_regular = nullptr;
    shared_ptr <lzmaEncode> lzma_encode_packed = nullptr;
    
    std::vector<std::shared_ptr<mush::guiAccessible>> _guiBuffers;
    
    float _threshold = 0.1f;
    std::shared_ptr<mush::timerWrapper> _timer = nullptr;
    
    std::string _name;
};

#endif
