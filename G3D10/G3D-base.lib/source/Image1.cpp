/**
  \file G3D-base.lib/source/Image1.cpp

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/


#include "G3D-base/Image1.h"
#include "G3D-base/Image1unorm8.h"
#include "G3D-base/Image.h"
#include "G3D-base/Color4.h"
#include "G3D-base/Color4unorm8.h"
#include "G3D-base/Color1.h"
#include "G3D-base/Color1unorm8.h"
#include "G3D-base/ImageFormat.h"
#include "G3D-base/PixelTransferBuffer.h"
#include "G3D-base/CPUPixelTransferBuffer.h"

namespace G3D {

Image1::Image1(int w, int h, WrapMode wrap) : Map2D<Color1, Color1>(w, h, wrap) {
    setAll(Color1(0.0f));
}


shared_ptr<Image1> Image1::fromImage1unorm8(const shared_ptr<Image1unorm8>& im) {
    shared_ptr<Image1> out = createEmpty(static_cast<WrapMode>(im->wrapMode()));
    out->resize(im->width(), im->height());

    int N = im->width() * im->height();
    const Color1unorm8* src = reinterpret_cast<Color1unorm8*>(im->getCArray());
    for (int i = 0; i < N; ++i) {
        out->data[i] = Color1(src[i]);
    }

    return out;
}


shared_ptr<Image1> Image1::createEmpty(int width, int height, WrapMode wrap) {
    return shared_ptr<Image1>(new Type(width, height, wrap));
}


shared_ptr<Image1> Image1::createEmpty(WrapMode wrap) {
    return createEmpty(0, 0, wrap);
}


shared_ptr<Image1> Image1::fromFile(const String& filename, WrapMode wrap) {
    shared_ptr<Image1> out = createEmpty(wrap);
    out->load(filename);
    return out;
}


void Image1::load(const String& filename) {
    shared_ptr<Image> image = Image::fromFile(filename);
    if (image->format() != ImageFormat::L32F()) {
        image->convertToL8();
    }

    switch (image->format()->code)
    {
        case ImageFormat::CODE_L8:
        case ImageFormat::CODE_R8:
            copyArray(static_cast<const Color1unorm8*>(dynamic_pointer_cast<CPUPixelTransferBuffer>(image->toPixelTransferBuffer())->buffer()), image->width(), image->height());
            break;
        case ImageFormat::CODE_L32F:
            copyArray(static_cast<const Color1*>(dynamic_pointer_cast<CPUPixelTransferBuffer>(image->toPixelTransferBuffer())->buffer()), image->width(), image->height());
            break;
        case ImageFormat::CODE_RGB8:
            copyArray(static_cast<const Color3unorm8*>(dynamic_pointer_cast<CPUPixelTransferBuffer>(image->toPixelTransferBuffer())->buffer()), image->width(), image->height());
            break;
        case ImageFormat::CODE_RGB32F:
            copyArray(static_cast<const Color3*>(dynamic_pointer_cast<CPUPixelTransferBuffer>(image->toPixelTransferBuffer())->buffer()), image->width(), image->height());
            break;
        case ImageFormat::CODE_RGBA8:
            copyArray(static_cast<const Color4unorm8*>(dynamic_pointer_cast<CPUPixelTransferBuffer>(image->toPixelTransferBuffer())->buffer()), image->width(), image->height());
            break;
        case ImageFormat::CODE_RGBA32F:
            copyArray(static_cast<const Color4*>(dynamic_pointer_cast<CPUPixelTransferBuffer>(image->toPixelTransferBuffer())->buffer()), image->width(), image->height());
            break;
        default:
            debugAssertM(false, "Trying to load unsupported image format");
            break;
    }

    setChanged(true);
}


shared_ptr<Image1> Image1::fromArray(const class Color3unorm8* ptr, int w, int h, WrapMode wrap) {
    shared_ptr<Image1> out = createEmpty(wrap);
    out->copyArray(ptr, w, h);
    return out;
}


shared_ptr<Image1> Image1::fromArray(const class Color1* ptr, int w, int h, WrapMode wrap) {
    shared_ptr<Image1> out = createEmpty(wrap);
    out->copyArray(ptr, w, h);
    return out;
}


shared_ptr<Image1> Image1::fromArray(const class Color1unorm8* ptr, int w, int h, WrapMode wrap) {
    shared_ptr<Image1> out = createEmpty(wrap);
    out->copyArray(ptr, w, h);
    return out;
}


shared_ptr<Image1> Image1::fromArray(const class Color3* ptr, int w, int h, WrapMode wrap) {
    shared_ptr<Image1> out = createEmpty(wrap);
    out->copyArray(ptr, w, h);
    return out;
}


shared_ptr<Image1> Image1::fromArray(const class Color4unorm8* ptr, int w, int h, WrapMode wrap) {
    shared_ptr<Image1> out = createEmpty(wrap);
    out->copyArray(ptr, w, h);
    return out;
}


shared_ptr<Image1> Image1::fromArray(const class Color4* ptr, int w, int h, WrapMode wrap) {
    shared_ptr<Image1> out = createEmpty(wrap);
    out->copyArray(ptr, w, h);
    return out;
}

void Image1::copyArray(const Color3unorm8* src, int w, int h) {
    resize(w, h);

    int N = w * h;
    Color1* dst = data.getCArray();
    // Convert int8 -> float
    for (int i = 0; i < N; ++i) {
        dst[i] = Color1(Color3(src[i]).average());
    }
}


void Image1::copyArray(const Color4unorm8* src, int w, int h) {
    resize(w, h);

    int N = w * h;
    Color1* dst = data.getCArray();
    
    // Strip alpha and convert
    for (int i = 0; i < N; ++i) {
        dst[i] = Color1(Color3(src[i].rgb()).average());
    }
}


void Image1::copyArray(const Color1* src, int w, int h) {
    resize(w, h);
    System::memcpy(getCArray(), src, w * h * sizeof(Color1));
}


void Image1::copyArray(const Color4* src, int w, int h) {
    resize(w, h);

    int N = w * h;
    Color1* dst = data.getCArray();
    
    // Strip alpha
    for (int i = 0; i < N; ++i) {
        dst[i] = Color1(src[i].rgb().average());
    }
}


void Image1::copyArray(const Color1unorm8* src, int w, int h) {
    resize(w, h);
    int N = w * h;

    Color1* dst = getCArray();
    for (int i = 0; i < N; ++i) {
        dst[i]= Color1(src[i]);
    }
}


void Image1::copyArray(const Color3* src, int w, int h) {
    resize(w, h);
    int N = w * h;

    Color1* dst = getCArray();
    for (int i = 0; i < N; ++i) {
        dst[i] = Color1(src[i].average());
    }
}


void Image1::save(const String& filename) {
    // To avoid saving as floating point image.  FreeImage cannot convert floating point to L8.
    shared_ptr<Image1unorm8> unorm8 = Image1unorm8::fromImage1(dynamic_pointer_cast<Image1>(shared_from_this()));
    unorm8->save(filename);
}


const ImageFormat* Image1::format() const {
    return ImageFormat::L32F();
}

} // G3D
