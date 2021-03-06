/**
  \file G3D-gfx.lib/include/G3D-gfx/BufferTexture.h

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/

#ifndef G3D_app_BufferTexture_h
#define G3D_app_BufferTexture_h

namespace G3D {

class GLPixelTransferBuffer;

/**
 \brief A 1D array stored on the GPU, backed by a buffer object. 
  Commonly used as storage for GL Compute data.

 Abstraction of OpenGL buffer textures. Buffer textures have a different size limit than normal
 1D textures, have a special creation path, and are more or less special cases in every way.
 Instead of shoehorning them into G3D::Texture, we created this lightweight wrapper. This 
 decision may be revisited in the near future, and the functionality rolled into G3D::Texture (or G3D::GLPixelTransferBuffer).


 It is valid to create a buffer texture from a buffer with a larger number of texels than 
 GL_MAX_TEXTURE_BUFFER_SIZE, but one can only access the first GL_MAX_TEXTURE_BUFFER_SIZE texels 
 from the buffer texture. Use someTexelsInaccessible() to check if there are inaccessible texels 
 in this object because of that. 

 Currently only allowed to be created from a GLPixelTransferBuffer... this will change in the near future

  

  \sa G3D::Texture
 */
class BufferTexture {
private:

    /** OpenGL texture ID */
    GLuint  m_textureID;
    shared_ptr<GLPixelTransferBuffer> m_buffer;

    String m_name;

    BufferTexture(const String& name, const shared_ptr<GLPixelTransferBuffer>& buffer, GLuint textureID);

public:

    static shared_ptr<BufferTexture> create(const String& name, const shared_ptr<GLPixelTransferBuffer>& buffer);

    ~BufferTexture();

    const ImageFormat* format() const;

    /** The underlying OpenGL Pixel Buffer Object */
    shared_ptr<GLPixelTransferBuffer> buffer() {
        return m_buffer;
    }

    /** Number of pixels (underlying buffer width x height) */
    int size() const;

    /** Returns true if the buffer associated with this texture has more texels than GL_MAX_TEXTURE_BUFFER_SIZE.
        This is valid, but the texels out of this range will be inaccessible in shaders. */
    bool someTexelsInaccessible() const;

    /** The sampler type this type of data corresponds to... used for binding to shaders */
    GLenum glslSamplerType() const;

    /**
     The OpenGL texture target this binds to (always GL_TEXTURE_BUFFER)
     */
    unsigned int openGLTextureTarget() const;

    inline unsigned int openGLID() const {
        return m_textureID;
    }

};

} // namespace G3D

#endif
