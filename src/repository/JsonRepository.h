#pragma once
#include <vector>
#include <string>
#include "../model/Sample.h"
#include "../model/Order.h"
#include "../model/ProductionEntry.h"

class JsonRepository {
public:
    explicit JsonRepository(const std::string& dbDir = "db");

    void load();
    void save() const;

    std::vector<Sample>&          samples();
    std::vector<Order>&           orders();
    std::vector<ProductionEntry>& productionQueue();

    const std::vector<Sample>&          samples()         const;
    const std::vector<Order>&           orders()          const;
    const std::vector<ProductionEntry>& productionQueue() const;

private:
    std::string                  m_dbDir;
    std::vector<Sample>          m_samples;
    std::vector<Order>           m_orders;
    std::vector<ProductionEntry> m_productionQueue;
};
