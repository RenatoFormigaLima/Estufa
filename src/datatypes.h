#pragma once

namespace devices_types{

    enum device_type
    {
        sensor,
        lcd,
        rele
    };

    enum RELESTATE
    {
        ON = 0x00,
        OFF
    };

    class device{
        private:
            struct dev_data
            {
                int type;
                int id;
                int value;
            } typedef dev;

        public:
            device(int type=0, int id=0, int value=0);
            device(int type=0, int value=0);
            device(int type=0);
            ~device() = default;
            
    };

}