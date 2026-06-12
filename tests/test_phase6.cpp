#include <gtest/gtest.h>
#include <filesystem>
#include <memory>
#include "repository/JsonRepository.h"
#include "service/ReleaseService.h"

namespace fs = std::filesystem;

class Phase6Test : public ::testing::Test {
protected:
    const std::string testDbDir = "test_db_phase6";
    std::unique_ptr<JsonRepository> repo;
    std::unique_ptr<ReleaseService> service;

    void SetUp() override {
        fs::remove_all(testDbDir);
        repo    = std::make_unique<JsonRepository>(testDbDir);
        service = std::make_unique<ReleaseService>(*repo);
    }

    void TearDown() override {
        fs::remove_all(testDbDir);
    }

    void addOrder(const std::string& orderId, OrderStatus status) {
        repo->orders().push_back(
            {orderId, "S-001", "고객", 50, status, "2026-06-12 10:00:00"});
    }
};

// ── confirmedOrders ───────────────────────────────────────────────────────────

TEST_F(Phase6Test, confirmedOrders_빈_목록) {
    EXPECT_TRUE(service->confirmedOrders().empty());
}

TEST_F(Phase6Test, confirmedOrders_CONFIRMED만_반환) {
    addOrder("ORD-0001", OrderStatus::CONFIRMED);
    addOrder("ORD-0002", OrderStatus::RESERVED);
    addOrder("ORD-0003", OrderStatus::PRODUCING);
    addOrder("ORD-0004", OrderStatus::REJECTED);
    addOrder("ORD-0005", OrderStatus::RELEASED);

    auto list = service->confirmedOrders();
    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(list[0]->orderId, "ORD-0001");
}

TEST_F(Phase6Test, confirmedOrders_여러건_반환) {
    addOrder("ORD-0001", OrderStatus::CONFIRMED);
    addOrder("ORD-0002", OrderStatus::CONFIRMED);
    addOrder("ORD-0003", OrderStatus::RESERVED);

    EXPECT_EQ(service->confirmedOrders().size(), 2u);
}

// ── release ───────────────────────────────────────────────────────────────────

TEST_F(Phase6Test, release_CONFIRMED_RELEASED_전이) {
    addOrder("ORD-0001", OrderStatus::CONFIRMED);

    service->release("ORD-0001");

    EXPECT_EQ(repo->orders()[0].status, OrderStatus::RELEASED);
}

TEST_F(Phase6Test, release_후_confirmedOrders_목록에서_제거) {
    addOrder("ORD-0001", OrderStatus::CONFIRMED);
    addOrder("ORD-0002", OrderStatus::CONFIRMED);

    service->release("ORD-0001");

    auto list = service->confirmedOrders();
    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(list[0]->orderId, "ORD-0002");
}

TEST_F(Phase6Test, release_재고_변동_없음) {
    repo->samples().push_back({"S-001", "시료", 0.5, 0.92, 100});
    addOrder("ORD-0001", OrderStatus::CONFIRMED);

    service->release("ORD-0001");

    EXPECT_EQ(repo->samples()[0].stock, 100);
}

// ── 오류 케이스 ───────────────────────────────────────────────────────────────

TEST_F(Phase6Test, release_없는_주문_예외) {
    EXPECT_THROW(service->release("ORD-XXXX"), std::invalid_argument);
}

TEST_F(Phase6Test, release_RESERVED_주문_예외) {
    addOrder("ORD-0001", OrderStatus::RESERVED);
    EXPECT_THROW(service->release("ORD-0001"), std::logic_error);
}

TEST_F(Phase6Test, release_PRODUCING_주문_예외) {
    addOrder("ORD-0001", OrderStatus::PRODUCING);
    EXPECT_THROW(service->release("ORD-0001"), std::logic_error);
}

TEST_F(Phase6Test, release_이미_RELEASED_주문_예외) {
    addOrder("ORD-0001", OrderStatus::RELEASED);
    EXPECT_THROW(service->release("ORD-0001"), std::logic_error);
}

// ── 영속성 ───────────────────────────────────────────────────────────────────

TEST_F(Phase6Test, release_후_저장_재로드_일치) {
    addOrder("ORD-0001", OrderStatus::CONFIRMED);
    service->release("ORD-0001");
    repo->save();

    JsonRepository  repo2(testDbDir);
    repo2.load();
    ReleaseService  svc2(repo2);

    EXPECT_EQ(repo2.orders()[0].status, OrderStatus::RELEASED);
    EXPECT_TRUE(svc2.confirmedOrders().empty());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
