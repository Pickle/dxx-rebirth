/**
 *
 *  Copyright (C) 2011-2012 Scott R. Smith
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 *
 */

#ifndef CBUFFER_H
#define CBUFFER_H

#include "stdint.h"
#include "console.h"
#include "ogl_init.h"

using namespace std;

#define BUFFER_OFFSET(i)    ((uint8_t *)NULL + (i))

class CBuffer
{
    public:
        CBuffer();
        virtual ~CBuffer();

        void    Close   ( void );
        int8_t  Open    ( GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage );
        int8_t  Bind    ( void );
        int8_t  Unbind  ( void );
        void    Data    ( GLintptr offset, GLsizeiptr size, const GLvoid * data );

    private:
        GLenum  Target;
        GLuint  Buffer;
};

#endif // CBUFFER_H
