#include <gtest/gtest.h>
#include <filesystem>
#include "model/Sample.h"
#include "model/Order.h"
#include "repository/JsonRepository.h"

namespace fs = std::filesystem;

class Phase1Test : public ::testing::Test {
protected:
    const std::string testDbDir = "test_db_phase1";

    void SetUp() override {
        fs::remove_all(testDbDir);
    }

    void TearDown() override {
        fs::remove_all(testDbDir);
    }
};

TEST_F(Phase1Test, Sample_직렬화_역직렬화_왕복) {
    Sample s{"S-001", "실리콘 웨이퍼-8인치", 0.5, 0.92, 480};
    nlohmann::json j = s;
    Sample s2 = j.get<Sample>();

    EXPECT_EQ(s2.id, "S-001");
    EXPECT_EQ(s2.name, "실리콘 웨이퍼-8인치");
    EXPECT_DOUBLE_EQ(s2.avgProductionTime, 0.5);
    EXPECT_DOUBLE_EQ(s2.yield, 0.92);
    EXPECT_EQ(s2.stock, 480);
}

TEST_F(Phase1Test, Order_직렬화_역직렬화_왕복) {
    Order o{"ORD-20260416-0001", "S-001", "삼성전자", 200, OrderStatus::RESERVED, "2026-04-16 09:00:00"};
    nlohmann::json j = o;
    Order o2 = j.get<Order>();

    EXPECT_EQ(o2.orderId, "ORD-20260416-0001");
    EXPECT_EQ(o2.sampleId, "S-001");
    EXPECT_EQ(o2.customerName, "삼성전자");
    EXPECT_EQ(o2.quantity, 200);
    EXPECT_EQ(o2.status, OrderStatus::RESERVED);
    EXPECT_EQ(o2.createdAt, "2026-04-16 09:00:00");
}

TEST_F(Phase1Test, OrderStatus_모든_상태_직렬화_검증) {
    using S = OrderStatus;
    EXPECT_EQ(nlohmann::json(S::RESERVED) .get<std::string>(), "RESERVED");
    EXPECT_EQ(nlohmann::json(S::REJECTED) .get<std::string>(), "REJECTED");
    EXPECT_EQ(nlohmann::json(S::PRODUCING).get<std::string>(), "PRODUCING");
    EXPECT_EQ(nlohmann::json(S::CONFIRMED).get<std::string>(), "CONFIRMED");
    EXPECT_EQ(nlohmann::json(S::RELEASED) .get<std::string>(), "RELEASED");
}

TEST_F(Phase1Test, OrderStatus_문자열_역직렬화_검증) {
    using S = OrderStatus;
    EXPECT_EQ(nlohmann::json("RESERVED") .get<S>(), S::RESERVED);
    EXPECT_EQ(nlohmann::json("REJECTED") .get<S>(), S::REJECTED);
    EXPECT_EQ(nlohmann::json("PRODUCING").get<S>(), S::PRODUCING);
    EXPECT_EQ(nlohmann::json("CONFIRMED").get<S>(), S::CONFIRMED);
    EXPECT_EQ(nlohmann::json("RELEASED") .get<S>(), S::RELEASED);
}

TEST_F(Phase1Test, Repository_저장_후_로드_데이터_일치) {
    JsonRepository repo(testDbDir);
    repo.samples().push_back({"S-001", "실리콘 웨이퍼-8인치", 0.5, 0.92, 480});
    repo.orders().push_back({"ORD-20260416-0001", "S-001", "삼성전자", 100, OrderStatus::RESERVED, "2026-04-16 09:00:00"});
    repo.save();

    JsonRepository repo2(testDbDir);
    repo2.load();

    ASSERT_EQ(repo2.samples().size(), 1u);
    EXPECT_EQ(repo2.samples()[0].id, "S-001");
    EXPECT_DOUBLE_EQ(repo2.samples()[0].yield, 0.92);
    EXPECT_EQ(repo2.samples()[0].stock, 480);

    ASSERT_EQ(repo2.orders().size(), 1u);
    EXPECT_EQ(repo2.orders()[0].orderId, "ORD-20260416-0001");
    EXPECT_EQ(repo2.orders()[0].status, OrderStatus::RESERVED);
}

TEST_F(Phase1Test, Repository_여러_주문_상태_왕복) {
    JsonRepository repo(testDbDir);
    repo.orders().push_back({"ORD-0001", "S-001", "A사", 10, OrderStatus::PRODUCING,  "2026-01-01 00:00:00"});
    repo.orders().push_back({"ORD-0002", "S-001", "B사", 20, OrderStatus::CONFIRMED,  "2026-01-02 00:00:00"});
    repo.orders().push_back({"ORD-0003", "S-001", "C사", 30, OrderStatus::RELEASED,   "2026-01-03 00:00:00"});
    repo.orders().push_back({"ORD-0004", "S-001", "D사", 40, OrderStatus::REJECTED,   "2026-01-04 00:00:00"});
    repo.save();

    JsonRepository repo2(testDbDir);
    repo2.load();

    ASSERT_EQ(repo2.orders().size(), 4u);
    EXPECT_EQ(repo2.orders()[0].status, OrderStatus::PRODUCING);
    EXPECT_EQ(repo2.orders()[1].status, OrderStatus::CONFIRMED);
    EXPECT_EQ(repo2.orders()[2].status, OrderStatus::RELEASED);
    EXPECT_EQ(repo2.orders()[3].status, OrderStatus::REJECTED);
}

TEST_F(Phase1Test, Repository_빈_DB_로드_시_빈_컬렉션) {
    JsonRepository repo(testDbDir);
    repo.load();

    EXPECT_TRUE(repo.samples().empty());
    EXPECT_TRUE(repo.orders().empty());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
