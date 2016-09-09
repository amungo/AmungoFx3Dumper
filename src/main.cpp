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

    cout << "*** Amungo's dumper for nut4nt board ***" << endl << endl;
    if ( argn != 6 ) {
        cout << "Usage: "
             << "AmungoFx3Dumper"   << " "
             << "FX3_IMAGE"         << " "
             << "NT1065_CFG"        << " "
             << "OUT_FILE"          << " "
             << "SECONDS"           << " "
             << "cypress | libusb"
             << endl << endl;

        cout << "Example (dumping one minute of signal and use cypress driver):" << endl;
        cout << "AmungoFx3Dumper fx3_nt1065_8bit.img  nt1065.hex  dump4ch.bin  60  cypress" << endl;
        return 0;
    }

    std::string fximg( argv[1] );
    std::string ntcfg( argv[2] );
    std::string dumpfile( argv[3] );
    double seconds = atof( argv[4] );
    std::string driver( argv[5] );

    bool useCypress = ( driver == std::string( "cypress" ) );

    cout << "------------------------------" << endl;
    if ( seconds ) {
        cout << "Dump " << seconds << " seconds to '" << dumpfile << "'" << endl;
    } else {
        cout << "No dumping - just testing!" << endl;
    }
    cout << "Using fx3 image from '" << fximg << "' and nt1065 config from '" << ntcfg << "'" << endl;
    cout << "Your OS use _" << ( useCypress ? "cypress" : "libusb" ) << "_ driver" << endl;
    cout << "------------------------------" << endl;

    char answer = 'n';
    cout << "Is this correct? [y/n] ";
    cin >> answer;

    if ( answer != 'y' ) {
        cout << "Try to run with correct parameters" << endl;
        return 0;
    }

    cout << "Wait while device is being initing..." << endl;
    FX3DevIfce* dev = nullptr;

#ifdef WIN32
    if ( useCypress ) {
        dev = new FX3DevCyAPI();
    } else {
        cout << endl << endl << "!!! WARNING !!!" << endl << "THERE ARE PROBLEMS WITH libusb at this time" << endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
        dev = new FX3Dev();
    }
#else
    dev = new FX3Dev();
#endif

    if ( dev->init(fximg.c_str(), ntcfg.c_str() ) != FX3_ERR_OK ) {
        cout << endl << "Problems with hardware or driver type" << endl;
        return -1;
    }
    cout << "Device was inited." << endl << endl;

    cout << "Determinating sample rate";
    if ( !seconds ) {
        cout << " and USB noise level...";
    }
    cout << endl;

    dev->getDebugInfoFromBoard(false);
    dev->startRead(nullptr);

    double size_mb = 0.0;
    double phy_errs = 0;
    int sleep_ms = 200;
    int iter_cnt = 20;
    double overall_seconds = ( sleep_ms * iter_cnt ) / 1000.0;
    fx3_dev_debug_info_t info = dev->getDebugInfoFromBoard();;
    for ( int i = 0; i < iter_cnt; i++ ) {
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
        info = dev->getDebugInfoFromBoard();
        size_mb += info.size_tx_mb_inc;
        phy_errs += info.phy_err_inc;
    }

    int64_t CHIP_SR = (int64_t)((size_mb * 1024.0 * 1024.0 )/overall_seconds);

    cout << endl;
    cout << "SAMPLE RATE  is ~" << CHIP_SR / 1000000 << " MHz " << endl;
    if ( !seconds ) {
        cout << "NOISE  LEVEL is  " << phy_errs / size_mb << " noisy packets per one megabyte" << endl;
    }
    cout << endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));


    const int64_t bytes_per_sample = 1;
    int64_t bytes_to_dump = (int64_t)( CHIP_SR * seconds * bytes_per_sample );
    uint32_t overs_cnt_at_start = info.overflows;

    if ( bytes_to_dump ) {
        cout << "Start dumping data" << endl;
    } else {
        cout << "Start testing USB transfer" << endl;
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
                cout << "\r";
            } else {
                cout << endl << "Just testing. Press Ctrl-C to exit.  ";
            }
            if ( bytes_to_dump ) {
                DumperStatus_e status = dumper.GetStatus();
                if ( status == DS_DONE ) {
                    break;
                } else if ( status == DS_ERROR ) {
                    cerr << "Stop because of FILE IO errors" << endl;
                    break;
                }
                cout << dumper.GetBytesToGo() / ( bytes_per_sample * CHIP_SR ) << " seconds to go. ";
            }

            info = dev->getDebugInfoFromBoard();
            info.overflows -= overs_cnt_at_start;
            cout << "Overflows count: " << info.overflows << "    ";

            if ( info.overflows_inc ) {
                throw std::runtime_error( "### OVERFLOW DETECTED ON BOARD. DATA SKIP IS VERY POSSIBLE. EXITING ###" );
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(iter_time_ms));
        }
        cout << endl;

        cerr << "Dump done" << endl;
    } catch ( std::exception& e ){
        cout << endl << "Error!" << endl;
        cerr << e.what();
    }

    return 0;
}

