#include <chrono>
#include <cstring>
#include <exception>
#include <iostream>

#include "display.hh"

using namespace std;
using namespace std::chrono;

void program_body()
{
  VideoDisplay display { 1920, 1080, true }; // fullscreen window @ 1920x1080 luma resolution
  display.window().hide_cursor( true );
  display.window().set_swap_interval( 1 ); // wait for vertical retrace before swapping buffer

  /* all white (235 = max luma in typical Y'CbCr colorspace */
  Raster420 white { 1920, 1080 };
  memset( white.Y.mutable_pixels(), 235, white.Y.width() * white.Y.height() );
  Texture420 white_texture { white };

  /* left half white */
  Raster420 left_white { 1920, 1080 };

  for ( unsigned int y = 0; y < left_white.Y.height(); y++ ) {
    for ( unsigned int x = 0; x < left_white.Y.width(); x++ ) {
      const uint8_t color = ( x < left_white.Y.width() / 2 ) ? 235 : 16;
      left_white.Y.at( x, y ) = color;
    }
  }

  Texture420 left_white_texture { left_white };

  /* all black (16 = min luma in typical Y'CbCr colorspace */
  Raster420 black { 1920, 1080 };
  memset( black.Y.mutable_pixels(), 16, black.Y.width() * black.Y.height() );
  Texture420 black_texture { black };

  /* alternate black and white */
  unsigned int frame_count = 0;

  const auto start_time = steady_clock::now();

  while ( true ) {
    display.draw( left_white_texture );
    frame_count++;
    display.draw( white_texture );
    frame_count++;
    display.draw( black_texture );
    frame_count++;

    if ( frame_count % 480 == 0 ) {
      const auto now = steady_clock::now();

      const auto ms_elapsed = duration_cast<milliseconds>( now - start_time ).count();

      cout << "Drew " << frame_count << " frames in " << ms_elapsed
           << " milliseconds = " << 1000.0 * double( frame_count ) / ms_elapsed << " frames per second.\n";
    }
  }
}

int main()
{
  try {
    program_body();
  } catch ( const exception& e ) {
    cerr << "Exception: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
