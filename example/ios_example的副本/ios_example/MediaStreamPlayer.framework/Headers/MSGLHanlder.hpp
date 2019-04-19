//
//  MSGLHanlder.hpp
//  MediaStreamPlayer
//
//  Created by xiaoming on 2019/3/1.
//  Copyright © 2019 freecoder. All rights reserved.
//

#ifndef MSGLHanlder_hpp
#define MSGLHanlder_hpp

#include "MSOpenGLES.hpp"

namespace MS {
    
    class MSGLHanlder {
        
        GLuint program;     // 二进制着色器程序对象
        GLuint attrBuffer;  // 顶点属性的缓冲区对象
        GLuint yTexture;    // Y 纹理对象
        GLuint uTexture;    // Cb纹理对象
        GLuint vTexture;    // Cr纹理对象
        
    public:
        MSGLHanlder(const char * MSNonnull const vshFilePath,
                    const char * MSNonnull const fshFilePath);
        
        ~MSGLHanlder();
        
        GLuint getProgram() const;
        
        GLuint getYtexture() const;
        
        GLuint getUtexture() const;
        
        GLuint getVtexture() const;
        
    };

}

#endif /* MSGLHanlder_hpp */
