// This file was developed by Thomas Müller <thomas94@gmx.net>.
// It is published under the BSD-style license contained in the LICENSE.txt file.

#pragma once

#include "../include/Image.h"
#include "../include/ImageButton.h"
#include "../include/ImageCanvas.h"

#include <nanogui/screen.h>
#include <nanogui/opengl.h>
#include <nanogui/glutil.h>

#include <vector>
#include <memory>

TEV_NAMESPACE_BEGIN

class ImageViewer : public nanogui::Screen {
public:
    ImageViewer();

    bool dropEvent(const std::vector<std::string>& filenames) override;

    bool keyboardEvent(int key, int scancode, int action, int modifiers) override;

    void draw(NVGcontext *ctx) override;

    void addImage(std::shared_ptr<Image> image, bool shallSelect = false);

    void selectImage(size_t index);

    float exposure();
    void setExposure(float value);

    void fitAllImages();

private:

    struct ImageInfo {
        std::shared_ptr<Image> image;
        nanogui::ref<ImageButton> button;
    };

    int mMenuWidth = 200;

    nanogui::Label* mExposureLabel;
    nanogui::Slider* mExposureSlider;

    size_t mCurrentImage = 0;
    std::vector<ImageInfo> mImageInfos;
    nanogui::Widget* mImageButtonContainer;
    nanogui::VScrollPanel* mImageScrollContainer;

    ImageCanvas* mImageCanvas;
};

TEV_NAMESPACE_END
