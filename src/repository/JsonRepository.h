#pragma once
#include <vector>
#include <string>
#include "../model/Sample.h"
#include "../model/Order.h"

class JsonRepository {
public:
    explicit JsonRepository(const std::string& dbDir = "db");

    void load();
    void save() const;

    std::vector<Sample>&       samples();
    std::vector<Order>&        orders();
    const std::vector<Sample>& samples() const;
    const std::vector<Order>&  orders() const;

private:
    std::string          m_dbDir;
    std::vector<Sample>  m_samples;
    std::vector<Order>   m_orders;
};
