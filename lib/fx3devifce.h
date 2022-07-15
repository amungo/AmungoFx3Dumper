#ifndef FX3DEVIFCE
#define FX3DEVIFCE

#include "IFx3Device.h"
#include "fx3deverr.h"
#include "fx3devdebuginfo.h"
#include "fx3commands.h"
#include <stddef.h>
#include <mutex>



class FX3DevIfce : public IFx3Device {
public:

    FX3DevIfce();

    int addRef();
    void Release();

    // Opens device and flash it if neccessary (set firmwareFileName to NULL to disable flashing)
    virtual fx3_dev_err_t init( const char* firmwareFileName,
                                const char* additionalFirmwareFileName ) = 0;
    virtual fx3_dev_err_t init_fpga(const char* bitFileName);

    // Starts reading of signal from device and sends data to handler.
    // If handler is NULL, data will be read and skipped
    virtual void startRead( DeviceDataHandlerIfce* handler ) = 0;

    // Stop's reading from device.
    virtual void stopRead() = 0;
    virtual void changeHandler(DeviceDataHandlerIfce* handler);
    virtual fx3_dev_err_t getReceiverRegValue(unsigned char addr, unsigned char& value);
    virtual fx3_dev_err_t putReceiverRegValue(unsigned char addr, unsigned char value);

    virtual void sendAttCommand5bits( uint32_t bits ) = 0;

    virtual fx3_dev_debug_info_t getDebugInfoFromBoard( bool ask_speed_only = false ) = 0;
    virtual void readFwVersion();

    //----------------------- Lattice control ------------------
    virtual fx3_dev_err_t send8bitSPI(uint8_t addr, uint8_t data);
    virtual fx3_dev_err_t read8bitSPI(uint8_t addr, uint8_t* data);
    virtual fx3_dev_err_t sendECP5(uint8_t* buf, long len);
    virtual fx3_dev_err_t recvECP5(uint8_t* buf, long len);
    virtual fx3_dev_err_t resetECP5();
    virtual fx3_dev_err_t switchoffECP5();
    virtual fx3_dev_err_t checkECP5();
    virtual fx3_dev_err_t csonECP5();
    virtual fx3_dev_err_t csoffECP5();
    virtual fx3_dev_err_t setDAC(unsigned int data);
    virtual fx3_dev_err_t device_start();
    virtual fx3_dev_err_t device_stop();
    virtual fx3_dev_err_t device_reset();
    virtual fx3_dev_err_t reset_nt1065();
    virtual fx3_dev_err_t load1065Ctrlfile(const char* fwFileName, int lastaddr);

protected:
    virtual fx3_dev_err_t ctrlToDevice(   uint8_t cmd, uint16_t value = 0, uint16_t index = 0, void* data = nullptr, size_t data_len = 0 ) = 0;
    virtual fx3_dev_err_t ctrlFromDevice( uint8_t cmd, uint16_t value = 0, uint16_t index = 0, void* dest = nullptr, size_t data_len = 0 ) = 0;

    virtual void writeGPIO( uint32_t gpio, uint32_t value );
    virtual void readGPIO( uint32_t gpio, uint32_t* value );

    virtual fx3_dev_err_t resetFx3Chip();
    virtual void pre_init_fx3();
    virtual void init_ntlab_default();


    virtual uint32_t GetNt1065ChipID();
    virtual void readNtReg(uint32_t reg);

    virtual void startGpif();

    virtual ~FX3DevIfce() {}

    FirmwareDescription_t fwDescription;

protected:
    DeviceDataHandlerIfce* data_handler;

private:
    std::mutex m_mutex;
    int m_ref_count;

};

#endif // FX3DEVIFCE

