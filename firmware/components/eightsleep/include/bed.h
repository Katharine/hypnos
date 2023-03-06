//
// Created by Katharine Berry on 3/5/23.
//

#ifndef EIGHTSLEEP_BED_H
#define EIGHTSLEEP_BED_H

namespace eightsleep {

struct Bed {
    int currentTemp;
    int targetTemp;
    bool active;
};

}

#endif //EIGHTSLEEP_BED_H
