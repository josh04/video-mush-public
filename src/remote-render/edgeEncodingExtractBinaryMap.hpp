//
//  edgeEncodingExtractBinaryMap.hpp
//  video-mush
//
//  Created by Josh McNamee on 25/10/2016.
//
//

#ifndef edgeEncodingExtractBinaryMap_hpp
#define edgeEncodingExtractBinaryMap_hpp

#include <memory>
#include <vector>
#include <Mush Core/integerMapProcess.hpp>
#include <Mush Core/registerContainer.hpp>

class edgeEncodingExtractBinaryMap : public mush::integerMapProcess {
public:
    edgeEncodingExtractBinaryMap();
    ~edgeEncodingExtractBinaryMap();
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
    
    void process() override;
    
private:
    mush::registerContainer<mush::imageBuffer> _input;
    
    cl::Kernel * _edge = nullptr;
    
};

#endif /* edgeEncodingExtractBinaryMap_hpp */
