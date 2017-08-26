// This file was developed by Thomas Müller <thomas94@gmx.net>.
// It is published under the BSD-style license contained in the LICENSE.txt file.

#pragma once

#include "../include/Channel.h"
#include "../include/GlTexture.h"

#include <map>
#include <string>
#include <vector>

class Image {
public:
    Image(const std::string& filename);

    const std::string& name() {
        return mName;
    }

    const Channel* channel(const std::string& channelName) const {
        if (mChannels.count(channelName) == 0) {
            return nullptr;
        }
        return &mChannels.at(channelName);
    }

    const GlTexture* texture(const std::string& channelName);

    const Eigen::Vector2i& size() {
        return mSize;
    }

private:
    void readStbi(const std::string& filename);
    void readExr(const std::string& filename);

    std::string mName;
    Eigen::Vector2i mSize;

    size_t mNumChannels;
    std::map<std::string, Channel> mChannels;
    std::map<std::string, GlTexture> mTextures;
};
