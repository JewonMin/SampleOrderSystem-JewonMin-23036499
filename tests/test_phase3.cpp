#include <gtest/gtest.h>
#include <filesystem>
#include <memory>
#include <regex>
#include "repository/JsonRepository.h"
#include "service/OrderService.h"

namespace fs = std::filesystem;

class Phase3Test : public ::testing::Test {
protected:
    const std::string testDbDir = "test_db_phase3";
    std::unique_ptr<JsonRepository> repo;
    std::unique_ptr<OrderService>   service;

    void SetUp() override {
        fs::remove_all(testDbDir);
        repo = std::make_unique<JsonRepository>(testDbDir);
        repo->samples().push_back({"S-001", "실리콘 웨이퍼", 0.5, 0.92, 480});
        repo->samples().push_back({"S-002", "GaN 에피택셀",  0.3, 0.78, 220});
        service = std::make_unique<OrderService>(*repo);
    }

    void TearDown() override {
        fs::remove_all(testDbDir);
    }

    static int extractNNNN(const std::string& orderId) {
        auto pos = orderId.rfind('-');
        return std::stoi(orderId.substr(pos + 1));
    }
};

TEST_F(Phase3Test, 주문_생성_RESERVED_상태) {
    Order o = service->create("S-001", "삼성전자", 50);
    EXPECT_EQ(o.status,       OrderStatus::RESERVED);
    EXPECT_EQ(o.sampleId,     "S-001");
    EXPECT_EQ(o.customerName, "삼성전자");
    EXPECT_EQ(o.quantity,     50);
}

TEST_F(Phase3Test, 주문번호_형식_ORD_YYYYMMDD_NNNN) {
    Order o = service->create("S-001", "고객A", 10);
    // ORD-20260612-0001 = 17자
    EXPECT_EQ(o.orderId.size(), 17u);
    EXPECT_EQ(o.orderId.substr(0, 4), "ORD-");
    // 날짜 8자리 숫자
    std::string datePart = o.orderId.substr(4, 8);
    EXPECT_TRUE(std::regex_match(datePart, std::regex("\\d{8}")));
    // NNNN 4자리 숫자
    std::string seqPart = o.orderId.substr(13, 4);
    EXPECT_TRUE(std::regex_match(seqPart, std::regex("\\d{4}")));
}

TEST_F(Phase3Test, 당일_순번_자동_증가) {
    Order o1 = service->create("S-001", "고객A", 10);
    Order o2 = service->create("S-001", "고객B", 20);
    Order o3 = service->create("S-002", "고객C", 30);

    EXPECT_EQ(extractNNNN(o1.orderId), 1);
    EXPECT_EQ(extractNNNN(o2.orderId), 2);
    EXPECT_EQ(extractNNNN(o3.orderId), 3);
}

TEST_F(Phase3Test, 주문번호_중복_없음) {
    Order o1 = service->create("S-001", "A사", 10);
    Order o2 = service->create("S-001", "B사", 10);
    EXPECT_NE(o1.orderId, o2.orderId);
}

TEST_F(Phase3Test, 미등록_시료_주문_거부) {
    EXPECT_THROW(service->create("S-999", "고객A", 10), std::invalid_argument);
}

TEST_F(Phase3Test, 수량_0이하_주문_거부) {
    EXPECT_THROW(service->create("S-001", "고객A",  0), std::invalid_argument);
    EXPECT_THROW(service->create("S-001", "고객A", -1), std::invalid_argument);
}

TEST_F(Phase3Test, 빈_고객명_주문_거부) {
    EXPECT_THROW(service->create("S-001", "", 10), std::invalid_argument);
}

TEST_F(Phase3Test, 주문_생성_후_목록에_포함) {
    service->create("S-001", "A사", 10);
    service->create("S-002", "B사", 20);
    EXPECT_EQ(service->all().size(), 2u);
}

TEST_F(Phase3Test, 주문_저장_후_재로드_일치) {
    Order o = service->create("S-001", "삼성전자", 100);
    repo->save();

    JsonRepository repo2(testDbDir);
    repo2.load();
    OrderService service2(repo2);

    ASSERT_EQ(service2.all().size(), 1u);
    EXPECT_EQ(service2.all()[0].orderId,      o.orderId);
    EXPECT_EQ(service2.all()[0].sampleId,     "S-001");
    EXPECT_EQ(service2.all()[0].customerName, "삼성전자");
    EXPECT_EQ(service2.all()[0].quantity,     100);
    EXPECT_EQ(service2.all()[0].status,       OrderStatus::RESERVED);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
