#pragma once
#include "../service/ReleaseService.h"
#include "../service/SampleService.h"

class ReleaseView {
public:
    ReleaseView(ReleaseService& releaseService, SampleService& sampleService);
    void run();

private:
    void showList();

    ReleaseService& m_releaseService;
    SampleService&  m_sampleService;
};
