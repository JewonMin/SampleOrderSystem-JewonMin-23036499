#include "JsonRepository.h"
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;

JsonRepository::JsonRepository(const std::string& dbDir) : m_dbDir(dbDir) {}

void JsonRepository::load() {
    fs::create_directories(m_dbDir);

    std::string samplesPath = m_dbDir + "/samples.json";
    if (fs::exists(samplesPath)) {
        std::ifstream f(samplesPath);
        m_samples = json::parse(f).get<std::vector<Sample>>();
    }

    std::string ordersPath = m_dbDir + "/orders.json";
    if (fs::exists(ordersPath)) {
        std::ifstream f(ordersPath);
        m_orders = json::parse(f).get<std::vector<Order>>();
    }
}

void JsonRepository::save() const {
    fs::create_directories(m_dbDir);

    std::ofstream(m_dbDir + "/samples.json") << json(m_samples).dump(2);
    std::ofstream(m_dbDir + "/orders.json")  << json(m_orders).dump(2);
}

std::vector<Sample>&       JsonRepository::samples()       { return m_samples; }
std::vector<Order>&        JsonRepository::orders()        { return m_orders; }
const std::vector<Sample>& JsonRepository::samples() const { return m_samples; }
const std::vector<Order>&  JsonRepository::orders()  const { return m_orders; }
