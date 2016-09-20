#include <iostream>
#include <chrono>

#include "hwfx3/fx3dev.h"

#ifdef WIN32
#include "hwfx3/fx3devcyapi.h"
#endif

#include "processors/streamdumper.h"

using namespace std;

int main( int argn, const char** argv )
{
#ifndef WIN32
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
#endif

    cerr << "*** Amungo's dumper for nut4nt board ***" << endl << endl;
    if ( argn != 6 ) {
        cerr << "Usage: "
             << "AmungoFx3Dumper"     << " "
             << "FX3_IMAGE"           << " "
             << "NT1065_CFG"          << " "
             << " OUT_FILE | stdout " << " "
             << " SECONDS | inf"      << " "
             << "cypress | libusb"
             << endl << endl;

        cerr << "Use 'stdout'' as a file name to direct signal to standart output stream" << endl;
        cerr << endl;

        cerr << "Example (dumping one minute of signal and use cypress driver):" << endl;
        cerr << "AmungoFx3Dumper AmungoItsFx3Firmware.img  nt1065.hex  dump4ch.bin  60  cypress" << endl;
        cerr << endl;

        cerr << "Example (dumping signal non-stop to stdout):" << endl;
        cerr << "AmungoFx3Dumper AmungoItsFx3Firmware.img  nt1065.hex  stdout  inf  libusb" << endl;
        cerr << endl;
        return 0;
    }

    std::string fximg( argv[1] );
    std::string ntcfg( argv[2] );
    std::string dumpfile( argv[3] );

    double seconds = 0.0;
    const double INF_SECONDS = 10.0 * 365.0 * 24.0 * 60.0 * 60.0;
    if ( string(argv[4]) == string("inf") ) {
        seconds = INF_SECONDS;
    } else {
        seconds = atof( argv[4] );
    }

    std::string driver( argv[5] );

    bool useCypress = ( driver == string( "cypress" ) );

    cerr << "------------------------------" << endl;
    if ( seconds >= INF_SECONDS ) {
        cerr << "Dump non-stop to " << dumpfile << endl;
    } else if ( seconds > 0.0 ) {
        cerr << "Dump " << seconds << " seconds to '" << dumpfile << "'" << endl;
    } else {
        cerr << "No dumping - just testing!" << endl;
    }
    cerr << "Using fx3 image from '" << fximg << "' and nt1065 config from '" << ntcfg << "'" << endl;
    cerr << "Your OS use _" << ( useCypress ? "cypress" : "libusb" ) << "_ driver" << endl;
    cerr << "------------------------------" << endl;

//    char answer = 'n';
//    cerr << "Is this correct? [y/n] ";
//    cin >> answer;

//    if ( answer != 'y' ) {
//        cerr << "Try to run with correct parameters" << endl;
//        return 0;
//    }

    cerr << "Wait while device is being initing..." << endl;
    FX3DevIfce* dev = nullptr;

#ifdef WIN32
    if ( useCypress ) {
        dev = new FX3DevCyAPI();
    } else {
        dev = new FX3Dev( 2 * 1024 * 1024, 7 );
    }
#else
    dev = new FX3Dev( 2 * 1024 * 1024, 7 );
#endif

    if ( dev->init(fximg.c_str(), ntcfg.c_str() ) != FX3_ERR_OK ) {
        cerr << endl << "Problems with hardware or driver type" << endl;
        return -1;
    }
    cerr << "Device was inited." << endl << endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    cerr << "Determinating sample rate";
    if ( !seconds ) {
        cerr << " and USB noise level...";
    }
    cerr << endl;

    dev->startRead(nullptr);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    dev->getDebugInfoFromBoard(false);

    double size_mb = 0.0;
    double phy_errs = 0;
    int sleep_ms = 200;
    int iter_cnt = 20;
    double overall_seconds = ( sleep_ms * iter_cnt ) / 1000.0;
    fx3_dev_debug_info_t info = dev->getDebugInfoFromBoard();
    for ( int i = 0; i < iter_cnt; i++ ) {
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
        info = dev->getDebugInfoFromBoard();
        //info.print();
        cerr << ".";
        size_mb += info.size_tx_mb_inc;
        phy_errs += info.phy_err_inc;
    }
    cerr << endl;

    int64_t CHIP_SR = (int64_t)((size_mb * 1024.0 * 1024.0 )/overall_seconds);

    cerr << endl;
    cerr << "SAMPLE RATE  is ~" << CHIP_SR / 1000000 << " MHz " << endl;
    if ( !seconds ) {
        cerr << "NOISE  LEVEL is  " << phy_errs / size_mb << " noisy packets per one megabyte" << endl;
    }
    cerr << endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));


    const int64_t bytes_per_sample = 1;
    int64_t bytes_to_dump = (int64_t)( CHIP_SR * seconds * bytes_per_sample );
    uint32_t overs_cnt_at_start = info.overflows;

    if ( bytes_to_dump ) {
        cerr << "Start dumping data" << endl;
    } else {
        cerr << "Start testing USB transfer" << endl;
    }
    int32_t iter_time_ms = 5000;
    try {

        StreamDumper dumper( dumpfile, bytes_to_dump );
        if ( bytes_to_dump ) {
            dev->changeHandler(&dumper);
        } else {
            dev->changeHandler(nullptr);
        }

        for ( ;; ) {
            if ( bytes_to_dump ) {
                cerr << "\r";
            } else {
                cerr << endl << "Just testing. Press Ctrl-C to exit.  ";
            }
            if ( bytes_to_dump ) {
                DumperStatus_e status = dumper.GetStatus();
                if ( status == DS_DONE ) {
                    break;
                } else if ( status == DS_ERROR ) {
                    cerr << "Stop because of FILE IO errors" << endl;
                    break;
                }
                cerr << dumper.GetBytesToGo() / ( bytes_per_sample * CHIP_SR ) << " seconds to go. ";
            }

            info = dev->getDebugInfoFromBoard();
            info.overflows -= overs_cnt_at_start;
            cerr << "Overflows count: " << info.overflows << "    ";

            if ( info.overflows_inc ) {
                throw std::runtime_error( "### OVERFLOW DETECTED ON BOARD. DATA SKIP IS VERY POSSIBLE. EXITING ###" );
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(iter_time_ms));
        }
        cerr << endl;

        cerr << "Dump done" << endl;
    } catch ( std::exception& e ){
        cerr << endl << "Error!" << endl;
        cerr << e.what();
    }

    return 0;
}

