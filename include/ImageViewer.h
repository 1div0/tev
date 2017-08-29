// This file was developed by Thomas Müller <thomas94@gmx.net>.
// It is published under the BSD 3-Clause License within the LICENSE file.

#pragma once

#include "../include/Image.h"
#include "../include/ImageButton.h"
#include "../include/ImageCanvas.h"

#include <nanogui/glutil.h>
#include <nanogui/opengl.h>
#include <nanogui/screen.h>
#include <nanogui/slider.h>

#include <vector>
#include <memory>

TEV_NAMESPACE_BEGIN

class ImageViewer : public nanogui::Screen {
public:
    ImageViewer();

    bool dropEvent(const std::vector<std::string>& filenames) override;

    bool keyboardEvent(int key, int scancode, int action, int modifiers) override;

    void drawContents() override;

    void addImage(std::shared_ptr<Image> image, bool shallSelect = false);

    void selectImage(size_t index);

    size_t layer() {
        return mCurrentLayer;
    }

    void selectLayer(size_t index);
    void selectLayer(std::string name);

    void unselectReference();
    void selectReference(size_t index);

    float exposure() {
        return mExposureSlider->value();
    }

    void setExposure(float value);

    float offset() {
        return mOffsetSlider->value();
    }

    void setOffset(float value);

    void normalizeExposureAndOffset();
    void resetExposureAndOffset();

    ETonemap tonemap() {
        return mImageCanvas->tonemap();
    }

    void setTonemap(ETonemap tonemap);

    EMetric metric() {
        return mImageCanvas->metric();
    }

    void setMetric(EMetric metric);

    void fitAllImages();
    void maximize();
    void updateLayout();

private:
    void updateTitle();
    std::string layerName(size_t index);

    size_t currentImageId() const {
        auto pos = static_cast<size_t>(std::distance(mImages.begin(), find(mImages.begin(), mImages.end(), mCurrentImage)));
        return pos >= mImages.size() ? 0 : pos;
    }

    size_t currentReferenceId() const {
        auto pos = static_cast<size_t>(std::distance(mImages.begin(), find(mImages.begin(), mImages.end(), mCurrentReference)));
        return pos >= mImages.size() ? 0 : pos;
    }

    nanogui::Widget* mVerticalScreenSplit;

    int mFooterHeight = 25;
    nanogui::Widget* mSidebar;

    nanogui::Label* mExposureLabel;
    nanogui::Slider* mExposureSlider;

    nanogui::Label* mOffsetLabel;
    nanogui::Slider* mOffsetSlider;

    nanogui::Widget* mTonemapButtonContainer;
    nanogui::Widget* mMetricButtonContainer;

    std::shared_ptr<Image> mCurrentImage;
    std::shared_ptr<Image> mCurrentReference;

    std::vector<std::shared_ptr<Image>> mImages;
    nanogui::Widget* mImageButtonContainer;
    nanogui::VScrollPanel* mImageScrollContainer;

    ImageCanvas* mImageCanvas;

    nanogui::Widget* mLayerButtonContainer;
    size_t mCurrentLayer = 0;
};

TEV_NAMESPACE_END
