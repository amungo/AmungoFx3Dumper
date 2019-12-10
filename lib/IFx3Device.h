#ifndef __IFX3_DEVICE_H__
#define __IFX3_DEVICE_H__

#include "fx3devdebuginfo.h"


// Interface for handling data from FX3 device
class DeviceDataHandlerIfce {
public:
    virtual void HandleDeviceData( void* data_pointer, unsigned int size_in_bytes ) = 0;
};

class IFx3Device
{
public:
    virtual int addRef() = 0;
    virtual void Release() = 0;

    // Opens device and flash it if neccessary (set firmwareFileName to NULL to disable flashing)
    virtual fx3_dev_err_t init( const char* firmwareFileName, const char* additionalFirmwareFileName ) = 0;
    virtual fx3_dev_err_t init_fpga(const char* algoFileName, const char* dataFileName) = 0;
    // Starts reading of signal from device and sends data to handler.
    // If handler is NULL, data will be read and skipped
    virtual void startRead(DeviceDataHandlerIfce* handler ) = 0;
    // Stop's reading from device.
    virtual void stopRead() = 0;

    virtual void changeHandler(DeviceDataHandlerIfce* handler) = 0;
    virtual fx3_dev_err_t getReceiverRegValue(unsigned char addr, unsigned char& value) = 0;
    virtual fx3_dev_err_t putReceiverRegValue(unsigned char addr, unsigned char value) = 0;

    virtual void sendAttCommand5bits( unsigned int bits ) = 0;
    virtual fx3_dev_debug_info_t getDebugInfoFromBoard( bool ask_speed_only = false ) = 0;
    virtual void readFwVersion() = 0;

    //------------------------ Lattice control -----------------------------
    virtual fx3_dev_err_t send8bitSPI(unsigned char addr, unsigned char data) = 0;
    virtual fx3_dev_err_t read8bitSPI(unsigned char addr, unsigned char* data) = 0;
    virtual fx3_dev_err_t sendECP5(uint8_t* buf, long len) = 0;
    virtual fx3_dev_err_t recvECP5(uint8_t* buf, long len) = 0;
    virtual fx3_dev_err_t resetECP5() = 0;
    virtual fx3_dev_err_t switchoffECP5() = 0;
    virtual fx3_dev_err_t checkECP5() = 0;
    virtual fx3_dev_err_t csonECP5() = 0;
    virtual fx3_dev_err_t csoffECP5() = 0;
    virtual fx3_dev_err_t setDAC(unsigned int data) = 0;
    virtual fx3_dev_err_t device_start() = 0;
    virtual fx3_dev_err_t device_stop() = 0;
    virtual fx3_dev_err_t device_reset() = 0;
    virtual fx3_dev_err_t reset_nt1065() = 0;
    virtual fx3_dev_err_t load1065Ctrlfile(const char* fwFileName, int lastaddr) = 0;

protected:
    virtual ~IFx3Device() {}
};

#endif // __IFX3_DEVICE_H__

