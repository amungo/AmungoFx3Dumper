#include <iostream>
#include <chrono>

#include "hwfx3/fx3dev.h"
#include "hwfx3/fx3devcyapi.h"

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
    cout << "Dump " << seconds << " seconds to '" << dumpfile << "'" << endl;
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
    if ( useCypress ) {
        dev = new FX3DevCyAPI();
    } else {
        dev = new FX3Dev();
    }

    if ( dev->init(fximg.c_str(), ntcfg.c_str() ) != FX3_ERR_OK ) {
        cout << endl << "Problems with hardware or driver type" << endl;
        return -1;
    }
    cout << "Device was inited." << endl << endl;

    cout << "Determinating sample rate..." << endl;
    fx3_dev_debug_info_t info1 = dev->getDebugInfoFromBoard(true);
    dev->startRead(nullptr);
    double sleep_seconds = 5.1;
    std::this_thread::sleep_for(std::chrono::milliseconds((int)(1000.0 * sleep_seconds)));
    fx3_dev_debug_info_t info2 = dev->getDebugInfoFromBoard(true);
    size_t CHIP_SR = (size_t)(info2.size_tx_mb_inc - info1.size_tx_mb_inc) * ( 1024.0 * 1024.0 )/sleep_seconds;

    cout << "Sample rate is ~" << CHIP_SR / 1000000 << " MHz " << endl;
    const size_t bytes_per_sample = 1;
    size_t bytes_to_dump = (size_t)( CHIP_SR * seconds * bytes_per_sample );



    cout << "Start dumping data" << endl;


    try {
        StreamDumper dumper( dumpfile, bytes_to_dump );
        dev->changeHandler(&dumper);

        for ( int i = 0; i < 300; i++ ) {
            DumperStatus_e status = dumper.GetStatus();
            if ( status == DS_DONE ) {
                break;
            } else if ( status == DS_ERROR ) {
                cerr << "Stop because of FILE IO errors" << endl;
                break;
            }
            cout << "\r";

            cout << dumper.GetBytesToGo() / ( bytes_per_sample * CHIP_SR ) << " seconds to go      ";

            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        }
        cout << endl;

        cerr << "Dump done" << endl;
    } catch ( std::exception& e ){
        cout << endl << "Error!" << endl;
        cerr << e.what();
    }

    return 0;
}

