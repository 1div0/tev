// This file was developed by Thomas Müller <thomas94@gmx.net>.
// It is published under the BSD 3-Clause License within the LICENSE file.

#include <tev/imageio/StbiLdrImageSaver.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <fstream>
#include <vector>

using namespace Eigen;
using namespace filesystem;
using namespace std;

TEV_NAMESPACE_BEGIN

void StbiLdrImageSaver::save(ofstream& f, const path& path, const vector<char>& data, const Vector2i& imageSize, int nChannels) const {
    static const auto stbiOfstreamWrite = [](void* context, void* data, int size) {
        reinterpret_cast<ofstream*>(context)->write(reinterpret_cast<char*>(data), size);
    };

    auto extension = toLower(path.extension());

    if (extension == "jpg" || extension == "jpeg") {
        stbi_write_jpg_to_func(stbiOfstreamWrite, &f, imageSize.x(), imageSize.y(), nChannels, data.data(), 100);
    } else if (extension == "png") {
        stbi_write_png_to_func(stbiOfstreamWrite, &f, imageSize.x(), imageSize.y(), nChannels, data.data(), 0);
    } else if (extension == "bmp") {
        stbi_write_bmp_to_func(stbiOfstreamWrite, &f, imageSize.x(), imageSize.y(), nChannels, data.data());
    } else if (extension == "tga") {
        stbi_write_tga_to_func(stbiOfstreamWrite, &f, imageSize.x(), imageSize.y(), nChannels, data.data());
    } else {
        throw invalid_argument{tfm::format("Image '%s' has unknown format.", path)};
    }
}

TEV_NAMESPACE_END