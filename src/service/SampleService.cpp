#include "SampleService.h"
#include <cstdio>

SampleService::SampleService(JsonRepository& repo) : m_repo(repo) {}

Sample SampleService::add(const std::string& name, double avgProductionTime,
                           double yield, int stock) {
    if (name.empty())
        throw std::invalid_argument("시료명이 비어 있습니다.");
    if (avgProductionTime <= 0)
        throw std::invalid_argument("평균 생산시간은 0 초과 숫자여야 합니다.");
    if (yield <= 0 || yield > 1)
        throw std::invalid_argument("수율은 0 초과 1 이하 숫자여야 합니다.");
    if (stock < 0)
        throw std::invalid_argument("재고는 0 이상 정수여야 합니다.");

    Sample s{generateId(), name, avgProductionTime, yield, stock};
    m_repo.samples().push_back(s);
    return s;
}

const std::vector<Sample>& SampleService::all() const {
    return m_repo.samples();
}

const Sample* SampleService::findById(const std::string& id) const {
    for (const auto& s : m_repo.samples())
        if (s.id == id) return &s;
    return nullptr;
}

std::vector<const Sample*> SampleService::searchByName(const std::string& keyword) const {
    std::vector<const Sample*> result;
    for (const auto& s : m_repo.samples())
        if (s.name.find(keyword) != std::string::npos)
            result.push_back(&s);
    return result;
}

std::string SampleService::generateId() const {
    int maxNum = 0;
    for (const auto& s : m_repo.samples()) {
        if (s.id.size() >= 3 && s.id[0] == 'S' && s.id[1] == '-') {
            try {
                int n = std::stoi(s.id.substr(2));
                if (n > maxNum) maxNum = n;
            } catch (...) {}
        }
    }
    char buf[16];
    std::snprintf(buf, sizeof(buf), "S-%03d", maxNum + 1);
    return buf;
}
