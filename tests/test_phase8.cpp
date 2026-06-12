#include <gtest/gtest.h>
#include <filesystem>
#include <memory>
#include <ctime>
#include "repository/JsonRepository.h"
#include "service/SampleService.h"
#include "service/OrderService.h"
#include "service/ApprovalService.h"
#include "service/ProductionService.h"
#include "service/ReleaseService.h"
#include "service/MonitorService.h"

namespace fs = std::filesystem;

// 전 서비스를 함께 사용하는 통합 테스트
class Phase8Test : public ::testing::Test {
protected:
    const std::string testDbDir = "test_db_phase8";
    std::unique_ptr<JsonRepository>    repo;
    std::unique_ptr<SampleService>     sampleSvc;
    std::unique_ptr<OrderService>      orderSvc;
    std::unique_ptr<ApprovalService>   approvalSvc;
    std::unique_ptr<ProductionService> productionSvc;
    std::unique_ptr<ReleaseService>    releaseSvc;
    std::unique_ptr<MonitorService>    monitorSvc;

    void SetUp() override {
        fs::remove_all(testDbDir);
        repo          = std::make_unique<JsonRepository>(testDbDir);
        sampleSvc     = std::make_unique<SampleService>(*repo);
        orderSvc      = std::make_unique<OrderService>(*repo);
        approvalSvc   = std::make_unique<ApprovalService>(*repo);
        productionSvc = std::make_unique<ProductionService>(*repo);
        releaseSvc    = std::make_unique<ReleaseService>(*repo);
        monitorSvc    = std::make_unique<MonitorService>(*repo);
    }

    void TearDown() override {
        fs::remove_all(testDbDir);
    }
};

// ── 전체 플로우: 재고 충분 ────────────────────────────────────────────────────

TEST_F(Phase8Test, 전체플로우_재고충분_RESERVED_CONFIRMED_RELEASED) {
    sampleSvc->add("실리콘A", 0.5, 0.92, 200);
    const std::string sid = repo->samples()[0].id;

    const std::string oid = orderSvc->create(sid, "고객사A", 100).orderId;
    EXPECT_EQ(repo->orders()[0].status, OrderStatus::RESERVED);

    auto r = approvalSvc->approve(oid);
    EXPECT_EQ(r.decision, ApprovalService::ApprovalResult::Decision::CONFIRMED);
    EXPECT_EQ(repo->orders()[0].status, OrderStatus::CONFIRMED);
    EXPECT_EQ(repo->samples()[0].stock, 100);  // 200 - 100

    releaseSvc->release(oid);
    EXPECT_EQ(repo->orders()[0].status, OrderStatus::RELEASED);
    EXPECT_EQ(repo->samples()[0].stock, 100);  // 재고 변동 없음
}

// ── 전체 플로우: 재고 부족 ────────────────────────────────────────────────────

TEST_F(Phase8Test, 전체플로우_재고부족_PRODUCING_CONFIRMED_RELEASED) {
    sampleSvc->add("갈륨비소", 0.5, 0.92, 30);
    const std::string sid = repo->samples()[0].id;

    const std::string oid = orderSvc->create(sid, "고객사B", 200).orderId;

    auto r = approvalSvc->approve(oid);
    EXPECT_EQ(r.decision, ApprovalService::ApprovalResult::Decision::PRODUCING);
    EXPECT_EQ(repo->orders()[0].status, OrderStatus::PRODUCING);
    EXPECT_EQ(repo->samples()[0].stock, 30);   // 생산 승인 시 재고 불변
    ASSERT_EQ(repo->productionQueue().size(), 1u);

    productionSvc->complete(oid);
    EXPECT_EQ(repo->orders()[0].status, OrderStatus::CONFIRMED);
    EXPECT_TRUE(repo->productionQueue().empty());
    // stock = 30 + (206 - 200) = 36
    EXPECT_EQ(repo->samples()[0].stock, 36);

    releaseSvc->release(oid);
    EXPECT_EQ(repo->orders()[0].status, OrderStatus::RELEASED);
}

// ── 전체 플로우: 주문 거절 ────────────────────────────────────────────────────

TEST_F(Phase8Test, 전체플로우_주문거절_재고_불변) {
    sampleSvc->add("인듐인", 0.5, 0.92, 50);
    const std::string sid = repo->samples()[0].id;
    const std::string oid = orderSvc->create(sid, "고객사C", 30).orderId;

    approvalSvc->reject(oid);
    EXPECT_EQ(repo->orders()[0].status, OrderStatus::REJECTED);
    EXPECT_EQ(repo->samples()[0].stock, 50);   // 거절 시 재고 불변
}

// ── 모니터 카운트 정확성 ──────────────────────────────────────────────────────

TEST_F(Phase8Test, 모니터_카운트_전체_플로우_반영) {
    sampleSvc->add("테스트시료", 0.5, 0.92, 500);
    const std::string sid = repo->samples()[0].id;

    // 3건 주문 생성
    auto o1 = orderSvc->create(sid, "고객1", 50).orderId;
    auto o2 = orderSvc->create(sid, "고객2", 50).orderId;
    auto o3 = orderSvc->create(sid, "고객3", 50).orderId;

    auto s0 = monitorSvc->orderSummary();
    EXPECT_EQ(s0.reserved, 3);

    // o1 승인
    approvalSvc->approve(o1);
    // o2 거절
    approvalSvc->reject(o2);
    // o3 그대로

    auto s1 = monitorSvc->orderSummary();
    EXPECT_EQ(s1.reserved,  1);
    EXPECT_EQ(s1.confirmed, 1);
    // REJECTED는 OrderSummary 대상 제외 (order-status.md)

    // o1 출고
    releaseSvc->release(o1);

    auto s2 = monitorSvc->orderSummary();
    EXPECT_EQ(s2.confirmed, 0);
    EXPECT_EQ(s2.released,  1);
}

// ── 재고 일관성 ───────────────────────────────────────────────────────────────

TEST_F(Phase8Test, 연속_승인_재고_일관성) {
    sampleSvc->add("웨이퍼", 0.5, 0.92, 300);
    const std::string sid = repo->samples()[0].id;

    auto o1 = orderSvc->create(sid, "A사", 100).orderId;
    auto o2 = orderSvc->create(sid, "B사", 100).orderId;
    auto o3 = orderSvc->create(sid, "C사", 100).orderId;

    approvalSvc->approve(o1);  // stock: 300 → 200
    approvalSvc->approve(o2);  // stock: 200 → 100
    approvalSvc->approve(o3);  // stock: 100 → 0

    EXPECT_EQ(repo->samples()[0].stock, 0);
    EXPECT_EQ(monitorSvc->orderSummary().confirmed, 3);
}

// ── 저장/재로드 후 서비스 일관성 ─────────────────────────────────────────────

TEST_F(Phase8Test, 저장_재로드_후_서비스_상태_일치) {
    sampleSvc->add("저장시료", 0.5, 0.92, 100);
    const std::string sid = repo->samples()[0].id;
    const std::string oid = orderSvc->create(sid, "테스트고객", 60).orderId;
    approvalSvc->approve(oid);
    repo->save();

    // 재로드
    JsonRepository    repo2(testDbDir);
    repo2.load();
    ReleaseService    relSvc2(repo2);
    MonitorService    monSvc2(repo2);

    EXPECT_EQ(repo2.orders()[0].status,   OrderStatus::CONFIRMED);
    EXPECT_EQ(repo2.samples()[0].stock,   40);  // 100 - 60
    EXPECT_EQ(relSvc2.confirmedOrders().size(), 1u);
    EXPECT_EQ(monSvc2.orderSummary().confirmed, 1);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
