//
//  edgeEncodingProcessor.cpp
//  video-mush
//
//  Created by Josh McNamee on 26/10/2016.
//
//

#include "edgeEncodingProcessor.hpp"
#include "../laplaceProcess.hpp"
#include "edgeThreshold.hpp"
#include "diffuseProcess.hpp"

edgeEncodingProcessor::edgeEncodingProcessor(float threshold, std::shared_ptr<mush::timerWrapper> timer_wrapper, std::string name) : mush::imageProcessor(), _threshold(threshold), _timer(timer_wrapper), _name(name) {
}

edgeEncodingProcessor::~edgeEncodingProcessor() {

}

void edgeEncodingProcessor::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
    assert(buffers.size() == 1);
    auto input = buffers.begin()[0];
    
    laplace = make_shared<laplaceProcess>();
    laplace->init(context, input);
    
    edgeSamples = make_shared<edgeThreshold>(_threshold);
    edgeSamples->init(context, {input, laplace});
    
    regular_samples = make_shared<regularEdgeSamples>();
    regular_samples->init(context, {input});
    
    lzma_encode_regular = make_shared<lzmaEncode>();
    lzma_encode_regular->init(context, {regular_samples});
    
    edge_extract_binary = make_shared<edgeEncodingExtractBinaryMap>();
    edge_extract_binary->init(context, {edgeSamples});
    
    
    sample_packer = make_shared<edgeSamplePacker>();
    sample_packer->init(context, {edgeSamples, edge_extract_binary});
    
    lzma_encode_packed = make_shared<lzmaEncode>();
    lzma_encode_packed->init(context, {sample_packer});
    
    jbig2 = make_shared<jbig2EncodeProcess>();
    jbig2->init(context, {edge_extract_binary});
    
    jbig2_dec = make_shared<jbig2DecodeProcess>();
    jbig2_dec->init(context, {jbig2});
    
    reconstruction = make_shared<diffuseProcess>();
    reconstruction->init(context, {edgeSamples, regular_samples});
    
    laplace->setTagInGuiName("Laplacian");
    edgeSamples->setTagInGuiName("Edge Samples");
    regular_samples->setTagInGuiName("Regular Samples");
    edge_extract_binary->setTagInGuiName("Edge Sample Map");
    jbig2_dec->setTagInGuiName("Decoded Edge Sample Map");
    reconstruction->setTagInGuiName("Edge Reconstruction");
    
    if (_timer != nullptr) {
        std::string text = _name + " JBIG2 P s";
        _timer->register_metric(jbig2, text);
        
        std::string text2 = _name + " Edge Sample P s";
        _timer->register_metric(sample_packer, text2);
        
        std::string text3 = _name + " Reg LZMA P s";
        _timer->register_metric(lzma_encode_regular, text3);
        std::string text4 = _name + " Edge LZMA P s";
        _timer->register_metric(lzma_encode_packed, text4);
    }
}

void edgeEncodingProcessor::process() {
    laplace->process();
    edgeSamples->process();
    regular_samples->process();
    lzma_encode_regular->process();
    _timer->metric(lzma_encode_regular);
    edge_extract_binary->process();
    sample_packer->process();
    _timer->metric(sample_packer);
    lzma_encode_packed->process();
    _timer->metric(lzma_encode_packed);
    jbig2->process();
    _timer->metric(jbig2);
    jbig2_dec->process();
    reconstruction->process();
}

const std::vector<std::shared_ptr<mush::ringBuffer>> edgeEncodingProcessor::getBuffers() const {
    return {reconstruction};
}

std::vector<std::shared_ptr<mush::guiAccessible>> edgeEncodingProcessor::getGuiBuffers() {
    return {laplace, edgeSamples, regular_samples, edge_extract_binary, jbig2_dec, reconstruction};
}

void edgeEncodingProcessor::go() {
    while (laplace->good()) {
        process();
    }
    
    release();
}

void edgeEncodingProcessor::release() {
    laplace->release();
    edgeSamples->release();
    regular_samples->release();
    lzma_encode_regular->release();
    edge_extract_binary->release();
    sample_packer->release();
    lzma_encode_packed->release();
    jbig2->release();
    jbig2_dec->release();
    reconstruction->release();
}
