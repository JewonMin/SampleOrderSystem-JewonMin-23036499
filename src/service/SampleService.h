#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include "../repository/JsonRepository.h"

class SampleService {
public:
    explicit SampleService(JsonRepository& repo);

    // 등록: 유효성 검사 후 자동 ID 부여, 저장소에 추가
    Sample add(const std::string& name, double avgProductionTime, double yield, int stock);

    const std::vector<Sample>& all() const;
    const Sample* findById(const std::string& id) const;
    std::vector<const Sample*> searchByName(const std::string& keyword) const;

private:
    JsonRepository& m_repo;
    std::string generateId() const;
};
