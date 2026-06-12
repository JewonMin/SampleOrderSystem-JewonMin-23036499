#include "JsonRepository.h"
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;

JsonRepository::JsonRepository(const std::string& dbDir) : m_dbDir(dbDir) {}

void JsonRepository::load() {
    fs::create_directories(m_dbDir);

    auto tryLoad = [&](const std::string& file, auto& vec) {
        std::string path = m_dbDir + "/" + file;
        if (fs::exists(path)) {
            std::ifstream f(path);
            using T = typename std::remove_reference_t<decltype(vec)>::value_type;
            vec = json::parse(f).get<std::vector<T>>();
        }
    };

    tryLoad("samples.json",          m_samples);
    tryLoad("orders.json",           m_orders);
    tryLoad("production_queue.json", m_productionQueue);
}

void JsonRepository::save() const {
    fs::create_directories(m_dbDir);

    std::ofstream(m_dbDir + "/samples.json")          << json(m_samples).dump(2);
    std::ofstream(m_dbDir + "/orders.json")            << json(m_orders).dump(2);
    std::ofstream(m_dbDir + "/production_queue.json")  << json(m_productionQueue).dump(2);
}

std::vector<Sample>&          JsonRepository::samples()         { return m_samples; }
std::vector<Order>&           JsonRepository::orders()          { return m_orders; }
std::vector<ProductionEntry>& JsonRepository::productionQueue() { return m_productionQueue; }

const std::vector<Sample>&          JsonRepository::samples()         const { return m_samples; }
const std::vector<Order>&           JsonRepository::orders()          const { return m_orders; }
const std::vector<ProductionEntry>& JsonRepository::productionQueue() const { return m_productionQueue; }
