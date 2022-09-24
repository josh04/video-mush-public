#ifndef OCULUSEYESPRITE_HPP
#define OCULUSEYESPRITE_HPP

#include <azure/sprite.hpp>

namespace mush {

class oculusEyeSprite : public azure::Sprite {
public:
	oculusEyeSprite(std::shared_ptr<azure::Program> program, std::shared_ptr<azure::Texture> texture);
	~oculusEyeSprite();

private:

};

}

#endif