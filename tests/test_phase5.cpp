#include <gtest/gtest.h>
#include <filesystem>
#include <memory>
#include <ctime>
#include "repository/JsonRepository.h"
#include "service/ProductionService.h"

namespace fs = std::filesystem;

class Phase5Test : public ::testing::Test {
protected:
    const std::string testDbDir = "test_db_phase5";
    std::unique_ptr<JsonRepository>    repo;
    std::unique_ptr<ProductionService> service;

    void SetUp() override {
        fs::remove_all(testDbDir);
        repo    = std::make_unique<JsonRepository>(testDbDir);
        service = std::make_unique<ProductionService>(*repo);
    }

    void TearDown() override {
        fs::remove_all(testDbDir);
    }

    void addSample(const std::string& id, int stock = 100) {
        repo->samples().push_back({id, "시료-" + id, 0.5, 0.92, stock});
    }

    void addProducingOrder(const std::string& orderId, const std::string& sampleId,
                           int quantity, int shortage, int actualQty,
                           double totalTime, long long startEpoch = 0) {
        repo->orders().push_back(
            {orderId, sampleId, "고객", quantity,
             OrderStatus::PRODUCING, "2026-06-12 10:00:00"});
        if (startEpoch == 0) startEpoch = static_cast<long long>(std::time(nullptr));
        repo->productionQueue().push_back(
            {orderId, sampleId, shortage, actualQty,
             totalTime, startEpoch, "2026-06-12 10:00:00"});
    }
};

// ── list ──────────────────────────────────────────────────────────────────────

TEST_F(Phase5Test, list_빈_큐_빈_목록) {
    EXPECT_TRUE(service->list().empty());
}

TEST_F(Phase5Test, list_항목_개수_일치) {
    addSample("S-001");
    addSample("S-002");
    addProducingOrder("ORD-0001", "S-001", 200, 170, 206, 164.8);
    addProducingOrder("ORD-0002", "S-002", 100, 80,  97,  48.5);

    auto items = service->list();
    ASSERT_EQ(items.size(), 2u);
    EXPECT_EQ(items[0].orderId, "ORD-0001");
    EXPECT_EQ(items[1].orderId, "ORD-0002");
}

TEST_F(Phase5Test, list_시료명_포함) {
    addSample("S-001");
    addProducingOrder("ORD-0001", "S-001", 200, 170, 206, 164.8);

    auto items = service->list();
    ASSERT_EQ(items.size(), 1u);
    EXPECT_EQ(items[0].sampleName, "시료-S-001");
    EXPECT_EQ(items[0].actualProductQty, 206);
    EXPECT_NEAR(items[0].totalProductionTime, 164.8, 0.001);
}

TEST_F(Phase5Test, 진행률_시작_직후_0에_근접) {
    addSample("S-001");
    long long now = static_cast<long long>(std::time(nullptr));
    addProducingOrder("ORD-0001", "S-001", 200, 170, 206, 1000.0, now);

    auto items = service->list();
    ASSERT_EQ(items.size(), 1u);
    // 방금 시작했으므로 진행률은 거의 0
    EXPECT_LT(items[0].progressPct, 1.0);
}

TEST_F(Phase5Test, 진행률_충분한_시간_경과시_100) {
    addSample("S-001");
    // 100000초 전에 시작, 총 1분 → 완료 상태
    long long past = static_cast<long long>(std::time(nullptr)) - 100000LL;
    addProducingOrder("ORD-0001", "S-001", 200, 170, 206, 1.0, past);

    auto items = service->list();
    ASSERT_EQ(items.size(), 1u);
    EXPECT_NEAR(items[0].progressPct, 100.0, 0.001);
}

TEST_F(Phase5Test, 진행률_100_초과하지_않음) {
    addSample("S-001");
    long long veryOld = 1000LL;  // 1970년대 → elapsed 엄청 큼
    addProducingOrder("ORD-0001", "S-001", 200, 170, 206, 0.001, veryOld);

    auto items = service->list();
    ASSERT_EQ(items.size(), 1u);
    EXPECT_LE(items[0].progressPct, 100.0);
}

// ── complete ──────────────────────────────────────────────────────────────────

TEST_F(Phase5Test, complete_주문_CONFIRMED_전이) {
    addSample("S-001", 30);
    addProducingOrder("ORD-0001", "S-001", 200, 170, 206, 164.8);

    service->complete("ORD-0001");

    EXPECT_EQ(repo->orders()[0].status, OrderStatus::CONFIRMED);
}

TEST_F(Phase5Test, complete_큐에서_제거) {
    addSample("S-001", 30);
    addProducingOrder("ORD-0001", "S-001", 200, 170, 206, 164.8);

    service->complete("ORD-0001");

    EXPECT_TRUE(repo->productionQueue().empty());
}

TEST_F(Phase5Test, complete_재고_정확히_갱신) {
    // stock=30, actualQty=206, orderQty=200 → 30 + (206-200) = 36
    addSample("S-001", 30);
    addProducingOrder("ORD-0001", "S-001", 200, 170, 206, 164.8);

    service->complete("ORD-0001");

    EXPECT_EQ(repo->samples()[0].stock, 36);
}

TEST_F(Phase5Test, complete_초과생산_없을때_재고_0) {
    // actualQty == orderQty → 재고 변동 없음 (기존 재고만 남음)
    addSample("S-001", 50);
    addProducingOrder("ORD-0001", "S-001", 100, 50, 100, 50.0);
    // stock=50, actualQty=100, orderQty=100 → 50 + (100-100) = 50

    service->complete("ORD-0001");

    EXPECT_EQ(repo->samples()[0].stock, 50);
}

TEST_F(Phase5Test, complete_여러_항목_중_하나만_처리) {
    addSample("S-001", 30);
    addSample("S-002", 0);
    addProducingOrder("ORD-0001", "S-001", 200, 170, 206, 164.8);
    addProducingOrder("ORD-0002", "S-002", 50,  50,  61,  30.5);

    service->complete("ORD-0001");

    EXPECT_EQ(repo->productionQueue().size(), 1u);
    EXPECT_EQ(repo->productionQueue()[0].orderId, "ORD-0002");
    EXPECT_EQ(repo->orders()[0].status, OrderStatus::CONFIRMED);
    EXPECT_EQ(repo->orders()[1].status, OrderStatus::PRODUCING);
}

// ── 오류 케이스 ───────────────────────────────────────────────────────────────

TEST_F(Phase5Test, complete_없는_orderId_예외) {
    EXPECT_THROW(service->complete("ORD-XXXX"), std::logic_error);
}

TEST_F(Phase5Test, complete_FIFO_두번째_항목_직접완료_예외) {
    addSample("S-001", 0);
    addSample("S-002", 0);
    addProducingOrder("ORD-0001", "S-001", 100, 100, 121, 96.8);
    addProducingOrder("ORD-0002", "S-002", 50,  50,  61,  48.8);

    // 두 번째 항목을 직접 완료 시도 → FIFO 위반 예외
    EXPECT_THROW(service->complete("ORD-0002"), std::logic_error);
    // 첫 번째 항목은 완료 가능
    EXPECT_NO_THROW(service->complete("ORD-0001"));
}

TEST_F(Phase5Test, complete_PRODUCING_아닌_주문_예외) {
    addSample("S-001", 30);
    // 큐에 첫 번째로 추가, 그러나 주문 상태는 CONFIRMED
    repo->orders().push_back(
        {"ORD-0001", "S-001", "고객", 200, OrderStatus::CONFIRMED, "2026-06-12 10:00:00"});
    long long now = static_cast<long long>(std::time(nullptr));
    repo->productionQueue().push_back(
        {"ORD-0001", "S-001", 170, 206, 164.8, now, "2026-06-12 10:00:00"});

    EXPECT_THROW(service->complete("ORD-0001"), std::logic_error);
}

// ── 영속성 ───────────────────────────────────────────────────────────────────

TEST_F(Phase5Test, complete_후_저장_재로드_일치) {
    addSample("S-001", 30);
    addProducingOrder("ORD-0001", "S-001", 200, 170, 206, 164.8);

    service->complete("ORD-0001");
    repo->save();

    JsonRepository    repo2(testDbDir);
    repo2.load();
    ProductionService svc2(repo2);

    EXPECT_TRUE(repo2.productionQueue().empty());
    EXPECT_EQ(repo2.orders()[0].status, OrderStatus::CONFIRMED);
    EXPECT_EQ(repo2.samples()[0].stock, 36);
    EXPECT_TRUE(svc2.list().empty());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
