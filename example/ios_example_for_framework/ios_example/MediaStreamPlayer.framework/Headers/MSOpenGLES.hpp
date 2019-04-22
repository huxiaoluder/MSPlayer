//
//  MSOpenGLES.hpp
//  MediaStreamPlayer
//
//  Created by xiaoming on 2019/3/12.
//  Copyright © 2019 freecoder. All rights reserved.
//

#ifndef MSOpenGLES_hpp
#define MSOpenGLES_hpp

#include <OpenGLES/ES3/glext.h>
#include "MSMacros.h"

namespace MS {
    namespace OpenGLES {
        
        /**
         加载编译着色器
         
         @param shaderType  着色器类型 GL_XXX_SHADER
         @param shaderStr   着色器内容
         @return 着色器对象(free by caller)
         */
        GLuint loadShader(const GLenum shaderType, const GLchar * MSNonnull const shaderStr);
        
        /**
         链接着色器程序
         
         @param vertexShader    顶点着色器
         @param fragmentShader  片段着色器
         @return 着色器链接程序对象(free by caller)
         */
        GLuint linkProgram(const GLuint vertexShader, const GLuint fragmentShader);
        
        /**
         绑定顶点属性缓冲区, 该函数只绑定 全窗口顶点坐标 和 全窗口纹理坐标, 并启用对应的属性
         
         @return 缓冲区对象(目标类型: GL_ARRAY_BUFFER)
         */
        GLuint bindFullViewportAttrBuffer();
        
        /**
         申请纹理对象, 并做相应的纹理配置
         
         @return 纹理对象(free by caller)
         */
        GLuint generateEmptyTexture2D();
        
        /**
         提交纹理数据到 GPU
         
         @Note: innerformat pixelformat type 共同进行纹理格式调配
         @param texture     纹理对象
         @param innerformat 纹理内部数据格式
         @param pixelformat 像素数据格式
         @param width       像素宽度
         @param height      像素高度
         @param type        像素数据类型
         @param pixels      像素数据
         */
        GLvoid commitTexture2DPixels(const GLuint  texture,
                                     const GLint   innerformat,
                                     const GLenum  pixelformat,
                                     const GLsizei width,
                                     const GLsizei height,
                                     const GLenum  type,
                                     const GLvoid * MSNonnull const pixels);
    
        
        /**
         激活纹理单元,并绑定纹理对象到程序对象中对应的的采样器

         @Note: 激活纹理完成后, 不能解绑 glBindTexture(GL_TEXTURE_2D, 0), 否则无法出图(具体原因待验证)
         @param texture 纹理对象
         @param program 程序对象
         @param textureUnit 纹理单元
         @param samplerName 采样器名
         */
        GLvoid activeTexture2DToProgram(const GLuint texture,
                                        const GLuint program,
                                        const GLenum textureUnit,
                                        const GLchar * MSNonnull const samplerName);
    }
}

#endif /* MSOpenGLES_hpp */
