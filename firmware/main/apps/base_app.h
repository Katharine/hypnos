//
// Created by Katharine Berry on 3/3/23.
//

#ifndef FIRMWARE_BASE_APP_H
#define FIRMWARE_BASE_APP_H

namespace apps {

class BaseApp {
public:
    virtual void present() = 0;
    virtual ~BaseApp() = default;
};

}

#endif //FIRMWARE_BASE_APP_H
