#include <chrono>
#include <cstring>
#include <exception>
#include <iostream>

#include "display.hh"

using namespace std;
using namespace std::chrono;

void program_body()
{
  /* all white */
  Raster420 white { 1920, 1080 };
  memset( white.Y.mutable_pixels(), 235, white.Y.width() * white.Y.height() );

  /* all black */
  Raster420 black { 1920, 1080 };
  memset( black.Y.mutable_pixels(), 16, black.Y.width() * black.Y.height() );

  VideoDisplay display { white, true }; // fullscreen window @ 1920x1080 luma resolution
  display.window().hide_cursor( true );
  display.window().set_swap_interval( 0 ); // don't wait for vertical retrace to swap buffers

  /* alternate black and white */
  unsigned int frame_count = 0;

  const auto start_time = steady_clock::now();

  while ( true ) {
    display.draw( white );
    frame_count++;
    display.draw( black );
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
