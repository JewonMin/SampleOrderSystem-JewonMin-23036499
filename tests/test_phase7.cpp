#include <gtest/gtest.h>
#include <filesystem>
#include <memory>
#include <ctime>
#include "repository/JsonRepository.h"
#include "service/MonitorService.h"

namespace fs = std::filesystem;

class Phase7Test : public ::testing::Test {
protected:
    const std::string testDbDir = "test_db_phase7";
    std::unique_ptr<JsonRepository> repo;
    std::unique_ptr<MonitorService> service;

    void SetUp() override {
        fs::remove_all(testDbDir);
        repo    = std::make_unique<JsonRepository>(testDbDir);
        service = std::make_unique<MonitorService>(*repo);
    }

    void TearDown() override {
        fs::remove_all(testDbDir);
    }

    void addOrder(const std::string& id, OrderStatus status) {
        repo->orders().push_back({id, "S-001", "고객", 50, status, "2026-06-12 10:00:00"});
    }

    void addSample(const std::string& id, int stock) {
        repo->samples().push_back({id, "시료-" + id, 0.5, 0.92, stock});
    }

    void addProductionEntry(const std::string& orderId, const std::string& sampleId,
                            int actualQty) {
        long long now = static_cast<long long>(std::time(nullptr));
        repo->productionQueue().push_back(
            {orderId, sampleId, 50, actualQty, 100.0, now, "2026-06-12 10:00:00"});
    }
};

// ── orderSummary ──────────────────────────────────────────────────────────────

TEST_F(Phase7Test, orderSummary_초기_모두_0) {
    auto s = service->orderSummary();
    EXPECT_EQ(s.reserved,  0);
    EXPECT_EQ(s.producing, 0);
    EXPECT_EQ(s.confirmed, 0);
    EXPECT_EQ(s.released,  0);
    EXPECT_EQ(s.rejected,  0);
}

TEST_F(Phase7Test, orderSummary_각_상태_카운트_정확) {
    addOrder("ORD-001", OrderStatus::RESERVED);
    addOrder("ORD-002", OrderStatus::RESERVED);
    addOrder("ORD-003", OrderStatus::PRODUCING);
    addOrder("ORD-004", OrderStatus::CONFIRMED);
    addOrder("ORD-005", OrderStatus::RELEASED);
    addOrder("ORD-006", OrderStatus::RELEASED);
    addOrder("ORD-007", OrderStatus::REJECTED);

    auto s = service->orderSummary();
    EXPECT_EQ(s.reserved,  2);
    EXPECT_EQ(s.producing, 1);
    EXPECT_EQ(s.confirmed, 1);
    EXPECT_EQ(s.released,  2);
    EXPECT_EQ(s.rejected,  1);
}

TEST_F(Phase7Test, orderSummary_주문_없으면_모두_0) {
    auto s = service->orderSummary();
    EXPECT_EQ(s.reserved + s.producing + s.confirmed + s.released + s.rejected, 0);
}

// ── stockStatus ───────────────────────────────────────────────────────────────

TEST_F(Phase7Test, stockStatus_시료_없으면_빈_목록) {
    EXPECT_TRUE(service->stockStatus().empty());
}

TEST_F(Phase7Test, stockStatus_재고_반환) {
    addSample("S-001", 100);
    addSample("S-002", 50);

    auto stocks = service->stockStatus();
    ASSERT_EQ(stocks.size(), 2u);
    EXPECT_EQ(stocks[0].id,    "S-001");
    EXPECT_EQ(stocks[0].stock, 100);
    EXPECT_EQ(stocks[1].id,    "S-002");
    EXPECT_EQ(stocks[1].stock, 50);
}

TEST_F(Phase7Test, stockStatus_생산중_없으면_inProductionQty_0) {
    addSample("S-001", 30);

    auto stocks = service->stockStatus();
    ASSERT_EQ(stocks.size(), 1u);
    EXPECT_EQ(stocks[0].inProductionQty, 0);
}

TEST_F(Phase7Test, stockStatus_생산중_수량_합산) {
    addSample("S-001", 30);
    addOrder("ORD-001", OrderStatus::PRODUCING);
    addOrder("ORD-002", OrderStatus::PRODUCING);
    addProductionEntry("ORD-001", "S-001", 206);
    addProductionEntry("ORD-002", "S-001", 100);

    auto stocks = service->stockStatus();
    ASSERT_EQ(stocks.size(), 1u);
    EXPECT_EQ(stocks[0].inProductionQty, 306);  // 206 + 100
}

TEST_F(Phase7Test, stockStatus_다른_시료_생산중_분리) {
    addSample("S-001", 10);
    addSample("S-002", 20);
    addOrder("ORD-001", OrderStatus::PRODUCING);
    addProductionEntry("ORD-001", "S-002", 150);

    auto stocks = service->stockStatus();
    ASSERT_EQ(stocks.size(), 2u);
    EXPECT_EQ(stocks[0].inProductionQty, 0);    // S-001: 생산 없음
    EXPECT_EQ(stocks[1].inProductionQty, 150);  // S-002: 생산 중
}

// ── 영속성 (데이터 반영) ──────────────────────────────────────────────────────

TEST_F(Phase7Test, 주문_상태변경_후_summary_반영) {
    addOrder("ORD-001", OrderStatus::RESERVED);

    auto s1 = service->orderSummary();
    EXPECT_EQ(s1.reserved, 1);

    repo->orders()[0].status = OrderStatus::CONFIRMED;

    auto s2 = service->orderSummary();
    EXPECT_EQ(s2.reserved,  0);
    EXPECT_EQ(s2.confirmed, 1);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
