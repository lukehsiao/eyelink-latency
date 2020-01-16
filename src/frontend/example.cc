#include <chrono>
#include <cstring>
#include <exception>
#include <iostream>

#include "display.hh"

using namespace std;
using namespace std::chrono;

void program_body()
{
  const unsigned int BOX_DIM = 50;
  bool triggered = false;

  VideoDisplay display { 1920, 1080, true }; // fullscreen window @ 1920x1080 luma resolution
  display.window().hide_cursor( true );
  display.window().set_swap_interval( 1 ); // wait for vertical retrace before swapping buffer

  /* There are 4 textures that are switched between. Until the Eyelink detects a change in eye position, we only have
   * the frame clock in the top left corner. After, we also include a white square at the bottom right.
   */

  /* top left box white (235 = max luma in typical Y'CbCr colorspace) */
  Raster420 clock_white { 1920, 1080 };

  for ( unsigned int y = 0; y < clock_white.Y.height(); y++ ) {
    for ( unsigned int x = 0; x < clock_white.Y.width(); x++ ) {
      const uint8_t color = ( x < BOX_DIM && y < BOX_DIM ) ? 235 : 16;
      clock_white.Y.at( x, y ) = color;
    }
  }

  Texture420 clock_white_texture { clock_white };

  /* all black (16 = min luma in typical Y'CbCr colorspace) */
  Raster420 clock_black { 1920, 1080 };
  memset( clock_black.Y.mutable_pixels(), 16, clock_black.Y.width() * clock_black.Y.height() );
  Texture420 clock_black_texture { clock_black };

  /* top left box white (235 = max luma in typical Y'CbCr colorspace) */
  Raster420 triggered_clock_white { 1920, 1080 };

  for ( unsigned int y = 0; y < triggered_clock_white.Y.height(); y++ ) {
    for ( unsigned int x = 0; x < triggered_clock_white.Y.width(); x++ ) {
      const uint8_t color = ( (x < BOX_DIM && y < BOX_DIM) || 
                              (x < BOX_DIM && y > (triggered_clock_white.Y.height() - BOX_DIM) ) ) ? 235 : 16;
      triggered_clock_white.Y.at( x, y ) = color;
    }
  }

  Texture420 triggered_clock_white_texture { triggered_clock_white };

  /* all black (16 = min luma in typical Y'CbCr colorspace) */
  Raster420 triggered_clock_black { 1920, 1080 };

  for ( unsigned int y = 0; y < triggered_clock_white.Y.height(); y++ ) {
    for ( unsigned int x = 0; x < triggered_clock_white.Y.width(); x++ ) {
      const uint8_t color = ( x < BOX_DIM && y > (triggered_clock_white.Y.height() - BOX_DIM) ) ? 235 : 16;
      triggered_clock_white.Y.at( x, y ) = color;
    }
  }

  Texture420 triggered_clock_black_texture { triggered_clock_black };

  unsigned int frame_count = 0;

  const auto start_time = steady_clock::now();

  while ( true ) {
    /* alternate clock_black and clock_white or, if triggered, triggered_clock_black and triggered_clock_white */
    if ( triggered ) {
      display.draw( clock_white_texture );
    } else {
      display.draw( triggered_clock_white_texture );
    }
    frame_count++;
    if ( triggered ) {
      display.draw( clock_black_texture );
    } else {
      display.draw( triggered_clock_black_texture );
    }
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
