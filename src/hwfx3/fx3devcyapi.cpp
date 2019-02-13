#include "fx3devcyapi.h"
#ifndef NO_CY_API

#include <sstream>

#include "pointdrawer.h"
#include "HexParser.h"
#include "host_commands.h"

#include "lattice/lfe5u_core.h"
#include "lattice/lfe5u_opcode.h"

FX3DevCyAPI::FX3DevCyAPI() :
    data_handler( NULL ),
    last_overflow_count( 0 ),
    size_tx_mb( 0.0 )
{
     m_SSPICore = std::shared_ptr<SSPICore>(new SSPICore(this));
}

FX3DevCyAPI::~FX3DevCyAPI() {
    stopRead();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    fprintf( stderr, "Reseting chip\n" );


    if ( resetECP5() == FX3_ERR_OK) {
        fprintf( stderr, "Going to reset lattice. Please wait\n" );
    }
    else {
        fprintf( stderr, "__error__ LATTICE CHIP RESET failed\n" );
    }

    if ( device_reset() == FX3_ERR_OK ) {
        fprintf( stderr, "Fx3 is going to reset. Please wait\n" );
        for ( int i = 0; i < 20; i++ ) {
            std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
            fprintf( stderr, "*" );
        }
        fprintf( stderr, "reinit done\n" );
    } else {
        fprintf( stderr, "__error__ FX3 CHIP RESET failed. Do you use last firmware?\n" );
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

fx3_dev_err_t FX3DevCyAPI::init(const char *firmwareFileName, const char *additionalFirmwareFileName) {
    StartParams.USBDevice = new CCyFX3Device();
    int boot = 0;
    int stream = 0;
    if(log)
        fprintf(stderr, "Enter in FX3DevCyAPI::init \n");

    fx3_dev_err_t res = scan( boot, stream );
    if ( res != FX3_ERR_OK ) {
            fprintf(stderr, " Error to scan(boot, stream)");
        return res;
    }

    bool need_fw_load = stream == 0 && boot > 0;

    if ( need_fw_load ) {
        if ( log ) fprintf( stderr, "FX3DevCyAPI::init() fw load needed\n" );

        FILE* f = fopen( firmwareFileName, "r" );
        if ( !f ) {
            fprintf(stderr, "Error to open firmware file \n");
            return FX3_ERR_FIRMWARE_FILE_IO_ERROR;
        }

        if ( StartParams.USBDevice->IsBootLoaderRunning() ) {
            int retCode  = StartParams.USBDevice->DownloadFw((char*)firmwareFileName, FX3_FWDWNLOAD_MEDIA_TYPE::RAM);

            if ( retCode != FX3_FWDWNLOAD_ERROR_CODE::SUCCESS ) {
                fprintf(stderr ,"Error to load firmware \n");
                switch( retCode ) {
                    case INVALID_FILE:
                    case CORRUPT_FIRMWARE_IMAGE_FILE:
                    case INCORRECT_IMAGE_LENGTH:
                    case INVALID_FWSIGNATURE:
                        return FX3_ERR_FIRMWARE_FILE_CORRUPTED;
                    default:
                        return FX3_ERR_CTRL_TX_FAIL;
                }
            } else {
                if ( log ) fprintf( stderr, "FX3DevCyAPI::init() boot ok!\n" );
            }
        } else {
            fprintf( stderr, "__error__ FX3DevCyAPI::init() StartParams.USBDevice->IsBootLoaderRunning() is FALSE\n" );
            return FX3_ERR_BAD_DEVICE;
        }
        last_overflow_count = 0;
    }

    if ( need_fw_load ) {
        int PAUSE_AFTER_FLASH_SECONDS = 2;
        if ( log ) fprintf( stderr, "FX3DevCyAPI::Init() flash completed!\nPlease wait for %d seconds\n", PAUSE_AFTER_FLASH_SECONDS );
        for ( int i = 0; i < PAUSE_AFTER_FLASH_SECONDS * 2; i++ ) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            fprintf( stderr, "*" );
        }
        fprintf( stderr, "\n" );

        delete StartParams.USBDevice;
        StartParams.USBDevice = new CCyFX3Device();
        res = scan( boot, stream );
        if ( res != FX3_ERR_OK ) {
            fprintf(stderr, "Error in second scan(boot, stream) \n");
            return res;
        }
    }

    res = prepareEndPoints();
    if ( res != FX3_ERR_OK ) {
        fprintf(stderr, "Error in prepareEndPoints() \n");
        return res;
    }

#if 0
    if ( additionalFirmwareFileName != NULL ) {
        if ( additionalFirmwareFileName[ 0 ] != 0 ) {

            PointDrawer drawer(100);

            res = loadAdditionalFirmware( additionalFirmwareFileName, 48 );
            if ( res != FX3_ERR_OK ) {
                fprintf( stderr, "FX3DevCyAPI::Init() __error__ loadAdditionalFirmware %d %s\n", res, fx3_get_error_string( res ) );
                return res;
            }
        }
    }
#endif

    bool In;
    int Attr, MaxPktSize, MaxBurst, Interface, Address;
    int EndPointsCount = EndPointsParams.size();
    for(int i = 0; i < EndPointsCount; i++){
        GetEndPointParamsByInd(i, &Attr, &In, &MaxPktSize, &MaxBurst, &Interface, &Address);
        if ( log ) fprintf( stderr, "FX3DevCyAPI::Init() EndPoint[%d], Attr=%d, In=%d, MaxPktSize=%d, MaxBurst=%d, Infce=%d, Addr=%d\n",
                 i, Attr, In, MaxPktSize, MaxBurst, Interface, Address);
    }

#if 0
    print_version();
#endif

    return res;
}


fx3_dev_err_t FX3DevCyAPI::init_fpga(const char* algoFileName, const char* dataFileName)
{
    fx3_dev_err_t retCode = FX3_ERR_OK;
    int siRetCode = m_SSPICore->SSPIEm_preset( algoFileName, dataFileName);
    siRetCode = m_SSPICore->SSPIEm(0xFFFFFFFF);

    retCode = (siRetCode == PROC_OVER) ? FX3_ERR_OK : FX3_ERR_FPGA_DATA_FILE_IO_ERROR;

    if(retCode == FX3_ERR_OK)
    {
        // Set DAC
        retCode = send24bitSPI8bit(0x000AFFFF<<4);
        retCode = device_stop();
        retCode = reset_nt1065();
    }

    return retCode;
}


void FX3DevCyAPI::startRead(DeviceDataHandlerIfce *handler) {
    size_tx_mb = 0.0;
    startTransferData(0, 128, 64, 1500);
    data_handler = handler;
    xfer_thread = std::thread(&FX3DevCyAPI::xfer_loop, this);

    device_start();
}

void FX3DevCyAPI::stopRead() {
    StartParams.bStreaming = false;
    if ( xfer_thread.joinable() ) {
        if ( log ) fprintf( stderr, "FX3DevCyAPI::stopRead(). xfer_thread.join()\n" );
        xfer_thread.join();
    }
    changeHandler( NULL );
    if ( log ) fprintf( stderr, "FX3DevCyAPI::stopRead() all done!\n" );

    device_stop();
}

void FX3DevCyAPI::changeHandler(DeviceDataHandlerIfce *handler) {
    std::lock_guard<std::mutex> guard(handler_mtx);
    data_handler = handler;
}

void FX3DevCyAPI::sendAttCommand5bits(uint32_t bits) {
    std::lock_guard<std::mutex> critical_section(this->mtx);

    UCHAR buf[16];
    buf[0]=(UCHAR)(bits);

    if ( log ) fprintf( stderr, "FX3DevCyAPI::sendAttCommand5bits() %0x02X\n", bits );
     
    long len=16;
    int success=StartParams.USBDevice->BulkOutEndPt->XferData(&buf[0],len);
    
    if ( !success ) {
        fprintf( stderr, "__error__ FX3DevCyAPI::sendAttCommand5bits() BulkOutEndPt->XferData return FALSE\n" );
    }
    
}

fx3_dev_debug_info_t FX3DevCyAPI::getDebugInfoFromBoard(bool ask_speed_only) {
    std::lock_guard<std::mutex> critical_section(this->mtx);

    if ( ask_speed_only ) {
        fx3_dev_debug_info_t info;
        info.status = FX3_ERR_OK;
        info.size_tx_mb_inc = size_tx_mb;
        size_tx_mb = 0.0;
        return info;
    } else {
        UCHAR buf[32];
        std::fill( &buf[0], &buf[31], 0 );

        CCyControlEndPoint* CtrlEndPt;
        CtrlEndPt = StartParams.USBDevice->ControlEndPt;
        CtrlEndPt->Target    = TGT_DEVICE;
        CtrlEndPt->ReqType   = REQ_VENDOR;
        CtrlEndPt->Direction = DIR_FROM_DEVICE;
        CtrlEndPt->ReqCode = 0xB4;
        CtrlEndPt->Value = 0;
        CtrlEndPt->Index = 1;
        long len=32;
        int success = CtrlEndPt->XferData( &buf[0], len );
        unsigned int* uibuf = ( unsigned int* ) &buf[0];

        fx3_dev_debug_info_t info;
        if ( !success ) {
            fprintf( stderr, "FX3DevCyAPI::getDebugInfoFromBoard() __error__  CtrlEndPt->XferData is FALSE\n" );
            info.status = FX3_ERR_CTRL_TX_FAIL;
        } else {
            info.status = FX3_ERR_OK;
        }

        info.transfers   = uibuf[ 0 ];
        info.overflows   = uibuf[ 1 ];
        info.phy_errs    = uibuf[ 2 ];
        info.lnk_errs    = uibuf[ 3 ];
        info.err_reg_hex = uibuf[ 4 ];
        info.phy_err_inc = uibuf[ 5 ];
        info.lnk_err_inc = uibuf[ 6 ];

        info.size_tx_mb_inc = size_tx_mb;
        size_tx_mb = 0.0;

        info.overflows_inc = info.overflows - last_overflow_count;
        last_overflow_count = info.overflows;
        return info;
    }
}

fx3_dev_err_t FX3DevCyAPI::getReceiverRegValue(uint8_t addr, uint8_t &value) {
    return read16bitSPI_ECP5( addr, &value );
}

fx3_dev_err_t FX3DevCyAPI::putReceiverRegValue(uint8_t addr, uint8_t value) {
    return send16bitSPI_ECP5(addr, value);
}

fx3_dev_err_t FX3DevCyAPI::reset() {
    UCHAR buf[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    LONG len = 16;
    int success = 0;

    CCyControlEndPoint* CtrlEndPt;

    CtrlEndPt = StartParams.USBDevice->ControlEndPt;
    if(CtrlEndPt != 0)
    {
        CtrlEndPt->Target = TGT_DEVICE;
        CtrlEndPt->ReqType = REQ_VENDOR;
        CtrlEndPt->Direction = DIR_TO_DEVICE;
        CtrlEndPt->ReqCode = CMD_CYPRESS_RESET;
        CtrlEndPt->Value = 0;
        CtrlEndPt->Index = 1;
        success = CtrlEndPt->XferData(&buf[0], len);
    }
    return success ? FX3_ERR_OK : FX3_ERR_CTRL_TX_FAIL;
}




fx3_dev_err_t FX3DevCyAPI::print_version() {
    //if ( log ) fprintf( stderr, "FX3Dev::read16bitSPI() from  0x%02X\n", addr );

    CCyControlEndPoint* CtrlEndPt;
    CtrlEndPt = StartParams.USBDevice->ControlEndPt;
    CtrlEndPt->Target = TGT_DEVICE;
    CtrlEndPt->ReqType = REQ_VENDOR;
    CtrlEndPt->Direction = DIR_FROM_DEVICE;
    CtrlEndPt->ReqCode = CMD_GET_VERSION;
    CtrlEndPt->Value = 0;
    CtrlEndPt->Index = 1;

    FirmwareDescription_t desc;
    LONG len = (LONG)sizeof(desc);
    int success = CtrlEndPt->XferData((UCHAR*)&desc, len);

    if ( success ) {
        fprintf( stderr, "\n---------------------------------"
                         "\nFirmware VERSION: %08X\n", desc.version );
    } else {
        fprintf( stderr, "__error__ FX3Dev::print_version() FAILED\n" );
    }

    return success ? FX3_ERR_OK : FX3_ERR_CTRL_TX_FAIL;
}

fx3_dev_err_t FX3DevCyAPI::scan(int &loadable_count , int &streamable_count) {
    streamable_count = 0;
    loadable_count = 0;
    if (StartParams.USBDevice == NULL) {
        fprintf( stderr, "FX3DevCyAPI::scan() USBDevice == NULL" );
        return FX3_ERR_USB_INIT_FAIL;
    }
    unsigned short product = StartParams.USBDevice->ProductID;
    unsigned short vendor  = StartParams.USBDevice->VendorID;
    if ( log ) fprintf( stderr, "Device: 0x%04X 0x%04X ", vendor, product );
    if ( vendor == VENDOR_ID && product == PRODUCT_STREAM ) {
        if ( log ) fprintf( stderr, " STREAM\n" );
        streamable_count++;
    } else if ( vendor == VENDOR_ID && product == PRODUCT_BOOT ) {
        if ( log ) fprintf( stderr,  "BOOT\n" );
        loadable_count++;
    }
    if ( log ) fprintf( stderr, "\n" );

    if (loadable_count + streamable_count == 0) {
        return FX3_ERR_BAD_DEVICE;
    } else {
        return FX3_ERR_OK;
    }
}

fx3_dev_err_t FX3DevCyAPI::prepareEndPoints() {

    if ( ( StartParams.USBDevice->VendorID != VENDOR_ID) ||
         ( StartParams.USBDevice->ProductID != PRODUCT_STREAM ) )
    {
        return FX3_ERR_BAD_DEVICE;
    }

    int interfaces = StartParams.USBDevice->AltIntfcCount()+1;

    StartParams.bHighSpeedDevice = StartParams.USBDevice->bHighSpeed;
    StartParams.bSuperSpeedDevice = StartParams.USBDevice->bSuperSpeed;

    for (int i=0; i< interfaces; i++)
    {
        StartParams.USBDevice->SetAltIntfc(i);

        int eptCnt = StartParams.USBDevice->EndPointCount();

        // Fill the EndPointsBox
        for (int e=1; e<eptCnt; e++)
        {
            CCyUSBEndPoint *ept = StartParams.USBDevice->EndPoints[e];
            // INTR, BULK and ISO endpoints are supported.
            if ((ept->Attributes >= 1) && (ept->Attributes <= 3))
            {
                EndPointParams Params;
                Params.Attr = ept->Attributes;
                Params.In = ept->bIn;
                Params.MaxPktSize = ept->MaxPktSize;
                Params.MaxBurst = StartParams.USBDevice->BcdUSB == 0x0300 ? ept->ssmaxburst : 0;
                Params.Interface = i;
                Params.Address = ept->Address;

                EndPointsParams.push_back(Params);
            }
        }
    }

    return FX3_ERR_OK;
}

void FX3DevCyAPI::startTransferData(unsigned int EndPointInd, int PPX, int QueueSize, int TimeOut) {
    if(EndPointInd >= EndPointsParams.size())
        return;

    StartParams.CurEndPoint = EndPointsParams[EndPointInd];
    StartParams.PPX = PPX;
    StartParams.QueueSize = QueueSize;
    StartParams.TimeOut = TimeOut;
    StartParams.bStreaming = true;
    StartParams.ThreadAlreadyStopped = false;
    //StartParams.DataProc = DataProc;

    int alt = StartParams.CurEndPoint.Interface;
    int eptAddr = StartParams.CurEndPoint.Address;
    int clrAlt = (StartParams.USBDevice->AltIntfc() == 0) ? 1 : 0;
    if (! StartParams.USBDevice->SetAltIntfc(alt))
    {
        StartParams.USBDevice->SetAltIntfc(clrAlt); // Cleans-up
        return;
    }

    StartParams.EndPt = StartParams.USBDevice->EndPointOf((UCHAR)eptAddr);
}

void FX3DevCyAPI::AbortXferLoop(StartDataTransferParams *Params, int pending, PUCHAR *buffers, CCyIsoPktInfo **isoPktInfos, PUCHAR *contexts, OVERLAPPED *inOvLap)
{
    //EndPt->Abort(); - This is disabled to make sure that while application is doing IO and user unplug the device, this function hang the app.
    if(log) fprintf(stderr, "FX3DevCyAPI::AbortXferLoop \n");

    long len = Params->EndPt->MaxPktSize * Params->PPX;

    for (int j=0; j< Params->QueueSize; j++)
    {
        if (j<pending)
        {
            if (!Params->EndPt->WaitForXfer(&inOvLap[j], Params->TimeOut))
            {
                Params->EndPt->Abort();
                if (Params->EndPt->LastError == ERROR_IO_PENDING)
                    WaitForSingleObject(inOvLap[j].hEvent,1000);
            }

            Params->EndPt->FinishDataXfer(buffers[j], len, &inOvLap[j], contexts[j]);
        }

        CloseHandle(inOvLap[j].hEvent);

        delete [] buffers[j];
        delete [] isoPktInfos[j];
    }

    delete [] buffers;
    delete [] isoPktInfos;
    delete [] contexts;

    Params->bStreaming = false;
    Params->ThreadAlreadyStopped = true;
}

void FX3DevCyAPI::GetEndPointParamsByInd(unsigned int EndPointInd, int *Attr, bool *In, int *MaxPktSize, int *MaxBurst, int *Interface, int *Address) {
    if(EndPointInd >= EndPointsParams.size())
        return;
    *Attr = EndPointsParams[EndPointInd].Attr;
    *In = EndPointsParams[EndPointInd].In;
    *MaxPktSize = EndPointsParams[EndPointInd].MaxPktSize;
    *MaxBurst = EndPointsParams[EndPointInd].MaxBurst;
    *Interface = EndPointsParams[EndPointInd].Interface;
    *Address = EndPointsParams[EndPointInd].Address;
}

fx3_dev_err_t FX3DevCyAPI::loadAdditionalFirmware(const char* fw_name, uint32_t stop_addr) {
    if ( !fw_name ) {
        return FX3_ERR_ADDFIRMWARE_FILE_IO_ERROR;
    }
    
    FILE* f = fopen( fw_name, "r" );
    if ( !f ) {
        fprintf( stderr, "FX3Dev::loadAdditionalFirmware __error__ can't open '%s'\n", fw_name );
        return FX3_ERR_ADDFIRMWARE_FILE_IO_ERROR;
    } else {
        fclose( f );
    }
    
    std::vector<uint32_t> addr;
    std::vector<uint32_t> data;
    
    parse_hex_file( fw_name, addr, data );
    
    for ( uint32_t i = 0; i < addr.size(); i++ ) {
        fx3_dev_err_t eres = send16bitSPI(data[i], addr[i]);
        if ( eres != FX3_ERR_OK ) {
            return eres;
        }
        
        uint32_t ADD_FW_LOAD_PAUSE_MS = 20;
        std::this_thread::sleep_for(std::chrono::milliseconds(ADD_FW_LOAD_PAUSE_MS));
        
        if ( addr[i] == stop_addr ) {
            break;
        }
    }
    return FX3_ERR_OK;
}


// ---------------------- Lattice control -------------------------------------
fx3_dev_err_t FX3DevCyAPI::send16bitSPI_ECP5(uint8_t _addr, uint8_t _data)
{
    UCHAR buf[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    buf[0] = (UCHAR)(_addr);
    buf[1] = (UCHAR)(_data);

    //qDebug() << "Reg" << _addr << " " << hex << _data;

    CCyControlEndPoint* CtrlEndPt;
    CtrlEndPt = StartParams.USBDevice->ControlEndPt;
    CtrlEndPt->Target = TGT_DEVICE;
    CtrlEndPt->ReqType = REQ_VENDOR;
    CtrlEndPt->Direction = DIR_TO_DEVICE;
    CtrlEndPt->ReqCode = 0xD6;
    CtrlEndPt->Value = 0;
    CtrlEndPt->Index = 1;
    long len = 16;
    int success = CtrlEndPt->XferData(buf, len);

    return success ? FX3_ERR_OK : FX3_ERR_CTRL_TX_FAIL;
}

fx3_dev_err_t FX3DevCyAPI::read16bitSPI_ECP5(uint8_t addr, uint8_t* data)
{
    UCHAR buf[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    uint8_t addr_fix = (addr|0x80);

    //fprintf( stderr, "FX3Dev::read16bitSPI_ECP5() from  0x%03X\n", addr );

    CCyControlEndPoint* CtrlEndPt;
    CtrlEndPt = StartParams.USBDevice->ControlEndPt;
    CtrlEndPt->Target = TGT_DEVICE;
    CtrlEndPt->ReqType = REQ_VENDOR;
    CtrlEndPt->Direction = DIR_FROM_DEVICE;
    CtrlEndPt->ReqCode = 0xD9;
    CtrlEndPt->Value = addr_fix;
    CtrlEndPt->Index = *data;
    long len = 16;
    int success = CtrlEndPt->XferData(&buf[0], len);

    *data = buf[0];
#if 0
    if ( success ) {
        fprintf( stderr, "[0x%03X] => 0x%02X\n", addr, *data);
    } else {
        fprintf( stderr, "__error__ FX3Dev::read16bitSPI_ECP5() FAILED\n" );
    }
#endif

    return success ? FX3_ERR_OK : FX3_ERR_CTRL_TX_FAIL;
}

fx3_dev_err_t FX3DevCyAPI::sendECP5(uint8_t* _data, long _data_len)
{
    UCHAR dummybuf[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    LONG  len = 16;
    UCHAR* buf = dummybuf;
    if(_data && _data_len != 0) {
        buf = (UCHAR*)_data;
        len = _data_len;
    }

    CCyControlEndPoint* CtrlEndPt = StartParams.USBDevice->ControlEndPt;
    CtrlEndPt->Target    = TGT_DEVICE;
    CtrlEndPt->ReqType   = REQ_VENDOR;
    CtrlEndPt->Direction = DIR_TO_DEVICE;
    CtrlEndPt->ReqCode   = CMD_ECP5_WRITE;
    CtrlEndPt->Value     = 0;
    CtrlEndPt->Index     = 1;
    int success = CtrlEndPt->XferData(buf, len);

    return success ? FX3_ERR_OK : FX3_ERR_CTRL_TX_FAIL;

}

fx3_dev_err_t FX3DevCyAPI::recvECP5(uint8_t* _data, long _data_len)
{
    UCHAR  dummybuf[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    LONG   len = 16;
    UCHAR* buf = dummybuf;
    if(_data && _data_len != 0) {
        buf = (UCHAR*)_data;
        len = _data_len;
    }

    CCyControlEndPoint* CtrlEndPt = StartParams.USBDevice->ControlEndPt;
    CtrlEndPt->Target    = TGT_DEVICE;
    CtrlEndPt->ReqType   = REQ_VENDOR;
    CtrlEndPt->Direction = DIR_FROM_DEVICE;
    CtrlEndPt->ReqCode   = CMD_ECP5_READ;
    CtrlEndPt->Value     = (WORD)len;
    // CtrlEndPt->Index     = 0;

    int success = CtrlEndPt->XferData(buf, len);

    return success ? FX3_ERR_OK : FX3_ERR_CTRL_TX_FAIL;
}

fx3_dev_err_t FX3DevCyAPI::resetECP5()
{
    UCHAR  dummybuf[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    CCyControlEndPoint* CtrlEndPt;
    CtrlEndPt = StartParams.USBDevice->ControlEndPt;
    CtrlEndPt->Target = TGT_DEVICE;
    CtrlEndPt->ReqType = REQ_VENDOR;
    CtrlEndPt->Direction = DIR_FROM_DEVICE;
    CtrlEndPt->ReqCode = CMD_ECP5_RESET;
    CtrlEndPt->Value = 0;
    CtrlEndPt->Index = 1;
    long len = 16;
    int success = CtrlEndPt->XferData(dummybuf, len);

    return (success & dummybuf[0]) ? FX3_ERR_OK : FX3_ERR_CTRL_TX_FAIL;
}

fx3_dev_err_t FX3DevCyAPI::checkECP5()
{
    UCHAR  dummybuf[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    memset(&dummybuf[0], 0xff, 16);

    CCyControlEndPoint* CtrlEndPt;
    CtrlEndPt = StartParams.USBDevice->ControlEndPt;
    CtrlEndPt->Target = TGT_DEVICE;
    CtrlEndPt->ReqType = REQ_VENDOR;
    CtrlEndPt->Direction = DIR_FROM_DEVICE;
    CtrlEndPt->ReqCode = CMD_ECP5_CHECK;
    CtrlEndPt->Value = 0;
    CtrlEndPt->Index = 1;
    long len = 16;
    int success = CtrlEndPt->XferData(dummybuf, len);

    return (success & dummybuf[0]) ? FX3_ERR_OK : FX3_ERR_CTRL_TX_FAIL;
}

fx3_dev_err_t FX3DevCyAPI::csonECP5()
{
    UCHAR  dummybuf[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    memset(&dummybuf[0], 0xff, 16);

    CCyControlEndPoint* CtrlEndPt;
    CtrlEndPt = StartParams.USBDevice->ControlEndPt;
    CtrlEndPt->Target = TGT_DEVICE;
    CtrlEndPt->ReqType = REQ_VENDOR;
    CtrlEndPt->Direction = DIR_TO_DEVICE;
    CtrlEndPt->ReqCode = CMD_ECP5_CSON;
    CtrlEndPt->Value = 0;
    CtrlEndPt->Index = 1;
    long len = 16;
    int success = CtrlEndPt->XferData(dummybuf, len);

    return (success & dummybuf[0]) ? FX3_ERR_OK : FX3_ERR_CTRL_TX_FAIL;
}

fx3_dev_err_t FX3DevCyAPI::csoffECP5()
{
    UCHAR  dummybuf[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    memset(&dummybuf[0], 0xff, 16);

    CCyControlEndPoint* CtrlEndPt;
    CtrlEndPt = StartParams.USBDevice->ControlEndPt;
    CtrlEndPt->Target = TGT_DEVICE;
    CtrlEndPt->ReqType = REQ_VENDOR;
    CtrlEndPt->Direction = DIR_TO_DEVICE;
    CtrlEndPt->ReqCode = CMD_ECP5_CSOFF;
    CtrlEndPt->Value = 0;
    CtrlEndPt->Index = 1;
    long len = 16;
    int success = CtrlEndPt->XferData(dummybuf, len);

    return (success & dummybuf[0]) ? FX3_ERR_OK : FX3_ERR_CTRL_TX_FAIL;
}

fx3_dev_err_t FX3DevCyAPI::send24bitSPI8bit(unsigned int data)
{
    UCHAR  buf[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    buf[0] = (UCHAR)(data>>16);
    buf[1] = (UCHAR)(data>>8);
    buf[2] = (UCHAR)(data);

    //qDebug() << "Reg" << hex << data;

    CCyControlEndPoint* CtrlEndPt;
    CtrlEndPt = StartParams.USBDevice->ControlEndPt;
    CtrlEndPt->Target = TGT_DEVICE;
    CtrlEndPt->ReqType = REQ_VENDOR;
    CtrlEndPt->Direction = DIR_TO_DEVICE;
    CtrlEndPt->ReqCode = 0xD8;
    CtrlEndPt->Value = 0;
    CtrlEndPt->Index = 1;
    long len = 16;
    int success = CtrlEndPt->XferData(buf, len);

    return success ? FX3_ERR_OK : FX3_ERR_CTRL_TX_FAIL;
}

fx3_dev_err_t FX3DevCyAPI::device_start()
{
    UCHAR buf[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    buf[2] = (UCHAR)(0xFF);

    CCyControlEndPoint* CtrlEndPt;
    CtrlEndPt = StartParams.USBDevice->ControlEndPt;

    if(CtrlEndPt) {
        CtrlEndPt->Target = TGT_DEVICE;
        CtrlEndPt->ReqType = REQ_VENDOR;
        CtrlEndPt->Direction = DIR_TO_DEVICE;
        CtrlEndPt->ReqCode = CMD_DEVICE_START;
        CtrlEndPt->Value = 0;
        CtrlEndPt->Index = 1;
        long len = 16;
        int success = CtrlEndPt->XferData(buf, len);

        return success ? FX3_ERR_OK : FX3_ERR_CTRL_TX_FAIL;
    }

    return FX3_ERR_BAD_DEVICE;
}

fx3_dev_err_t FX3DevCyAPI::device_stop()
{
    UCHAR  buf[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    buf[2] = (UCHAR)(0xFF);

    CCyControlEndPoint* CtrlEndPt;
    CtrlEndPt = StartParams.USBDevice->ControlEndPt;

    if(CtrlEndPt)
    {
        CtrlEndPt->Target = TGT_DEVICE;
        CtrlEndPt->ReqType = REQ_VENDOR;
        CtrlEndPt->Direction = DIR_TO_DEVICE;
        CtrlEndPt->ReqCode = CMD_DEVICE_STOP;
        CtrlEndPt->Value = 0;
        CtrlEndPt->Index = 1;
        long len = 16;
        int success = CtrlEndPt->XferData(buf, len);

        return success ? FX3_ERR_OK : FX3_ERR_CTRL_TX_FAIL;
    }

    return FX3_ERR_BAD_DEVICE;
}

fx3_dev_err_t FX3DevCyAPI::device_reset()
{
    UCHAR buf[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    buf[2] = (UCHAR)(0xFF);

    CCyControlEndPoint* CtrlEndPt;
    CtrlEndPt = StartParams.USBDevice->ControlEndPt;
    CtrlEndPt->Target = TGT_DEVICE;
    CtrlEndPt->ReqType = REQ_VENDOR;
    CtrlEndPt->Direction = DIR_TO_DEVICE;
    CtrlEndPt->ReqCode = CMD_CYPRESS_RESET;
    CtrlEndPt->Value = 0;
    CtrlEndPt->Index = 1;
    long len = 16;
    int success = CtrlEndPt->XferData(buf, len);

    return success ? FX3_ERR_OK : FX3_ERR_CTRL_TX_FAIL;
}

fx3_dev_err_t FX3DevCyAPI::reset_nt1065()
{
    UCHAR  buf[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    buf[2] = (UCHAR)(0xFF);

    CCyControlEndPoint* CtrlEndPt;
    CtrlEndPt = StartParams.USBDevice->ControlEndPt;
    CtrlEndPt->Target = TGT_DEVICE;
    CtrlEndPt->ReqType = REQ_VENDOR;
    CtrlEndPt->Direction = DIR_TO_DEVICE;
    CtrlEndPt->ReqCode = CMD_NT1065_RESET;
    CtrlEndPt->Value = 0;
    CtrlEndPt->Index = 1;
    long len = 16;
    int success = CtrlEndPt->XferData(buf, len);

    return success ? FX3_ERR_OK : FX3_ERR_CTRL_TX_FAIL;
}

fx3_dev_err_t FX3DevCyAPI::load1065Ctrlfile(const char* fwFileName, int lastaddr)
{
    fx3_dev_err_t retCode = FX3_ERR_OK;
    CHAR line[128];

    FILE* pFile = fopen( fwFileName, "r" );
    if(!pFile) {
        return FX3_ERR_FIRMWARE_FILE_IO_ERROR;
    }

    while(fgets(line, 128, pFile) != NULL)
    {
        std::string sline(line);
        if(sline[0] != ';')
        {
            int regpos = sline.find("Reg");
            if(regpos != std::string::npos)
            {
                std::string buf;
                std::stringstream ss(sline); // Insert the string into a stream
                std::vector<std::string> tokens;
                while(ss >> buf)
                    tokens.push_back(buf);
                if(tokens.size() == 2) // addr & value
                {
                    int addr = std::stoi(tokens[0].erase(0,3), nullptr, 10);
                    int value = std::stoi(tokens.at(1), nullptr, 16);
                    std::this_thread::sleep_for(std::chrono::milliseconds(20));
                    retCode = send16bitSPI_ECP5(addr, value);
                    if(retCode != FX3_ERR_OK || addr == lastaddr)
                        break;
                }
            }
        }
    }

    fclose(pFile);

    return retCode;
}

// ---------------------------------------------------------------------------


fx3_dev_err_t FX3DevCyAPI::send16bitSPI(unsigned char data, unsigned char addr) {
    std::lock_guard<std::mutex> critical_section(this->mtx);

    UCHAR buf[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    buf[0] = (UCHAR)(data);
    buf[1] = (UCHAR)(addr);
    
    //fprintf( stderr, "FX3Dev::send16bitToDevice( 0x%02X, 0x%02X )\n", data, addr );
    
    CCyControlEndPoint* CtrlEndPt;
    CtrlEndPt = StartParams.USBDevice->ControlEndPt;
    CtrlEndPt->Target = TGT_DEVICE;
    CtrlEndPt->ReqType = REQ_VENDOR;
    CtrlEndPt->Direction = DIR_TO_DEVICE;
    CtrlEndPt->ReqCode = 0xB3;
    CtrlEndPt->Value = 0;
    CtrlEndPt->Index = 1;
    long len = 16;
    int success = CtrlEndPt->XferData(&buf[0], len);
    
    return success ? FX3_ERR_OK : FX3_ERR_CTRL_TX_FAIL;
}


fx3_dev_err_t FX3DevCyAPI::read16bitSPI(unsigned char addr, unsigned char* data)
{
    std::lock_guard<std::mutex> critical_section(this->mtx);

    UCHAR buf[16];
    //addr |= 0x8000;
    buf[0] = (UCHAR)(*data);
    buf[1] = (UCHAR)(addr|0x80);

    //if ( log ) fprintf( stderr, "FX3Dev::read16bitSPI() from  0x%02X\n", addr );

    CCyControlEndPoint* CtrlEndPt;
    CtrlEndPt = StartParams.USBDevice->ControlEndPt;
    CtrlEndPt->Target = TGT_DEVICE;
    CtrlEndPt->ReqType = REQ_VENDOR;
    CtrlEndPt->Direction = DIR_FROM_DEVICE;
    CtrlEndPt->ReqCode = 0xB5;
    CtrlEndPt->Value = *data;
    CtrlEndPt->Index = addr|0x80;
    long len = 16;
    int success = CtrlEndPt->XferData(&buf[0], len);

    *data = buf[0];
    if ( success ) {
        //fprintf( stderr, "[0x%02X] is 0x%02X\n", addr, *data );
    } else {
        fprintf( stderr, "__error__ FX3Dev::read16bitSPI() FAILED\n" );
    }

    return success ? FX3_ERR_OK : FX3_ERR_CTRL_TX_FAIL;
}




fx3_dev_err_t FX3DevCyAPI::send24bitSPI(unsigned char data, unsigned short addr)
{
    std::lock_guard<std::mutex> critical_section(this->mtx);

    if ( log ) fprintf( stderr, "[0x%03X] <= 0x%02X\n", addr, data );

    UCHAR buf[16];
    addr |= 0x8000;
    buf[0] = (UCHAR)(data);
    buf[1] = (UCHAR)(addr&0x0FF);
    buf[2] = (UCHAR)(addr>>8);

    CCyControlEndPoint* CtrlEndPt;
    CtrlEndPt = StartParams.USBDevice->ControlEndPt;
    CtrlEndPt->Target = TGT_DEVICE;
    CtrlEndPt->ReqType = REQ_VENDOR;
    CtrlEndPt->Direction = DIR_TO_DEVICE;
    CtrlEndPt->ReqCode = 0xB6;
    CtrlEndPt->Value = 0;
    CtrlEndPt->Index = 1;
    long len = 16;
    int success = CtrlEndPt->XferData(&buf[0], len);

    if ( !success ) {
        fprintf( stderr, "__error__ FX3Dev::send24bitSPI() FAILED (len = %d)\n", len );
    }

    return success ? FX3_ERR_OK : FX3_ERR_CTRL_TX_FAIL;;
}

fx3_dev_err_t FX3DevCyAPI::read24bitSPI(unsigned short addr, unsigned char* data)
{
    std::lock_guard<std::mutex> critical_section(this->mtx);

    UCHAR buf[16];
    //addr |= 0x8000;
    buf[0] = (UCHAR)(*data);
    buf[1] = (UCHAR)(addr&0x0FF);
    buf[2] = (UCHAR)(addr>>8);

    if ( log ) fprintf( stderr, "FX3Dev::read24bitSPI() from  0x%03X\n", addr );

    CCyControlEndPoint* CtrlEndPt;
    CtrlEndPt = StartParams.USBDevice->ControlEndPt;
    CtrlEndPt->Target = TGT_DEVICE;
    CtrlEndPt->ReqType = REQ_VENDOR;
    CtrlEndPt->Direction = DIR_FROM_DEVICE;
    CtrlEndPt->ReqCode = 0xB5;
    CtrlEndPt->Value = *data;
    CtrlEndPt->Index = addr;
    long len = 16;
    int success = CtrlEndPt->XferData(&buf[0], len);

    *data = buf[0];
    if ( success ) {
        if ( log ) fprintf( stderr, "[0x%03X] => 0x%02X\n", addr, *data );
    } else {
        fprintf( stderr, "__error__ FX3Dev::read24bitSPI() FAILED\n" );
    }
    return success ? FX3_ERR_OK : FX3_ERR_CTRL_TX_FAIL;
}


void FX3DevCyAPI::xfer_loop() {
    if ( !log ) fprintf( stderr, "--- FX3DevCyAPI::xfer_loop() started ---\n");
    StartDataTransferParams* Params = &StartParams;
        
    if(Params->EndPt->MaxPktSize==0) {
        fprintf( stderr, "xfer_loop(): Params->EndPt->MaxPktSize==0 \n");
        return;
    }

    // Limit total transfer length to 4MByte
    long len = ((Params->EndPt->MaxPktSize) * Params->PPX);
    
    int maxLen = 0x400000;  //4MByte
    if (len > maxLen){
        Params->PPX = maxLen / (Params->EndPt->MaxPktSize);
        if((Params->PPX%8)!=0)
            Params->PPX -= (Params->PPX%8);
    }
    
    if ((Params->bSuperSpeedDevice || Params->bHighSpeedDevice) && (Params->EndPt->Attributes == 1)){  // HS/SS ISOC Xfers must use PPX >= 8
        Params->PPX = max(Params->PPX, 8);
        Params->PPX = (Params->PPX / 8) * 8;
        if(Params->bHighSpeedDevice)
            Params->PPX = max(Params->PPX, 128);
    }

    long BytesXferred = 0;
    unsigned long Successes = 0;
    unsigned long Failures = 0;
    int i = 0;

    // Allocate the arrays needed for queueing
    PUCHAR			*buffers		= new PUCHAR[Params->QueueSize];
    CCyIsoPktInfo	**isoPktInfos	= new CCyIsoPktInfo*[Params->QueueSize];
    PUCHAR			*contexts		= new PUCHAR[Params->QueueSize];
    OVERLAPPED		inOvLap[MAX_QUEUE_SZ];

    Params->EndPt->SetXferSize(len);

    // Allocate all the buffers for the queues
    for (i=0; i< Params->QueueSize; i++)
    {
        buffers[i]        = new UCHAR[len];
        isoPktInfos[i]    = new CCyIsoPktInfo[Params->PPX];
        inOvLap[i].hEvent = CreateEvent(NULL, false, false, NULL);

        memset(buffers[i],0x0A,len);
    }

    // Queue-up the first batch of transfer requests
    for (i=0; i< Params->QueueSize; i++)
    {
        contexts[i] = Params->EndPt->BeginDataXfer(buffers[i], len, &inOvLap[i]);
        if (Params->EndPt->NtStatus || Params->EndPt->UsbdStatus) // BeginDataXfer failed
        {
            fprintf( stderr, "__error__ FX3DevCyAPI::xfer_loop() BeginDataXfer (inital) failed, i=%d\n", i );
            AbortXferLoop(Params, i+1, buffers,isoPktInfos,contexts,inOvLap);
            return;
        }
    }

    i=0;



    // The infinite xfer loop.
    for (;Params->bStreaming;)
    {        
        long rLen = len;	// Reset this each time through because
        // FinishDataXfer may modify it

        if (!Params->EndPt->WaitForXfer(&inOvLap[i], Params->TimeOut))
        {
            Params->EndPt->Abort();
            if (Params->EndPt->LastError == ERROR_IO_PENDING)
                WaitForSingleObject(inOvLap[i].hEvent,2000);
        }

        if (Params->EndPt->Attributes == 1) // ISOC Endpoint
        {
            if (Params->EndPt->FinishDataXfer(buffers[i], rLen, &inOvLap[i], contexts[i], isoPktInfos[i]))
            {
                CCyIsoPktInfo *pkts = isoPktInfos[i];
                for (int j=0; j< Params->PPX; j++)
                {
                    if ((pkts[j].Status == 0) && (pkts[j].Length<=Params->EndPt->MaxPktSize))
                    {
                        BytesXferred += pkts[j].Length;
                        Successes++;
                    }
                    else
                        Failures++;
                    pkts[j].Length = 0;	// Reset to zero for re-use.
                    pkts[j].Status = 0;
                }
            }
            else
                Failures++;
        }
        else // BULK Endpoint
        {
            if (Params->EndPt->FinishDataXfer(buffers[i], rLen, &inOvLap[i], contexts[i]))
            {
                Successes++;
                BytesXferred += len;
            }
            else
                Failures++;
        }

        BytesXferred = max(BytesXferred, 0);

        size_tx_mb += ( ( double ) len ) / ( 1024.0 * 1024.0 );
        {
            std::lock_guard<std::mutex> guard(handler_mtx);
            if ( data_handler ) {
                data_handler->HandleDeviceData(buffers[i], len);
            }
        }

        // Re-submit this queue element to keep the queue full
        contexts[i] = Params->EndPt->BeginDataXfer(buffers[i], len, &inOvLap[i]);
        if (Params->EndPt->NtStatus || Params->EndPt->UsbdStatus) // BeginDataXfer failed
        {
            fprintf( stderr, "__error__ FX3DevCyAPI::xfer_loop() BeginDataXfer (continue) failed, i=%d\n", i );
            AbortXferLoop(Params, Params->QueueSize,buffers,isoPktInfos,contexts,inOvLap);
            return;
        }

        i = (i + 1) % Params->QueueSize;
    }  // End of the infinite loop

     if ( log ) fprintf(stderr, "--- FX3DevCyAPI::xfer_loop() Exit from xfer_loop ---\n");

    AbortXferLoop( Params, Params->QueueSize, buffers, isoPktInfos, contexts, inOvLap );

    if ( log ) fprintf( stderr, "--- FX3DevCyAPI::xfer_loop() finished ---\n" );
}

uint8_t FX3DevCyAPI::peek8(uint32_t register_address24) {
    uint8_t val = 0;
    if ( read24bitSPI( register_address24, &val ) == FX3_ERR_OK ) {
        return val;
    } else {
        return 0xFF;
    }
}

void FX3DevCyAPI::poke8(uint32_t register_address24, uint8_t value) {
    send24bitSPI( value, register_address24 );
}

#endif

