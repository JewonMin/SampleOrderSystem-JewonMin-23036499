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

    void addOrder(const std::string& id, OrderStatus status,
                  const std::string& sampleId = "S-001", int qty = 50) {
        repo->orders().push_back({id, sampleId, "고객", qty, status, "2026-06-12 10:00:00"});
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
    // REJECTED는 OrderSummary에 없음
}

TEST_F(Phase7Test, orderSummary_REJECTED_제외) {
    addOrder("ORD-001", OrderStatus::REJECTED);
    addOrder("ORD-002", OrderStatus::REJECTED);

    auto s = service->orderSummary();
    EXPECT_EQ(s.reserved + s.producing + s.confirmed + s.released, 0);
}

TEST_F(Phase7Test, orderSummary_주문_없으면_모두_0) {
    auto s = service->orderSummary();
    EXPECT_EQ(s.reserved + s.producing + s.confirmed + s.released, 0);
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
    addOrder("ORD-001", OrderStatus::PRODUCING, "S-002");
    addProductionEntry("ORD-001", "S-002", 150);

    auto stocks = service->stockStatus();
    ASSERT_EQ(stocks.size(), 2u);
    EXPECT_EQ(stocks[0].inProductionQty, 0);    // S-001: 생산 없음
    EXPECT_EQ(stocks[1].inProductionQty, 150);  // S-002: 생산 중
}

// ── 재고 상태 분류 (여유/부족/고갈) ──────────────────────────────────────────

TEST_F(Phase7Test, stockStatus_재고0_고갈) {
    addSample("S-001", 0);

    auto stocks = service->stockStatus();
    ASSERT_EQ(stocks.size(), 1u);
    EXPECT_EQ(stocks[0].status, MonitorService::StockStatus::DEPLETED);
}

TEST_F(Phase7Test, stockStatus_주문없으면_여유) {
    addSample("S-001", 100);

    auto stocks = service->stockStatus();
    ASSERT_EQ(stocks.size(), 1u);
    EXPECT_EQ(stocks[0].status, MonitorService::StockStatus::SUFFICIENT);
}

TEST_F(Phase7Test, stockStatus_재고가_주문보다_부족_SHORTAGE) {
    addSample("S-001", 30);
    addOrder("ORD-001", OrderStatus::RESERVED, "S-001", 100);

    auto stocks = service->stockStatus();
    ASSERT_EQ(stocks.size(), 1u);
    EXPECT_EQ(stocks[0].status, MonitorService::StockStatus::SHORTAGE);
    EXPECT_EQ(stocks[0].demandQty, 100);
}

TEST_F(Phase7Test, stockStatus_재고가_주문과_같으면_여유) {
    addSample("S-001", 100);
    addOrder("ORD-001", OrderStatus::RESERVED, "S-001", 100);

    auto stocks = service->stockStatus();
    ASSERT_EQ(stocks.size(), 1u);
    EXPECT_EQ(stocks[0].status, MonitorService::StockStatus::SUFFICIENT);
}

TEST_F(Phase7Test, stockStatus_PRODUCING_포함_부족판단) {
    addSample("S-001", 50);
    addOrder("ORD-001", OrderStatus::RESERVED,  "S-001", 30);
    addOrder("ORD-002", OrderStatus::PRODUCING, "S-001", 40);
    // demandQty = 30 + 40 = 70 > 50 → SHORTAGE

    auto stocks = service->stockStatus();
    ASSERT_EQ(stocks.size(), 1u);
    EXPECT_EQ(stocks[0].status, MonitorService::StockStatus::SHORTAGE);
    EXPECT_EQ(stocks[0].demandQty, 70);
}

TEST_F(Phase7Test, stockStatus_CONFIRMED_RELEASED_수요에_미포함) {
    addSample("S-001", 5);
    addOrder("ORD-001", OrderStatus::CONFIRMED, "S-001", 1000);
    addOrder("ORD-002", OrderStatus::RELEASED,  "S-001", 1000);
    // CONFIRMED/RELEASED는 demandQty에 포함 안 됨 → demandQty=0 → DEPLETED(stock=5은 아님) → SUFFICIENT

    auto stocks = service->stockStatus();
    ASSERT_EQ(stocks.size(), 1u);
    EXPECT_EQ(stocks[0].demandQty, 0);
    EXPECT_EQ(stocks[0].status, MonitorService::StockStatus::SUFFICIENT);
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
