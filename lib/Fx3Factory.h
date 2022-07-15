#ifndef __FX3_FACTORY_H__
#define __FX3_FACTORY_H__

class IFx3Device;

enum Fx3DriverType{
    LIBUSB_DEVICE_TYPE,
    CYAPI_DEVICE_TYPE
};


class Fx3DeviceFactory
{
public:
    static IFx3Device* CreateInstance(Fx3DriverType type, unsigned int one_block_size8 = 2 * 1024 * 1024, unsigned int dev_buffers_count = 4);
};

#endif // __IFX3_FACTORY_H__

