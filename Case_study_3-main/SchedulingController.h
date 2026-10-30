#ifndef SCHEDULING_CONTROLLER_H
#define SCHEDULING_CONTROLLER_H

#include "Interface.h"
#include "DataProcessor.h"

class SchedulingController {
private:
    Interface ui;
    DataProcessor xuLyDuLieu;
    bool isDataLoaded;

public:
    SchedulingController();
    void run();
};

#endif