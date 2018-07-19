#include <iostream>
#include <iomanip>
#include <chrono>
#include <cstddef>

#include <getopt.h>

#include "hwfx3/fx3dev.h"

#include "processors/streamdumper.h"
#include "processors/socketdumper.h"

using namespace std;

void usage() {
    cerr << "Amungo's dumper for nut4nt board (SW edition)\n";
    cerr << "Usage option:\n";
    cerr << "  --help\n    Prints this help\n";
    cerr << "Output destination:\n";
    cerr << "  --stdout\n    Use stdout for output. This is DEFAULT option\n";
    cerr << "  --file=filename\n    Use file 'filename' for output\n";
    cerr << "  --socket[=NNN]\n    Use TCP local port NNN for output. Default port is 13131\n";
    cerr << "Other options:\n";
    cerr << "  --firmware=filename\n    Use firmware file 'filename' (default is 'fx3sw.img')\n";
    cerr << "  --duration=F.FFF\n    Dump F.FFF seconds of signal\n";
    cerr << "  --verbose\n    Be verbose\n";
    cerr << endl;
}

int main( int argc, char** argv )
{
    enum OutputMode_e {
        HELP_MODE = 0,
        STDOUT_MODE,
        FILE_MODE,
        SOCKET_MODE
    };

    auto mode2str = [](int m) -> const char* {
        switch (m) {
            case (int)HELP_MODE:   return "Help mode";
            case (int)STDOUT_MODE: return "Stdout mode";
            case (int)FILE_MODE:   return "File mode";
            case (int)SOCKET_MODE: return "Socket mode";
        }
        return "No mode";
    };

    int runmode = STDOUT_MODE;
    std::string fximg("fx3sw.img");
    std::string dumpfile("stdout");
    int netport = 13131;
    int64_t duration_ms = 5000;
    int verbose = 0;

    while (true)
    {
        int c;
        static struct option long_options[] =
            {
                /* These options set a flag. */
                {"help",    no_argument,  &runmode, HELP_MODE},
                {"stdout",  no_argument,  &runmode, STDOUT_MODE},
                {"file",    required_argument, nullptr, 'f'},
                {"socket",  optional_argument, nullptr, 'p'},
                {"firmware",optional_argument, nullptr, 'w'},
                {"duration",required_argument, nullptr, 'd'},
                {"verbose", no_argument,       &verbose, 1},
                {nullptr, 0, nullptr, 0}
            };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long(argc, argv, "f:pwd:", long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c)
        {
            case 0:
                /* If this option set a flag, do nothing else now. */
                if (long_options[option_index].flag != 0)
                    break;
                printf ("option %s", long_options[option_index].name);
                if (optarg)
                    printf (" with arg %s", optarg);
                printf ("\n");
                break;
            case 'f':
                dumpfile = std::string(optarg);
                runmode  = FILE_MODE;
                break;
            case 'w':
                fximg = std::string(optarg);
                break;
            case 'p':
                netport = atoi(optarg);
                runmode = SOCKET_MODE;
                break;
            case 'd':
                duration_ms = atof(optarg) * 1000.0;
                break;
            // Error
            case '?': /* getopt_long already printed an error message. */ break;
            default:
                usage();
                return 0;
        }
    }

    if ( runmode == (int)HELP_MODE ) {
        usage();
        return 0;
    }

    if ( verbose ) {
        cerr << "\n";
        cerr << mode2str(runmode) << "\n";
        cerr << "firmware = '" << fximg << "'\n";
        switch (runmode) {
            case (int)FILE_MODE:
                cerr << "Output file = '" << dumpfile << "'\n";
                break;
            case (int)SOCKET_MODE:
                cerr << "Listen port = " << netport << "\n";
                break;
        }
    }

    FX3DevIfce* dev = nullptr;

    dev = new FX3Dev( 2 * 1024 * 1024, 7 );
    dev->log = verbose;

    if ( dev->init(fximg.c_str() ) != FX3_ERR_OK ) {
        cerr << endl << "Problems with hardware or driver type" << endl;
        return -1;
    }
    if ( verbose ) {
        cerr << "\n\n ==== Device was inited ==== \n" << endl;
    }

    StreamDumper* streamDumper = nullptr;
    SocketDumper* socketDumper = nullptr;
    int32_t iter_time_ms = 1000;
    if ( duration_ms > 60000 ) {
        iter_time_ms = 5000;
    } else if ( duration_ms > 10000 ) {
        iter_time_ms = 2000;
    } else if ( duration_ms > 4000 ) {
        iter_time_ms = 1000;
    } else if ( duration_ms > 2000 ) {
        iter_time_ms = 200;
    } else {
        iter_time_ms = 100;
    }
    try {

        DumperIfce* dumperInfoGetter = nullptr;
        if ( runmode == STDOUT_MODE || runmode == FILE_MODE ) {
            streamDumper = new StreamDumper( dumpfile );
            dev->startRead( streamDumper );
            dumperInfoGetter = streamDumper;

        } else if ( runmode == SOCKET_MODE ) {
            socketDumper = new SocketDumper( netport );
            dev->startRead( socketDumper );
            dumperInfoGetter = socketDumper;
        }

        auto start_time = chrono::system_clock::now();
        auto prev_time  = start_time;
        double prev_mb  = 0.0;

        cerr << std::fixed << std::setprecision( 1 );

        while ( true ) {
            std::this_thread::sleep_for(chrono::milliseconds(iter_time_ms));
            auto now_time = chrono::system_clock::now();
            auto delta_all = now_time - start_time;
            auto delta_iter = now_time - prev_time;
            prev_time = now_time;
            StreamDumper::DumpInfo_t info = dumperInfoGetter->GetInfo();
            if ( info.error_count ) {
                cerr << "Stop because of FILE IO errors" << endl;
                break;
            }
            double mb_all  = (double) info.bytes_dumped / ( 1000.0 * 1000.0 );
            double mb_iter = mb_all - prev_mb;
            prev_mb = mb_all;
            double rate_iter_mb = mb_iter / std::chrono::duration<double>(delta_iter).count();
            double rate_all_mb  = mb_all  / std::chrono::duration<double>(delta_all ).count();
            cerr << "[" << std::chrono::duration<double>(delta_all ).count() << " sec] "
                 << mb_all << " MB, "
                 << rate_iter_mb << " (" << rate_all_mb << ") MBps\n";

            if ( delta_all > std::chrono::milliseconds( duration_ms ) ) {
                cerr << "Done" << endl;
                break;
            }
        }
        cerr << endl;

        cerr << "Dump done" << endl;
    } catch ( std::exception& e ){
        cerr << endl << "Error!" << endl;
        cerr << e.what();
    }

    dev->changeHandler(nullptr);

    delete dev;

    if ( streamDumper ) {
        delete streamDumper;
    }
    if ( socketDumper ) {
        delete socketDumper;
    }

    cerr << "Bye" << endl;

    return 0;
}

