#include <gtest/gtest.h>
#include <filesystem>
#include <memory>
#include "repository/JsonRepository.h"
#include "service/SampleService.h"

namespace fs = std::filesystem;

class Phase2Test : public ::testing::Test {
protected:
    const std::string testDbDir = "test_db_phase2";
    std::unique_ptr<JsonRepository> repo;
    std::unique_ptr<SampleService>  service;

    void SetUp() override {
        fs::remove_all(testDbDir);
        repo    = std::make_unique<JsonRepository>(testDbDir);
        service = std::make_unique<SampleService>(*repo);
    }

    void TearDown() override {
        fs::remove_all(testDbDir);
    }
};

TEST_F(Phase2Test, 시료_등록_후_목록에_표시됨) {
    service->add("테스트 시료", 0.5, 0.92, 100);
    ASSERT_EQ(service->all().size(), 1u);
    EXPECT_EQ(service->all()[0].name, "테스트 시료");
    EXPECT_EQ(service->all()[0].stock, 100);
}

TEST_F(Phase2Test, 첫번째_시료_ID는_S001) {
    Sample s = service->add("A 시료", 0.3, 0.8, 0);
    EXPECT_EQ(s.id, "S-001");
}

TEST_F(Phase2Test, ID_순차_자동_증가) {
    service->add("A", 0.3, 0.8, 0);
    service->add("B", 0.4, 0.9, 0);
    Sample s3 = service->add("C", 0.5, 0.7, 0);
    EXPECT_EQ(s3.id, "S-003");
    EXPECT_EQ(service->all().size(), 3u);
}

TEST_F(Phase2Test, 이름_부분검색_일치_반환) {
    service->add("실리콘 웨이퍼-8인치", 0.5, 0.92, 100);
    service->add("GaN 에피택셀", 0.3, 0.78, 50);

    auto results = service->searchByName("웨이퍼");
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0]->name, "실리콘 웨이퍼-8인치");
}

TEST_F(Phase2Test, 이름_검색_여러_일치_반환) {
    service->add("웨이퍼-A", 0.5, 0.9, 0);
    service->add("웨이퍼-B", 0.4, 0.8, 0);
    service->add("기타 시료", 0.3, 0.7, 0);

    auto results = service->searchByName("웨이퍼");
    EXPECT_EQ(results.size(), 2u);
}

TEST_F(Phase2Test, 이름_검색_없는_결과_빈_벡터) {
    service->add("A 시료", 0.5, 0.9, 0);
    auto results = service->searchByName("존재하지않는");
    EXPECT_TRUE(results.empty());
}

TEST_F(Phase2Test, ID로_시료_조회_성공) {
    service->add("테스트", 0.5, 0.9, 10);
    const Sample* s = service->findById("S-001");
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->name, "테스트");
}

TEST_F(Phase2Test, 없는_ID_조회_nullptr_반환) {
    const Sample* s = service->findById("S-999");
    EXPECT_EQ(s, nullptr);
}

TEST_F(Phase2Test, 유효하지_않은_수율_예외) {
    EXPECT_THROW(service->add("X", 0.5, 1.1, 0),  std::invalid_argument);
    EXPECT_THROW(service->add("X", 0.5, 0.0, 0),  std::invalid_argument);
    EXPECT_THROW(service->add("X", 0.5, -0.1, 0), std::invalid_argument);
}

TEST_F(Phase2Test, 유효하지_않은_생산시간_예외) {
    EXPECT_THROW(service->add("X", 0.0,  0.9, 0), std::invalid_argument);
    EXPECT_THROW(service->add("X", -1.0, 0.9, 0), std::invalid_argument);
}

TEST_F(Phase2Test, 빈_이름_예외) {
    EXPECT_THROW(service->add("", 0.5, 0.9, 0), std::invalid_argument);
}

TEST_F(Phase2Test, 등록_데이터_저장_후_재로드_일치) {
    service->add("웨이퍼-A", 0.5, 0.92, 200);
    repo->save();

    JsonRepository repo2(testDbDir);
    repo2.load();
    SampleService service2(repo2);

    ASSERT_EQ(service2.all().size(), 1u);
    EXPECT_EQ(service2.all()[0].id,    "S-001");
    EXPECT_EQ(service2.all()[0].name,  "웨이퍼-A");
    EXPECT_DOUBLE_EQ(service2.all()[0].yield, 0.92);
    EXPECT_EQ(service2.all()[0].stock, 200);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
