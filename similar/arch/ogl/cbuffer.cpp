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

#include "cbuffer.h"

CBuffer::CBuffer() :
    Target  (GL_ARRAY_BUFFER),
    Buffer  (GL_INVALID_VALUE)
{
}

CBuffer::~CBuffer()
{
}

void CBuffer::Close( void )
{
    if (Buffer != GL_INVALID_VALUE) {
        glDeleteBuffers( 1, &Buffer );
    }

    Target  = GL_ARRAY_BUFFER;
    Buffer  = GL_INVALID_VALUE;
}

int8_t CBuffer::Open( GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage )
{
    Target  = target;

    // Bind the buffer and attach data array
    glGenBuffers( 1, &Buffer );
    if (Bind()) {
        return 1;
    }

    glBufferData( Target, size, data, usage );

    // Unbind the buffer
    if (Unbind()) {
        return 1;
    }

    return 0;
}

int8_t CBuffer::Bind( void )
{
    if (Buffer != GL_INVALID_VALUE) {
        glBindBuffer( Target, Buffer );
    } else {
        con_printf( CON_CRITICAL, "ERROR Vbo: Buffer Error could not bind, buffer index is invalid\n" );
        return 1;
    }

    return 0;
}

int8_t CBuffer::Unbind( void )
{
    if (Buffer != GL_INVALID_VALUE) {
        glBindBuffer( Target, 0 );
    } else {
        con_printf( CON_CRITICAL, "ERROR Vbo: Buffer Error could not unbind, buffer index is invalid\n" );
        return 1;
    }

    return 0;
}

void CBuffer::Data( GLintptr offset, GLsizeiptr size, const GLvoid * data )
{
    Bind();
    glBufferSubData( Target, offset, size, data );
    Unbind();
}
