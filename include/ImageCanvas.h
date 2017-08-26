// This file was developed by Thomas Müller <thomas94@gmx.net>.
// It is published under the BSD-style license contained in the LICENSE.txt file.

#pragma once

#include "../include/CheckerboardShader.h"
#include "../include/GammaShader.h"
#include "../include/GlTexture.h"
#include "../include/Image.h"

#include <nanogui/glcanvas.h>

#include <memory>

TEV_NAMESPACE_BEGIN

class ImageCanvas : public nanogui::GLCanvas {
public:
    ImageCanvas(nanogui::Widget* parent, float pixelRatio);

    bool mouseMotionEvent(const Eigen::Vector2i& p, const Eigen::Vector2i& rel, int button, int modifiers) override;

    bool scrollEvent(const Eigen::Vector2i& p, const Eigen::Vector2f& rel) override;

    void drawGL() override;

    void setExposure(float exposure) {
        mExposure = exposure;
    }

    void setImage(std::shared_ptr<Image> image) {
        mImage = image;
    }

private:
    // Assembles the transform from canonical space to
    // the [-1, 1] square for the current image.
    Eigen::Matrix3f imageTransform();

    float mPixelRatio = 1;
    float mExposure = 0;
    std::shared_ptr<Image> mImage;
    Eigen::Transform<float, 2, 2> mTransform = Eigen::Affine2f::Identity();

    GammaShader mShader;
    CheckerboardShader mCheckerboardShader;

    GlTexture mTextureBlack;
    GlTexture mTextureWhite;
};

TEV_NAMESPACE_END
