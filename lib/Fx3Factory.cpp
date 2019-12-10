#include "Fx3Factory.h"
#include "FX3Dev.h"
#include "fx3devcyapi.h"


IFx3Device* Fx3DeviceFactory::CreateInstance(Fx3DriverType type, unsigned int one_block_size8, unsigned int dev_buffers_count)
{
    if(type == LIBUSB_DEVICE_TYPE)
        return new FX3Dev(one_block_size8, dev_buffers_count);
    else if(type == CYAPI_DEVICE_TYPE)
        return new FX3DevCyAPI();
    else
        return NULL;
}
