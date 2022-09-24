//
//  guiBuffer.cpp
//  video-mush
//
//  Created by Josh McNamee on 02/02/2015.
//
//

#include "guiAccessible.hpp"
#include "tagInGui.hpp"
#include "gui.hpp"

using namespace mush;

void guiAccessible::guiTag(int i, tagInGui * tag) {
    tagGui = tag;
    tagInGuiMember = true;
    tagInGuiIndex = i;
    tagGui->gui->setUseExposure(i, tagInGuiUseExposure);
    if (rml.length() > 0) {
        tagGui->gui->addRocketGui(tagInGuiIndex, rml);
    }
}

void guiAccessible::passGuiTag(shared_ptr<mush::guiAccessible> pass) {
    tagInGui * tagg = tagGui;
    int in = tagInGuiIndex;
    tagGui = nullptr;
    tagInGuiIndex = -1;
    tagInGuiMember = false;
    
    pass->guiTag(in, tagg);
}

void guiAccessible::setTagInGuiRecording(bool set) {
    if (tagInGuiMember) {
        tagGui->gui->setRecording(tagInGuiIndex, set);
    }
}

void guiAccessible::inUnlock() {
/*    if (getTagInGuiMember() && tagGui != nullptr) {
        tagGui->copyImageIntoGuiBuffer(getTagInGuiIndex(), (cl::Image2D *)_getMem(next));
    }*/
    ringBuffer::inUnlock();
}

void guiAccessible::rocketGUI(const char *pathToRML) {
    rml = std::string(pathToRML);
}
