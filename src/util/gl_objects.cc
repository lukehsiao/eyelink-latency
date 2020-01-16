/* Copyright 2013-2018 the Alfalfa authors
                       and the Massachusetts Institute of Technology

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

      1. Redistributions of source code must retain the above copyright
         notice, this list of conditions and the following disclaimer.

      2. Redistributions in binary form must reproduce the above copyright
         notice, this list of conditions and the following disclaimer in the
         documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

#include <iostream>
#include <memory>
#include <stdexcept>

#include "gl_objects.hh"

using namespace std;

GLFWContext::GLFWContext()
{
  glfwSetErrorCallback( error_callback );

  glfwInit();
}

void GLFWContext::error_callback( const int, const char* const description )
{
  throw runtime_error( description );
}

GLFWContext::~GLFWContext()
{
  glfwTerminate();
}

Window::Window( const unsigned int width, const unsigned int height, const string& title, const bool fullscreen )
  : window_()
{
  glfwDefaultWindowHints();

  glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 );
  glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );
  glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );

  glfwWindowHint( GLFW_RESIZABLE, GL_TRUE );

  window_.reset(
    glfwCreateWindow( width, height, title.c_str(), fullscreen ? glfwGetPrimaryMonitor() : nullptr, nullptr ) );
  if ( not window_.get() ) {
    throw runtime_error( "could not create window" );
  }
}

void Window::make_context_current()
{
  glfwMakeContextCurrent( window_.get() );
  glCheck( "after MakeContextCurrent" );

  glewExperimental = GL_TRUE;
  glewInit();
  glCheck( "after initializing GLEW", true );
}

void Window::hide_cursor( const bool hidden )
{
  glfwSetInputMode( window_.get(), GLFW_CURSOR, hidden ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL );
}

bool Window::key_pressed( const int key ) const
{
  return GLFW_PRESS == glfwGetKey( window_.get(), key );
}

pair<unsigned int, unsigned int> Window::framebuffer_size( void ) const
{
  int width, height;
  glfwGetFramebufferSize( window_.get(), &width, &height );

  if ( width < 0 or height < 0 ) {
    throw runtime_error( "negative framebuffer width or height" );
  }

  return { width, height };
}

pair<unsigned int, unsigned int> Window::window_size() const
{
  int width, height;
  glfwGetWindowSize( window_.get(), &width, &height );

  if ( width < 0 or height < 0 ) {
    throw runtime_error( "negative window width or height" );
  }

  return { width, height };
}

void Window::Deleter::operator()( GLFWwindow* x ) const
{
  glfwHideWindow( x );
  glfwDestroyWindow( x );
}

void Texture::bind( const GLenum texture_unit ) const
{
  glActiveTexture( texture_unit );
  glBindTexture( GL_TEXTURE_RECTANGLE, num_ );
  glTexParameteri( GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
  glTexParameteri( GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
  glTexParameteri( GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
}

void Texture::load( const Plane& plane, const GLenum texture_unit )
{
  if ( plane.width() != width() or plane.height() != height() ) {
    throw runtime_error( "plane's dimensions don't match texture's" );
  }

  bind( texture_unit );

  glPixelStorei( GL_UNPACK_ROW_LENGTH, width_ );
  glTexImage2D( GL_TEXTURE_RECTANGLE, 0, GL_RGBA8, width_, height_, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr );
  glTexSubImage2D(
    GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, width_, height_, GL_LUMINANCE, GL_UNSIGNED_BYTE, &( plane.pixels().at( 0 ) ) );
}

Texture420::Texture420( const Raster420& sample )
  : Y( sample.Y.width(), sample.Y.height() )
  , Cb( sample.Cb.width(), sample.Cb.height() )
  , Cr( sample.Cr.width(), sample.Cr.height() )
{
  bind();

  load( sample );
}

void Texture420::load( const Raster420& raster )
{
  Y.load( raster.Y, GL_TEXTURE0 );
  Cb.load( raster.Cb, GL_TEXTURE1 );
  Cr.load( raster.Cr, GL_TEXTURE2 );
}

void Texture420::bind() const
{
  Y.bind( GL_TEXTURE0 );
  Cb.bind( GL_TEXTURE1 );
  Cr.bind( GL_TEXTURE2 );
}

void compile_shader( const GLuint num, const string& source )
{
  const char* source_c_str = source.c_str();
  glShaderSource( num, 1, &source_c_str, nullptr );
  glCompileShader( num );

  /* check if there were log messages */
  GLint log_length;
  glGetShaderiv( num, GL_INFO_LOG_LENGTH, &log_length );

  if ( log_length > 1 ) {
    unique_ptr<GLchar> buffer( new GLchar[log_length] );
    GLsizei written_length;
    glGetShaderInfoLog( num, log_length, &written_length, buffer.get() );

    if ( written_length + 1 != log_length ) {
      throw runtime_error( "GL shader log size mismatch" );
    }

    cerr << "GL shader compilation log: " << string( buffer.get(), log_length ) << endl;
  }

  /* check if it compiled */
  GLint success;
  glGetShaderiv( num, GL_COMPILE_STATUS, &success );

  if ( not success ) {
    throw runtime_error( "GL shader failed to compile" );
  }
}

GLint Program::attribute_location( const string& name ) const
{
  const GLint ret = glGetAttribLocation( num_, name.c_str() );
  if ( ret < 0 ) {
    throw runtime_error( "attribute not found: " + name );
  }
  return ret;
}

GLint Program::uniform_location( const string& name ) const
{
  const GLint ret = glGetUniformLocation( num_, name.c_str() );
  if ( ret < 0 ) {
    throw runtime_error( "uniform not found: " + name );
  }
  return ret;
}

void glCheck( const string& where, bool ignore )
{
  while ( true ) {
    const GLenum error = glGetError();

    if ( error == GL_NO_ERROR ) {
      break;
    }

    cerr << "GL error " << ( ignore ? "[ignored] " : "" ) << where << ": " << gluErrorString( error ) << endl;

    if ( not ignore ) {
      throw runtime_error( "GL error " + where );
    }

    ignore = false;
  }
}
