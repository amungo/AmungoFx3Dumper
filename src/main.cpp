#include <iostream>
#include <chrono>
#include <inttypes.h>

#include "IFx3Device.h"
#include "Fx3Factory.h"

#ifdef WIN32
#include "fx3devcyapi.h"
#endif

#include "processors/streamdumper.h"

using namespace std;

#ifdef WIN32
#pragma comment(lib, "legacy_stdio_definitions.lib")
#ifdef __cplusplus
FILE iob[] = { *stdin, *stdout, *stderr };
extern "C" {
    FILE* __cdecl _iob(void) { return iob; }
}
#endif
#endif

int main( int argn, const char** argv )
{
#ifndef WIN32
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
#endif

    cerr << "*** Amungo's dumper for nut4nt board ***" << endl << endl;
    if ( argn < 8 || argn > 9 ) {
        cerr << "Usage: "
             << "AmungoFx3Dumper"     << " "
             << "FX3_IMAGE"           << " "
             << "NT1065_CFG"          << " "
             << "Lattice_ALG"         << " "
             << "Lattice_DATA"        << " "
             << " OUT_FILE | stdout " << " "
             << " SECONDS | inf"      << " "
             << "cypress | libusb"
             << "[ SECONDS check_interval ]"
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
    std::string ecp5alg( argv[3] );
    std::string ecp5data( argv[4] );
    std::string dumpfile( argv[5] );

    double seconds = 0.0;
    const double INF_SECONDS = 10.0 * 365.0 * 24.0 * 60.0 * 60.0;
    if ( string(argv[6]) == string("inf") ) {
        seconds = INF_SECONDS;
    } else {
        seconds = atof( argv[6] );
    }

    std::string driver( argv[7] );

    bool useCypress = ( driver == string( "cypress" ) );

    int dbg_timeout = 0;
    if(argn == 9) {
        dbg_timeout = stoi(argv[8]);
    }

    cerr << "------------------------------" << endl;
    if ( seconds >= INF_SECONDS ) {
        cerr << "Dump non-stop to " << dumpfile << endl;
    } else if ( seconds > 0.0 ) {
        cerr << "Dump " << seconds << " seconds to '" << dumpfile << "'" << endl;
    } else {
        cerr << "No dumping - just testing!" << endl;
    }
    cerr << "Using fx3 image from '" << fximg << "' and nt1065 config from '" << ntcfg << "'" << endl;
    cerr << "Using lattice algorithm from '" << ecp5alg << "' and lattice data from '" << ecp5data << "'" << endl;
    cerr << "You choose to use __" << ( useCypress ? "cypress" : "libusb" ) << "__ driver" << endl;
    cerr << "------------------------------" << endl;

    cerr << "Wait while device is being initing..." << endl;
    IFx3Device* dev = nullptr;

#ifdef WIN32
    if ( useCypress ) {
        dev = Fx3DeviceFactory::CreateInstance(CYAPI_DEVICE_TYPE,  2 * 1024 * 1024, 7);
    } else {
        dev = Fx3DeviceFactory::CreateInstance(LIBUSB_DEVICE_TYPE, 2 * 1024 * 1024, 7);
    }
#else
    if ( useCypress ) {
        cerr << endl
             << "WARNING: you can't use cypress driver under Linux."
             << " Please check if you use correct scripts!"
             << endl;
    }
    //dev = new FX3Dev( 2 * 1024 * 1024, 7 );
    dev = Fx3DeviceFactory::CreateInstance(LIBUSB_DEVICE_TYPE, 2 * 1024 * 1024, 7);

#endif

    if ( dev->init(fximg.c_str(), 0/*ntcfg.c_str()*/ ) != FX3_ERR_OK ) {
        cerr << endl << "Problems with hardware or driver type" << endl;
        dev->Release();
        return -1;
    }

    if (dev->init_fpga(ecp5alg.c_str(), ecp5data.c_str()) != FX3_ERR_OK) {
        cerr << endl << "Problems with loading Lattice firmware (dev->init_fpga)" << endl;
        dev->Release();
        return -1;
    }

    if(dev->load1065Ctrlfile(ntcfg.c_str(), 48) != FX3_ERR_OK) {
        cerr << endl << "Problems with loading nt config file (dev->load1065Ctrlfile) " << endl;
        dev->Release();
        return -1;
    }

    cerr << "Device was inited." << endl << endl;
    //dev->log = false;

    std::this_thread::sleep_for(chrono::milliseconds(1000));

    cerr << "Determinating sample rate";
    if ( !seconds ) {
        cerr << " and USB noise level...";
    }
    cerr << endl;

    dev->startRead(nullptr);

    // This is temporary workaround for strange bug of 'odd launch'
    std::this_thread::sleep_for(chrono::milliseconds(200));
    dev->stopRead();
    std::this_thread::sleep_for(chrono::milliseconds(200));
    dev->startRead(nullptr);


    std::this_thread::sleep_for(chrono::milliseconds(1000));

    dev->getDebugInfoFromBoard(true);

    double size_mb = 0.0;
    double phy_errs = 0;
    int sleep_ms = 200;
    int iter_cnt = 5;
    double overall_seconds = ( sleep_ms * iter_cnt ) / 1000.0;
    fx3_dev_debug_info_t info = dev->getDebugInfoFromBoard(true);
    for ( int i = 0; i < iter_cnt; i++ ) {
        std::this_thread::sleep_for(chrono::milliseconds(sleep_ms));
        info = dev->getDebugInfoFromBoard(true);
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
    std::this_thread::sleep_for(chrono::milliseconds(1000));


    const int64_t bytes_per_sample = 1;
    int64_t bytes_to_dump = (int64_t)( CHIP_SR * seconds * bytes_per_sample );
    uint32_t overs_cnt_at_start = info.overflows;

    if ( bytes_to_dump ) {
        cerr << "Start dumping data" << endl;
    } else {
        cerr << "Start testing USB transfer" << endl;
    }
    StreamDumper* dumper = nullptr;
    int32_t iter_time_ms = 2000;
    thread poller;
    thread err_checker;
    bool poller_running = true;
    bool device_is_ok = true;
    try {

        dumper = new StreamDumper( dumpfile, bytes_to_dump );
        if ( bytes_to_dump ) {
            dev->changeHandler(dumper);
        } else {
            dev->changeHandler(nullptr);
        }

        auto start_time = chrono::system_clock::now();

        poller = thread( [&]() {
            //FILE* flog = fopen( "regdump.txt", "w" );
            while ( poller_running && device_is_ok ) {
                uint8_t wr_val;
                uint8_t rd_val[6];

                for ( int ch = 0; ch < 4 && poller_running; ch++ ) {
                    wr_val = ( ( ch << 4 ) | ( 0x0 << 1 ) | ( 0x1 << 0 ) );
                    fx3_dev_err_t res = FX3_ERR_OK;
                    res = dev->putReceiverRegValue( 0x05, wr_val );
                    if ( res != FX3_ERR_OK ) {
                        cerr << "dev->putReceiverRegValue( 0x05, 0x" << std::hex << (int)wr_val << " ) failed" << endl;
                        this_thread::sleep_for(chrono::milliseconds(100));
                        poller_running = false;
                        device_is_ok = false;
                        break;
                    }

                    do {
                        this_thread::sleep_for(chrono::microseconds(500));
                        res = dev->getReceiverRegValue( 0x05, rd_val[0] );
                        if ( rd_val[0] == 0xff || res != FX3_ERR_OK ) {
                            cerr << "Critical error while registry reading. Is your device is broken?" << endl
                                 << "Try do detach submodule and attach it again" << endl;
                            this_thread::sleep_for(chrono::milliseconds(100));
                            poller_running = false;
                            device_is_ok = false;
                            break;
                        }
                    } while ( ( rd_val[0] & 0x01 ) == 0x01 && res == FX3_ERR_OK && poller_running );
                    //--- cerr << " " << std::hex << (int)rd_val[0] << "--" << std::hex << (int)wr_val << endl; // !!!

                    auto cur_time = chrono::system_clock::now();
                    auto time_from_start = cur_time - start_time;
                    uint64_t ms_from_start = chrono::duration_cast<chrono::milliseconds>(time_from_start).count();

                    if ( res == FX3_ERR_OK ) res = dev->getReceiverRegValue( 0x06, rd_val[1] );
                    if ( res == FX3_ERR_OK ) res = dev->getReceiverRegValue( 0x07, rd_val[2] );
                    if ( res == FX3_ERR_OK ) res = dev->getReceiverRegValue( 0x08, rd_val[3] );
                    if ( res == FX3_ERR_OK ) res = dev->getReceiverRegValue( 0x09, rd_val[4] );
                    if ( res == FX3_ERR_OK ) res = dev->getReceiverRegValue( 0x0A, rd_val[5] );

                    if ( res != FX3_ERR_OK ) {
                        cerr << "Critical error while registry reading. Is your device is broken?" << endl
                             << "Try do detach submodule and attach it again" << endl;
                        this_thread::sleep_for(chrono::milliseconds(100));
                        poller_running = false;
                        device_is_ok = false;
                        break;
                    }

 #if 0
                    fprintf( flog, "%8" PRIu64 " ", ms_from_start);
                    for ( int i = 0; i < 6; i++ ) {
                        fprintf( flog, "%02X ", rd_val[i] );
                        rd_val[i] = 0x00;
                    }
                    fprintf( flog, "\n" );
#endif
                }

            }
//            fclose(flog);
            cerr << "Poller thread finished" << endl;
        });

        if(dbg_timeout) {
            printf("\nStart check errors thread. Check timeout: %d\n", dbg_timeout);

            err_checker = thread( [&](int timeout) {
                //int prev_overflow = 0;

                while(poller_running && device_is_ok) {
                    fx3_dev_debug_info_t info = dev->getDebugInfoFromBoard(false);
                    /*if(info.overflows != prev_overflow || info.phy_errs != 0 || info.lnk_errs != 0)*/ {
                        printf("\n---[request_num:%d ] FX3 overflow:%d, phy_err:%d, lnk_err:%d, phy_err_inc:%d, lnk_err_inc:%d ---\n", info.transfers,
                               info.overflows, info.phy_errs, info.lnk_errs, info.phy_err_inc, info.lnk_err_inc);
                        //prev_overflow = info.overflows;
                    }

                    this_thread::sleep_for(chrono::milliseconds(timeout*1000));
                }
            }, dbg_timeout);
        }

        while ( device_is_ok ) {
            if ( bytes_to_dump ) {
                cerr << "\r";
            } else {
                cerr << endl << "Just testing. Press Ctrl-C to exit.  ";
            }
            if ( bytes_to_dump ) {
                DumperStatus_e status = dumper->GetStatus();
                if ( status == DS_DONE ) {
                    break;
                } else if ( status == DS_ERROR ) {
                    cerr << "Stop because of FILE IO errors" << endl;
                    break;
                }
                cerr << dumper->GetBytesToGo() / ( bytes_per_sample * CHIP_SR ) << " seconds to go. ";
            }

            info = dev->getDebugInfoFromBoard(true);
            info.overflows -= overs_cnt_at_start;
            cerr << "Overflows count: " << info.overflows << "    ";

            if ( info.overflows_inc ) {
                throw std::runtime_error( "### OVERFLOW DETECTED ON BOARD. DATA SKIP IS VERY POSSIBLE. EXITING ###" );
            }

            std::this_thread::sleep_for(chrono::milliseconds(iter_time_ms));
        }
        cerr << endl;

        cerr << "Dump done" << endl;
    } catch ( std::exception& e ){
        cerr << endl << "Error!" << endl;
        cerr << e.what();
    }

    dev->changeHandler(nullptr);

    dev->stopRead();

    poller_running = false;
    if ( poller.joinable() ) {
        poller.join();
    }

    if(dbg_timeout && err_checker.joinable()) {
        err_checker.join();
    }

    dev->Release();
    delete dumper;

    return 0;
}

