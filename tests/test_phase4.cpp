#include <gtest/gtest.h>
#include <filesystem>
#include <memory>
#include "repository/JsonRepository.h"
#include "service/ApprovalService.h"

namespace fs = std::filesystem;

class Phase4Test : public ::testing::Test {
protected:
    const std::string testDbDir = "test_db_phase4";
    std::unique_ptr<JsonRepository>  repo;
    std::unique_ptr<ApprovalService> service;

    void SetUp() override {
        fs::remove_all(testDbDir);
        repo    = std::make_unique<JsonRepository>(testDbDir);
        service = std::make_unique<ApprovalService>(*repo);
    }

    void TearDown() override {
        fs::remove_all(testDbDir);
    }

    void addSample(const std::string& id, double yield, double avgTime, int stock) {
        repo->samples().push_back({id, "시료-" + id, avgTime, yield, stock});
    }

    void addOrder(const std::string& orderId, const std::string& sampleId, int qty) {
        repo->orders().push_back(
            {orderId, sampleId, "고객", qty, OrderStatus::RESERVED, "2026-06-12 10:00:00"});
    }
};

// ── 공식 검증 ────────────────────────────────────────────────────────────────

TEST_F(Phase4Test, 실생산량_공식_예시1) {
    // ceil(170 / (0.92 * 0.9)) = ceil(205.33) = 206
    EXPECT_EQ(ApprovalService::calcActualProductQty(170, 0.92), 206);
}

TEST_F(Phase4Test, 실생산량_공식_예시2) {
    // ceil(150 / (0.88 * 0.9)) = ceil(189.39) = 190
    EXPECT_EQ(ApprovalService::calcActualProductQty(150, 0.88), 190);
}

TEST_F(Phase4Test, 실생산량_공식_경계값) {
    // ceil(1 / (1.0 * 0.9)) = ceil(1.11) = 2
    EXPECT_EQ(ApprovalService::calcActualProductQty(1, 1.0), 2);
}

TEST_F(Phase4Test, 총생산시간_계산) {
    // avgTime=0.8, qty=206 → 164.8 min
    EXPECT_NEAR(ApprovalService::calcTotalProductionTime(206, 0.8), 164.8, 0.001);
}

// ── 승인 흐름 ────────────────────────────────────────────────────────────────

TEST_F(Phase4Test, 재고_충분_CONFIRMED_전이) {
    addSample("S-001", 0.92, 0.5, 100);
    addOrder("ORD-0001", "S-001", 50);

    auto r = service->approve("ORD-0001");

    EXPECT_EQ(r.decision, ApprovalService::ApprovalResult::Decision::CONFIRMED);
    EXPECT_EQ(r.shortage, 0);
    EXPECT_EQ(repo->orders()[0].status, OrderStatus::CONFIRMED);
    EXPECT_EQ(repo->samples()[0].stock, 50);   // 100 - 50
    EXPECT_TRUE(repo->productionQueue().empty());
}

TEST_F(Phase4Test, 재고_부족_PRODUCING_전이) {
    addSample("S-001", 0.92, 0.8, 30);
    addOrder("ORD-0001", "S-001", 200);

    auto r = service->approve("ORD-0001");

    EXPECT_EQ(r.decision, ApprovalService::ApprovalResult::Decision::PRODUCING);
    EXPECT_EQ(r.shortage, 170);
    EXPECT_EQ(r.actualProductQty, 206);  // ceil(170 / (0.92 * 0.9))
    EXPECT_NEAR(r.totalProductionTime, 164.8, 0.001);
    EXPECT_EQ(repo->orders()[0].status, OrderStatus::PRODUCING);

    ASSERT_EQ(repo->productionQueue().size(), 1u);
    const auto& entry = repo->productionQueue()[0];
    EXPECT_EQ(entry.orderId,          "ORD-0001");
    EXPECT_EQ(entry.sampleId,         "S-001");
    EXPECT_EQ(entry.shortageQty,      170);
    EXPECT_EQ(entry.actualProductQty, 206);
    EXPECT_NEAR(entry.totalProductionTime, 164.8, 0.001);
    EXPECT_GT(entry.startedAtEpoch, 0LL);
}

TEST_F(Phase4Test, 거절_REJECTED_전이_재고_불변) {
    addSample("S-001", 0.92, 0.5, 100);
    addOrder("ORD-0001", "S-001", 50);

    service->reject("ORD-0001");

    EXPECT_EQ(repo->orders()[0].status, OrderStatus::REJECTED);
    EXPECT_EQ(repo->samples()[0].stock, 100);  // 재고 변동 없음
    EXPECT_TRUE(repo->productionQueue().empty());
}

TEST_F(Phase4Test, 재고_정확히_같을때_CONFIRMED) {
    addSample("S-001", 0.9, 0.5, 100);
    addOrder("ORD-0001", "S-001", 100);

    auto r = service->approve("ORD-0001");
    EXPECT_EQ(r.decision, ApprovalService::ApprovalResult::Decision::CONFIRMED);
    EXPECT_EQ(repo->samples()[0].stock, 0);
}

// ── 연속 승인 독립 재고 체크 ─────────────────────────────────────────────────

TEST_F(Phase4Test, 동일시료_연속_3건_독립재고체크) {
    // 재고 200 → 100씩 3건 승인 → 첫 2건 CONFIRMED, 3번째 PRODUCING
    addSample("S-001", 0.92, 0.5, 200);
    addOrder("ORD-0001", "S-001", 100);
    addOrder("ORD-0002", "S-001", 100);
    addOrder("ORD-0003", "S-001", 100);

    auto r1 = service->approve("ORD-0001");
    auto r2 = service->approve("ORD-0002");
    auto r3 = service->approve("ORD-0003");

    EXPECT_EQ(r1.decision, ApprovalService::ApprovalResult::Decision::CONFIRMED);
    EXPECT_EQ(r2.decision, ApprovalService::ApprovalResult::Decision::CONFIRMED);
    EXPECT_EQ(r3.decision, ApprovalService::ApprovalResult::Decision::PRODUCING);
    EXPECT_EQ(r3.shortage, 100);
    EXPECT_EQ(repo->samples()[0].stock, 0);
    EXPECT_EQ(repo->productionQueue().size(), 1u);
}

TEST_F(Phase4Test, 재고0에서_주문_PRODUCING) {
    addSample("S-001", 0.9, 0.3, 0);
    addOrder("ORD-0001", "S-001", 50);

    auto r = service->approve("ORD-0001");
    EXPECT_EQ(r.decision, ApprovalService::ApprovalResult::Decision::PRODUCING);
    EXPECT_EQ(r.shortage, 50);
}

// ── 오류 케이스 ───────────────────────────────────────────────────────────────

TEST_F(Phase4Test, 없는_주문_승인_예외) {
    EXPECT_THROW(service->approve("ORD-XXXX"), std::invalid_argument);
}

TEST_F(Phase4Test, RESERVED_아닌_주문_승인_예외) {
    addSample("S-001", 0.92, 0.5, 100);
    repo->orders().push_back({"ORD-0001", "S-001", "고객", 50,
                               OrderStatus::CONFIRMED, "2026-06-12 10:00:00"});
    EXPECT_THROW(service->approve("ORD-0001"), std::logic_error);
}

TEST_F(Phase4Test, 없는_주문_거절_예외) {
    EXPECT_THROW(service->reject("ORD-XXXX"), std::invalid_argument);
}

TEST_F(Phase4Test, RESERVED_아닌_주문_거절_예외) {
    addSample("S-001", 0.92, 0.5, 100);
    repo->orders().push_back({"ORD-0001", "S-001", "고객", 50,
                               OrderStatus::PRODUCING, "2026-06-12 10:00:00"});
    EXPECT_THROW(service->reject("ORD-0001"), std::logic_error);
}

// ── 영속성 ───────────────────────────────────────────────────────────────────

TEST_F(Phase4Test, 생산큐_저장_후_재로드_일치) {
    addSample("S-001", 0.92, 0.8, 30);
    addOrder("ORD-0001", "S-001", 200);
    service->approve("ORD-0001");
    repo->save();

    JsonRepository  repo2(testDbDir);
    repo2.load();
    ApprovalService svc2(repo2);

    ASSERT_EQ(repo2.productionQueue().size(), 1u);
    EXPECT_EQ(repo2.productionQueue()[0].orderId,          "ORD-0001");
    EXPECT_EQ(repo2.productionQueue()[0].shortageQty,      170);
    EXPECT_EQ(repo2.productionQueue()[0].actualProductQty, 206);
    EXPECT_EQ(repo2.orders()[0].status, OrderStatus::PRODUCING);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
