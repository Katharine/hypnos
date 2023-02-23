#ifndef apps_base_app_h
#define apps_base_app_h

namespace apps {

class base_app {
public:
    virtual void tick() = 0;
    virtual void init() = 0;
};

}

#endif
