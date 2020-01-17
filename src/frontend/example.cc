#include <chrono>
#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>

#include <fcntl.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#include <core_expt.h>
#include <eyelink.h>

#include "display.hh"

#define BOX_DIM 100    /* Dimensions of the white square */
#define DIFF_THRESH 25 /* Abs diff for x or y to change before trigger */
#define SERIAL "/dev/ttyACM0"
#define BAUD B115200
#define NUM_TRIALS 1

using namespace std;
using namespace std::chrono;

/**
 * Setup the serial port interface attributes to 8-bit, no parity, 1 stop bit.
 *
 * @param fd    File descriptor of the serial port.
 * @param speed The baud rate to use.
 */
int set_interface_attribs( int fd, int speed )
{
  struct termios tty;

  if ( !isatty( fd ) ) {
    printf( "fd is not a TTY\n" );
    return -1;
  }

  if ( tcgetattr( fd, &tty ) < 0 ) {
    printf( "Error from tcgetattr: %s\n", strerror( errno ) );
    return -1;
  }

  tty.c_cflag |= CLOCAL | CREAD;
  tty.c_cflag &= ~CSIZE;
  tty.c_cflag |= CS8;      // 8-bit characters
  tty.c_cflag &= ~PARENB;  // no parity bit
  tty.c_cflag &= ~CSTOPB;  // only need 1 stop bit
  tty.c_cflag &= ~CRTSCTS; // no hardware flowcontrol

  tty.c_lflag |= ICANON | ISIG; // canonical input
  tty.c_lflag &= ~( ECHO | ECHOE | ECHONL | IEXTEN );

  tty.c_iflag &= ~IGNCR; // preserve carriage return
  tty.c_iflag &= ~INPCK;
  tty.c_iflag &= ~( INLCR | ICRNL | IUCLC | IMAXBEL );
  tty.c_iflag &= ~( IXON | IXOFF | IXANY ); // no SW flowcontrol

  tty.c_oflag &= ~OPOST;

  tty.c_cc[VEOL] = 0;
  tty.c_cc[VEOL2] = 0;
  tty.c_cc[VEOF] = 0x04;

  if ( cfsetospeed( &tty, speed ) < 0 || cfsetispeed( &tty, speed ) < 0 ) {
    printf( "unable to set correct baud rates.\n" );
    return -1;
  }

  if ( tcsetattr( fd, TCSANOW, &tty ) != 0 ) {
    printf( "Error from tcsetattr: %s\n", strerror( errno ) );
    return -1;
  }
  return 0;
}

/**
 * End recording: adds 100 msec of data to catch final events
 */
static void end_trial( void )
{
  pump_delay( 100 ); // provide a small amount of delay for last data
  stop_recording();
  while ( getkey() ) {
  };
}

int get_tracker_sw_version( char* verstr )
{
  int ln = 0;
  int st = 0;
  ln = strlen( verstr );
  while ( ln > 0 && verstr[ln - 1] == ' ' )
    verstr[--ln] = 0; // trim

  // find the start of the version number
  st = ln;
  while ( st > 0 && verstr[st - 1] != ' ' )
    st--;
  return atoi( &verstr[st] );
}

int initialize_eyelink()
{
  char verstr[50];
  int eyelink_ver = 0;
  int tracker_software_ver = 0;

  // Set the address of the tracker, this is hard-coded by the Eyelink
  set_eyelink_address( (char*)"100.1.1.1" );

  // Initialize the EyeLink DLL and connect to the tracker
  // *  0 opens a connection with the eye tracker
  // *  1 will create a dummy connection for simulation
  // * -1 initializes the DLL but does not open a connection
  if ( open_eyelink_connection( 0 ) )
    return -1;

  set_offline_mode();
  flush_getkey_queue();

  // Now configure tracker for display resolution
  eyecmd_printf( "screen_pixel_coords = %ld %ld %ld %ld", 0, 0, 1920, 1080 );

  eyelink_ver = eyelink_get_tracker_version( verstr );
  if ( eyelink_ver == 3 )
    tracker_software_ver = get_tracker_sw_version( verstr );

  // SET UP TRACKER CONFIGURATION
  // set parser saccade thresholds (conservative settings)
  if ( eyelink_ver >= 2 ) {
    eyecmd_printf( "select_parser_configuration 0" ); // 0 = standard sensitivity
    // turn off scenelink camera stuff
    if ( eyelink_ver == 2 ) {
      eyecmd_printf( "scene_camera_gazemap = NO" );
    }
  } else {
    eyecmd_printf( "saccade_velocity_threshold = 35" );
    eyecmd_printf( "saccade_acceleration_threshold = 9500" );
  }

  // set link data (used for gaze cursor)
  eyecmd_printf( "link_event_filter = LEFT,RIGHT,FIXATION,SACCADE,BLINK,BUTTON,INPUT" );
  eyecmd_printf( "link_sample_data = LEFT,RIGHT,GAZE,GAZERES,AREA,STATUS%s,INPUT",
                 ( tracker_software_ver >= 4 ) ? ",HTARGET" : "" );

  // Make sure we're still alive
  if ( !eyelink_is_connected() || break_pressed() ) {
    return -1;
  }
  return 0;
}

int gc_window_trial( VideoDisplay& display,
                     ofstream& log,
                     int arduino,
                     Texture420& clock_white,
                     Texture420& clock_black,
                     Texture420& triggered_white,
                     Texture420& triggered_black )
{
  static bool toggle = true;

  // Used to track gaze samples
  ALLF_DATA evt;
  float x, y;
  float x_new, y_new;

  // blank the screen
  display.draw( clock_black );

  // Ensure Eyelink has enough time to switch modes
  set_offline_mode();
  pump_delay( 50 );

  // Start data streaming
  // Note that we are ignoring the EDF file.
  int error = start_recording( 0, 0, 1, 1 );
  if ( error != 0 ) {
    return error;
  }

  // wait for link sample data
  if ( !eyelink_wait_for_block_start( 100, 1, 0 ) ) {
    display.draw( clock_black );
    end_trial();
    cerr << "ERROR: No link samples received!\n";
    return TRIAL_ERROR;
  }

  // determine which eye(s) are available
  int eye_used = eyelink_eye_available();

  // reset keys and buttons from tracker
  eyelink_flush_keybuttons( 0 );

  // First, initialize with a single valid sample.
  while ( true ) {
    if ( eyelink_newest_float_sample( NULL ) > 0 ) {
      eyelink_newest_float_sample( &evt );

      x = evt.fs.gx[eye_used];
      y = evt.fs.gy[eye_used];

      // make sure pupil is present
      if ( x != MISSING_DATA && y != MISSING_DATA && evt.fs.pa[eye_used] > 0 ) {
        break;
      }
    }
  }

  // Toggle a couple times just to have clock before the arduino switch
  display.draw( clock_white );
  display.draw( clock_black );
  display.draw( clock_white );
  display.draw( clock_black );

  // Send Arduino the command to switch LEDs
  int wlen = write( arduino, "g", 1 );
  if ( wlen != 1 ) {
    cerr << "[Error] Unable to send to arduino.\n";
    end_trial();
    return TRIAL_ERROR;
  }
  tcdrain( arduino );

  unsigned int frame_count = 0;

  const auto start_time = steady_clock::now();

  // Poll for new samples until the diff between samples is large enough to signify LEDs switched
  while ( true ) {
    // check for new sample update
    if ( eyelink_newest_float_sample( NULL ) > 0 ) {
      eyelink_newest_float_sample( &evt );

      x_new = evt.fs.gx[eye_used];
      y_new = evt.fs.gy[eye_used];

      // make sure pupil is present
      if ( x != MISSING_DATA && y != MISSING_DATA && evt.fs.pa[eye_used] > 0 ) {
        // Only trigger change when there is a large enough diff
        if ( abs( x - x_new ) >= DIFF_THRESH && abs( y - y_new ) >= DIFF_THRESH ) {

          const auto t1 = steady_clock::now();
          const auto sensing_delay = duration_cast<microseconds>( t1 - start_time ).count();
          cout << "Sensor delay " << sensing_delay << " us\n";

          // Draw a couple of the triggered frames and end
          const auto t2 = steady_clock::now();
          display.draw( triggered_black );
          const auto t3 = steady_clock::now();
          const auto drawing_delay = duration_cast<microseconds>( t3 - t2 ).count();
          display.draw( triggered_white );
          display.draw( triggered_black );
          cout << "Drawing delay " << drawing_delay << " us\n";

          // Blocking read call while we wait for the arduino's
          // end-to-end measurement.
          unsigned char buf[64]; // store serial data from Arduino
          int rdlen = read( arduino, buf, sizeof( buf ) - 1 );
          if ( rdlen > 0 ) {
            buf[rdlen] = 0;
            cout << "Read: " << buf << endl;
          } else if ( rdlen < 0 ) {
            cerr << "[Error] Unable to read from Arduino.";
            end_trial();
            return TRIAL_ERROR;
          } else {
            cerr << "Nothing read. EOF?\n";
          }

          // Log results to file
          log << buf << "," << sensing_delay << "," << drawing_delay << endl;

          end_trial();
          return check_record_exit();
        }
      }
    }

    // alternate clock_black and clock_white or, if triggered,
    // triggered_black and triggered_white
    if ( toggle ) {
      display.draw( clock_white );
    } else {
      display.draw( clock_black );
    }
    toggle = !toggle;
    frame_count++;

    if ( frame_count % 480 == 0 ) {
      const auto now = steady_clock::now();

      const auto ms_elapsed = duration_cast<milliseconds>( now - start_time ).count();

      cout << "Drew " << frame_count << " frames in " << ms_elapsed
           << " milliseconds = " << 1000.0 * double( frame_count ) / ms_elapsed << " frames per second.\n";
    }
  }

  // Call this at the end of the trial, to handle special conditions
  return check_record_exit();
}

int run_trials( VideoDisplay& display,
                Texture420& clock_white,
                Texture420& clock_black,
                Texture420& triggered_white,
                Texture420& triggered_black )
{
  // To communicate with arduino over serial
  int arduino;

  // Open the serial port to the Arduino
  arduino = open( (char*)SERIAL, O_RDWR | O_NOCTTY | O_SYNC );
  if ( arduino < 0 ) {
    printf( "Error opening %s: %s\n", SERIAL, strerror( errno ) );
    return ABORT_EXPT;
  }

  // baudrate 115200, 8 bits, no parity, 1 stop bit
  if ( set_interface_attribs( arduino, BAUD ) != 0 ) {
    printf( "Error setting serial interface attribs.\n" );
    close( arduino );
    return ABORT_EXPT;
  }

  // Draw textures once to warm up. This brings subsequent draw times to <1ms.
  display.draw( triggered_white );
  display.draw( triggered_black );
  display.draw( clock_white );
  display.draw( clock_black );

  // Arduino Uno uses DTR line to trigger a reset, so wait for it to boot fully.
  sleep( 5 );

  ofstream log;

  log.open( "results.csv" );
  log << "e2e (us), eyelink (us), drawing (us)\n";

  for ( unsigned int trial = 0; trial < NUM_TRIALS; trial++ ) {
    // abort if link is closed
    if ( eyelink_is_connected() == 0 || break_pressed() ) {
      close( arduino );
      log.close();
      return ABORT_EXPT;
    }

    int i = gc_window_trial( display, log, arduino, clock_white, clock_black, triggered_white, triggered_black );

    // Report errors
    switch ( i ) {
      case ABORT_EXPT: // handle experiment abort or disconnect
        cout << "EXPERIMENT ABORTED\n";
        close( arduino );
        log.close();
        return ABORT_EXPT;
      case REPEAT_TRIAL: // trial restart requested
        cout << "TRIAL REPEATED\n";
        trial--;
        break;
      case SKIP_TRIAL: // skip trial
        cout << "TRIAL ABORTED\n";
        break;
      case TRIAL_OK: // successful trial
        cout << "TRIAL OK\n";
        break;
      default: // other error code
        cout << "TRIAL ERROR\n";
        break;
    }
  }

  // clean up
  close( arduino );
  log.close();

  return 0;
}

void program_body()
{
  VideoDisplay display { 1920, 1080, true }; // fullscreen window @ 1920x1080 luma resolution
  display.window().hide_cursor( true );

  // whether to wait for vertical retrace before swapping buffer
  // *  0 for immediate updates
  // *  1 for updates synchronized with the vertical retrace
  // * -1 for adaptive vsync
  display.window().set_swap_interval( 1 );

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
  Raster420 triggered_white { 1920, 1080 };

  for ( unsigned int y = 0; y < triggered_white.Y.height(); y++ ) {
    for ( unsigned int x = 0; x < triggered_white.Y.width(); x++ ) {
      const uint8_t color =
        ( ( x < BOX_DIM && y < BOX_DIM ) || ( x < BOX_DIM && y > ( triggered_white.Y.height() - BOX_DIM ) ) ) ? 235
                                                                                                              : 16;
      triggered_white.Y.at( x, y ) = color;
    }
  }

  Texture420 triggered_white_texture { triggered_white };

  /* all black (16 = min luma in typical Y'CbCr colorspace) */
  Raster420 triggered_black { 1920, 1080 };

  for ( unsigned int y = 0; y < triggered_black.Y.height(); y++ ) {
    for ( unsigned int x = 0; x < triggered_black.Y.width(); x++ ) {
      const uint8_t color = ( x < BOX_DIM && y > ( triggered_black.Y.height() - BOX_DIM ) ) ? 235 : 16;
      triggered_black.Y.at( x, y ) = color;
    }
  }

  Texture420 triggered_black_texture { triggered_black };

  if ( initialize_eyelink() < 0 ) {
    cerr << "[Error] Unable to initialize EyeLink.\n";
    exit( EXIT_FAILURE );
  }

  run_trials( display, clock_white_texture, clock_black_texture, triggered_white_texture, triggered_black_texture );
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
