#pragma once
#include "../service/SampleService.h"

class SampleView {
public:
    explicit SampleView(SampleService& service);
    void run();

private:
    void showRegister();
    void showList();
    void showSearch();

    SampleService& m_service;
};
