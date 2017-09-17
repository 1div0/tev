// This file was developed by Thomas Müller <thomas94@gmx.net>.
// It is published under the BSD 3-Clause License within the LICENSE file.

#include <tev/ImageCanvas.h>
#include <tev/ThreadPool.h>

#include <nanogui/theme.h>

#include <numeric>

using namespace Eigen;
using namespace nanogui;
using namespace std;

TEV_NAMESPACE_BEGIN

ImageCanvas::ImageCanvas(nanogui::Widget* parent, float pixelRatio)
: GLCanvas(parent), mPixelRatio(pixelRatio) {
    setDrawBorder(false);
}

bool ImageCanvas::scrollEvent(const Vector2i& p, const Vector2f& rel) {
    if (GLCanvas::scrollEvent(p, rel)) {
        return true;
    }

    scale(rel.y(), p.cast<float>());
    return true;
}

void ImageCanvas::drawGL() {
    if (!mImage) {
        mShader.draw(
            2.0f * mSize.cast<float>().cwiseInverse() / mPixelRatio,
            Vector2f::Constant(20)
        );
        return;
    }

    if (!mReference) {
        mShader.draw(
            2.0f * mSize.cast<float>().cwiseInverse() / mPixelRatio,
            Vector2f::Constant(20),
            mImage->texture(getChannels(*mImage)),
            // The uber shader operates in [-1, 1] coordinates and requires the _inserve_
            // image transform to obtain texture coordinates in [0, 1]-space.
            transform(mImage.get()).inverse().matrix(),
            mExposure,
            mOffset,
            mTonemap
        );
        return;
    }

    mShader.draw(
        2.0f * mSize.cast<float>().cwiseInverse() / mPixelRatio,
        Vector2f::Constant(20),
        mImage->texture(getChannels(*mImage)),
        // The uber shader operates in [-1, 1] coordinates and requires the _inserve_
        // image transform to obtain texture coordinates in [0, 1]-space.
        transform(mImage.get()).inverse().matrix(),
        mReference->texture(getChannels(*mReference)),
        transform(mReference.get()).inverse().matrix(),
        mExposure,
        mOffset,
        mTonemap,
        mMetric
    );
}

void ImageCanvas::draw(NVGcontext *ctx) {
    GLCanvas::draw(ctx);

    if (mImage) {
        auto texToNano = textureToNanogui(mImage.get());
        auto nanoToTex = texToNano.inverse();

        Vector2f pixelSize = texToNano * Vector2f::Ones() - texToNano * Vector2f::Zero();

        Vector2f topLeft = (nanoToTex * Vector2f::Zero());
        Vector2f bottomRight = (nanoToTex * mSize.cast<float>());

        Vector2i startIndices = Vector2i{
            static_cast<int>(floor(topLeft.x())),
            static_cast<int>(floor(topLeft.y())),
        };

        Vector2i endIndices = Vector2i{
            static_cast<int>(ceil(bottomRight.x())),
            static_cast<int>(ceil(bottomRight.y())),
        };

        if (pixelSize.x() > 50) {
            float fontSize = pixelSize.x() / 6;
            float fontAlpha = min(1.0f, (pixelSize.x() - 50) / 30);

            vector<string> channels = getChannels(*mImage);
            // Remove duplicates
            channels.erase(unique(begin(channels), end(channels)), end(channels));

            vector<Color> colors;
            for (const auto& channel : channels) {
                colors.emplace_back(Channel::color(channel));
            }

            nvgFontSize(ctx, fontSize);
            nvgFontFace(ctx, "sans");
            nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

            Vector2i cur;
            vector<float> values;
            for (cur.y() = startIndices.y(); cur.y() < endIndices.y(); ++cur.y()) {
                for (cur.x() = startIndices.x(); cur.x() < endIndices.x(); ++cur.x()) {
                    Vector2i nano = (texToNano * (cur.cast<float>() + Vector2f::Constant(0.5f))).cast<int>();
                    getValuesAtNanoPos(nano, values);

                    TEV_ASSERT(values.size() >= colors.size(), "Can not have more values than channels.");

                    for (size_t i = 0; i < colors.size(); ++i) {
                        string str = tfm::format("%.4f", values[i]);
                        Vector2f pos{
                            mPos.x() + nano.x(),
                            mPos.y() + nano.y() + (i - 0.5f * (values.size() - 1)) * fontSize,
                        };

                        // First draw a shadow such that the font will be visible on white background.
                        nvgFontBlur(ctx, 2);
                        nvgFillColor(ctx, Color(0.0f, fontAlpha));
                        nvgText(ctx, pos.x() + 1, pos.y() + 1, str.c_str(), nullptr);

                        // Actual text.
                        nvgFontBlur(ctx, 0);
                        Color col = colors[i];
                        nvgFillColor(ctx, Color(col.r(), col.g(), col.b(), fontAlpha));
                        nvgText(ctx, pos.x(), pos.y(), str.c_str(), nullptr);
                    }
                }
            }
        }
    }

    // If we're not in fullscreen mode draw an inner drop shadow. (adapted from nanogui::Window)
    if (mPos.x() != 0) {
        int ds = mTheme->mWindowDropShadowSize, cr = mTheme->mWindowCornerRadius;
        NVGpaint shadowPaint = nvgBoxGradient(
            ctx, mPos.x(), mPos.y(), mSize.x(), mSize.y(), cr * 2, ds * 2,
            mTheme->mTransparent, mTheme->mDropShadow
        );

        nvgSave(ctx);
        nvgResetScissor(ctx);
        nvgBeginPath(ctx);
        nvgRect(ctx, mPos.x(), mPos.y(), mSize.x(), mSize.y());
        nvgRoundedRect(ctx, mPos.x() + ds, mPos.y() + ds, mSize.x() - 2 * ds, mSize.y() - 2 * ds, cr);
        nvgPathWinding(ctx, NVG_HOLE);
        nvgFillPaint(ctx, shadowPaint);
        nvgFill(ctx);
        nvgRestore(ctx);
    }
}

void ImageCanvas::translate(const Vector2f& amount) {
    mTransform = Translation2f(amount) * mTransform;
}

void ImageCanvas::scale(float amount, const Vector2f& origin) {
    float scaleFactor = pow(1.1f, amount);

    // Use the current cursor position as the origin to scale around.
    Vector2f offset = -(origin - position().cast<float>()) + 0.5f * mSize.cast<float>();
    auto scaleTransform =
        Translation2f(-offset) *
        Scaling(scaleFactor) *
        Translation2f(offset);

    mTransform = scaleTransform * mTransform;
}

vector<string> ImageCanvas::getChannels(const Image& image) {
    vector<vector<string>> groups = {
        { "R", "G", "B" },
        { "r", "g", "b" },
        { "X", "Y", "Z" },
        { "x", "y", "z" },
        { "U", "V" },
        { "u", "v" },
        { "Z" },
        { "z" },
    };

    string layerPrefix = mRequestedLayer.empty() ? "" : (mRequestedLayer + ".");

    vector<string> result;
    for (const auto& group : groups) {
        for (size_t i = 0; i < group.size(); ++i) {
            const auto& name = layerPrefix + group[i];
            if (image.hasChannel(name)) {
                result.emplace_back(name);
            }
        }

        if (!result.empty()) {
            break;
        }
    }

    string alphaChannelName = layerPrefix + "A";

    // No channels match the given groups; fall back to the first 3 channels.
    if (result.empty()) {
        const auto& channelNames = image.channelsInLayer(mRequestedLayer);
        for (const auto& name : channelNames) {
            if (name != alphaChannelName) {
                result.emplace_back(name);
            }

            if (result.size() >= 3) {
                break;
            }
        }
    }

    // If we found just 1 channel, let's display is as grayscale by duplicating it twice.
    if (result.size() == 1) {
        result.push_back(result[0]);
        result.push_back(result[0]);
    }

    // If there is an alpha layer, use it
    if (image.hasChannel(alphaChannelName)) {
        result.emplace_back(alphaChannelName);
    }

    return result;
}

Vector2i ImageCanvas::getImageCoords(const Image& image, Vector2i mousePos) {
    Vector2f imagePos = textureToNanogui(&image).inverse() * mousePos.cast<float>();
    return {
        static_cast<int>(floor(imagePos.x())),
        static_cast<int>(floor(imagePos.y())),
    };
}

float ImageCanvas::applyMetric(float image, float reference) {
    float diff = image - reference;
    switch (mMetric) {
        case EMetric::Error:                 return diff;
        case EMetric::AbsoluteError:         return abs(diff);
        case EMetric::SquaredError:          return diff * diff;
        case EMetric::RelativeAbsoluteError: return abs(diff) / (reference + 0.01f);
        case EMetric::RelativeSquaredError:  return diff * diff / (reference * reference + 0.01f);
        default:
            throw runtime_error{"Invalid metric selected."};
    }
}

void ImageCanvas::getValuesAtNanoPos(Vector2i mousePos, vector<float>& result) {
    result.clear();
    if (!mImage) {
        return;
    }

    Vector2i imageCoords = getImageCoords(*mImage, mousePos);
    const auto& channels = getChannels(*mImage);

    for (const auto& channel : channels) {
        result.push_back(mImage->channel(channel)->eval(imageCoords));
    }

    // Subtract reference if it exists.
    if (mReference) {
        Vector2i referenceCoords = getImageCoords(*mReference, mousePos);
        const auto& referenceChannels = getChannels(*mReference);
        for (size_t i = 0; i < result.size(); ++i) {
            float reference = i < referenceChannels.size() ?
                mReference->channel(referenceChannels[i])->eval(referenceCoords) :
                0.0f;

            result[i] = applyMetric(result[i], reference);
        }
    }
}

void ImageCanvas::fitImageToScreen(const Image& image) {
    Vector2f nanoguiImageSize = image.size().cast<float>() / mPixelRatio;
    mTransform = Scaling(mSize.cast<float>().cwiseQuotient(nanoguiImageSize).minCoeff());
}

void ImageCanvas::resetTransform() {
    mTransform = Affine2f::Identity();
}

float ImageCanvas::computeMeanValue() {
    if (!mImage) {
        return 0.0f;
    }

    const auto& channels = getChannels(*mImage);
    vector<float> means(channels.size(), 0);

    if (!mReference) {
        ThreadPool pool;
        pool.parallelFor(0, channels.size(), [&](size_t i) {
            const auto* chan = mImage->channel(channels[i]);
            const auto& channelData = chan->data();

            float mean = 0;
            for (size_t j = 0; j < channelData.size(); ++j) {
                mean += channelData[j];
            }

            means[i] = mean / channelData.size();
        });
    } else {
        Vector2i size = mImage->size();
        Vector2i offset = (mReference->size() - size) / 2;
        const auto& referenceChannels = getChannels(*mReference);

        ThreadPool pool;
        pool.parallelFor(0, channels.size(), [&](size_t i) {
            const auto* chan = mImage->channel(channels[i]);

            float mean = 0;
            if (i < referenceChannels.size()) {
                const Channel* referenceChan = mReference->channel(referenceChannels[i]);
                for (int y = 0; y < size.y(); ++y) {
                    for (int x = 0; x < size.x(); ++x) {
                        mean += applyMetric(
                            chan->eval({x, y}),
                            referenceChan->eval({x + offset.x(), y + offset.y()})
                        );
                    }
                }
            } else {
                for (int y = 0; y < size.y(); ++y) {
                    for (int x = 0; x < size.x(); ++x) {
                        mean += applyMetric(chan->eval({x, y}), 0);
                    }
                }
            }

            means[i] = mean / size.y() / size.x();
        });
    }

    return accumulate(begin(means), end(means), 0.0) / means.size();
}

Vector2f ImageCanvas::pixelOffset(const Vector2i& size) const {
    // Translate by half of a pixel to avoid pixel boundaries aligning perfectly with texels.
    // The translation only needs to happen for axes with even resolution. Odd-resolution
    // axes are implicitly shifted by half a pixel due to the centering operation.
    // Additionally, add 0.1111111 such that our final position is almost never 0
    // modulo our pixel ratio, which again avoids aligned pixel boundaries with texels.
    return Vector2f{
        size.x() % 2 == 0 ?  0.5f : 0.0f,
        size.y() % 2 == 0 ? -0.5f : 0.0f,
    } + Vector2f::Constant(0.1111111f);
}

Transform<float, 2, 2> ImageCanvas::transform(const Image* image) {
    if (!image) {
        return Transform<float, 2, 0>::Identity();
    }

    // Center image, scale to pixel space, translate to desired position,
    // then rescale to the [-1, 1] square for drawing.
    return
        Scaling(2.0f / mSize.x(), -2.0f / mSize.y()) *
        mTransform *
        Scaling(1.0f / mPixelRatio) *
        Translation2f(pixelOffset(image->size())) *
        Scaling(image->size().cast<float>()) *
        Translation2f(Vector2f::Constant(-0.5f));
}

Transform<float, 2, 2> ImageCanvas::textureToNanogui(const Image* image) {
    if (!image) {
        return Transform<float, 2, 0>::Identity();
    }

    // Move origin to centre of image, scale pixels, apply our transform, move origin back to top-left.
    return
        Translation2f(0.5f * mSize.cast<float>()) *
        mTransform *
        Scaling(1.0f / mPixelRatio) *
        Translation2f(-0.5f * image->size().cast<float>() + pixelOffset(image->size()));
}

TEV_NAMESPACE_END
