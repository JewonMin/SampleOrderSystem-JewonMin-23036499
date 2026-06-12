// ============================================================
// Final Verification Test Suite
// PRD 전체 기능 + 사이드/엣지 케이스 + 버그 회귀 테스트
// ============================================================
#include <gtest/gtest.h>
#include <filesystem>
#include <memory>
#include <ctime>
#include <string>
#include <algorithm>
#include <cstdio>

#include "repository/JsonRepository.h"
#include "service/SampleService.h"
#include "service/OrderService.h"
#include "service/ApprovalService.h"
#include "service/ProductionService.h"
#include "service/ReleaseService.h"
#include "service/MonitorService.h"

namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────────────────────────
// 공통 픽스처
// ─────────────────────────────────────────────────────────────────────────────
class FV : public ::testing::Test {
protected:
    const std::string db = "test_db_fv";

    std::unique_ptr<JsonRepository>    repo;
    std::unique_ptr<SampleService>     sampleSvc;
    std::unique_ptr<OrderService>      orderSvc;
    std::unique_ptr<ApprovalService>   approvalSvc;
    std::unique_ptr<ProductionService> productionSvc;
    std::unique_ptr<ReleaseService>    releaseSvc;
    std::unique_ptr<MonitorService>    monitorSvc;

    void SetUp() override {
        fs::remove_all(db);
        repo          = std::make_unique<JsonRepository>(db);
        sampleSvc     = std::make_unique<SampleService>(*repo);
        orderSvc      = std::make_unique<OrderService>(*repo);
        approvalSvc   = std::make_unique<ApprovalService>(*repo);
        productionSvc = std::make_unique<ProductionService>(*repo);
        releaseSvc    = std::make_unique<ReleaseService>(*repo);
        monitorSvc    = std::make_unique<MonitorService>(*repo);
    }

    void TearDown() override { fs::remove_all(db); }

    // 시료 직접 삽입 (서비스 우회, 엣지케이스 구성용)
    void rawSample(const std::string& id, const std::string& name,
                   double avgTime, double yield, int stock) {
        repo->samples().push_back({id, name, avgTime, yield, stock});
    }

    // 주문 직접 삽입
    void rawOrder(const std::string& orderId, const std::string& sampleId,
                  const std::string& customer, int qty, OrderStatus st) {
        repo->orders().push_back({orderId, sampleId, customer, qty, st, "2026-06-12 10:00:00"});
    }

    // 생산 큐 직접 삽입
    void rawQueue(const std::string& orderId, const std::string& sampleId,
                  int shortage, int actual, double totalMin,
                  long long startEpoch = 0) {
        if (startEpoch == 0) startEpoch = static_cast<long long>(std::time(nullptr));
        repo->productionQueue().push_back(
            {orderId, sampleId, shortage, actual, totalMin, startEpoch, "2026-06-12 10:00:00"});
    }

    // 오늘 날짜 접두사 반환 (ORD-YYYYMMDD-)
    std::string todayPrefix() const {
        std::time_t now = std::time(nullptr);
        struct tm t{};
        localtime_s(&t, &now);
        char buf[32];
        std::strftime(buf, sizeof(buf), "ORD-%Y%m%d-", &t);
        return buf;
    }
};

// =============================================================================
// [1] 시료 관리 (SampleService)
// =============================================================================

TEST_F(FV, Sample_등록_정상_ID_자동생성) {
    auto s = sampleSvc->add("실리콘 웨이퍼-8인치", 0.5, 0.92, 480);
    EXPECT_EQ(s.id, "S-001");
    EXPECT_EQ(s.name, "실리콘 웨이퍼-8인치");
    EXPECT_NEAR(s.avgProductionTime, 0.5, 0.001);
    EXPECT_NEAR(s.yield, 0.92, 0.001);
    EXPECT_EQ(s.stock, 480);
}

TEST_F(FV, Sample_다중등록_ID_순차증가) {
    auto s1 = sampleSvc->add("시료A", 0.5, 0.9, 100);
    auto s2 = sampleSvc->add("시료B", 0.5, 0.9, 100);
    auto s3 = sampleSvc->add("시료C", 0.5, 0.9, 100);
    EXPECT_EQ(s1.id, "S-001");
    EXPECT_EQ(s2.id, "S-002");
    EXPECT_EQ(s3.id, "S-003");
}

TEST_F(FV, Sample_기존ID있을때_ID_최대값기반_증가) {
    // S-010이 이미 있으면 다음은 S-011
    rawSample("S-010", "기존시료", 0.5, 0.9, 0);
    auto s = sampleSvc->add("신규시료", 0.5, 0.9, 0);
    EXPECT_EQ(s.id, "S-011");
}

TEST_F(FV, Sample_이름검색_키워드_일치) {
    sampleSvc->add("실리콘 웨이퍼-8인치", 0.5, 0.92, 100);
    sampleSvc->add("GaN 에피택셀-4인치", 0.3, 0.78, 50);
    sampleSvc->add("SiC 파워기판-6인치", 0.8, 0.92, 30);

    auto results = sampleSvc->searchByName("웨이퍼");
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0]->name, "실리콘 웨이퍼-8인치");
}

TEST_F(FV, Sample_이름검색_여러개_일치) {
    sampleSvc->add("산화막 웨이퍼-SiO2", 0.6, 0.88, 0);
    sampleSvc->add("실리콘 웨이퍼-8인치", 0.5, 0.92, 100);
    sampleSvc->add("SiC 파워기판-6인치", 0.8, 0.92, 30);

    auto results = sampleSvc->searchByName("웨이퍼");
    EXPECT_EQ(results.size(), 2u);
}

TEST_F(FV, Sample_이름검색_없으면_빈_목록) {
    sampleSvc->add("SiC 파워기판-6인치", 0.8, 0.92, 30);
    EXPECT_TRUE(sampleSvc->searchByName("존재하지않음").empty());
}

TEST_F(FV, Sample_findById_정상) {
    sampleSvc->add("실리콘 웨이퍼-8인치", 0.5, 0.92, 100);
    auto* s = sampleSvc->findById("S-001");
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->name, "실리콘 웨이퍼-8인치");
}

TEST_F(FV, Sample_findById_없으면_nullptr) {
    EXPECT_EQ(sampleSvc->findById("S-999"), nullptr);
}

TEST_F(FV, Sample_빈이름_예외) {
    EXPECT_THROW(sampleSvc->add("", 0.5, 0.9, 0), std::invalid_argument);
}

TEST_F(FV, Sample_수율_0_예외) {
    EXPECT_THROW(sampleSvc->add("시료", 0.5, 0.0, 0), std::invalid_argument);
}

TEST_F(FV, Sample_수율_1초과_예외) {
    EXPECT_THROW(sampleSvc->add("시료", 0.5, 1.1, 0), std::invalid_argument);
}

TEST_F(FV, Sample_평균생산시간_0_예외) {
    EXPECT_THROW(sampleSvc->add("시료", 0.0, 0.9, 0), std::invalid_argument);
}

TEST_F(FV, Sample_음수재고_예외) {
    EXPECT_THROW(sampleSvc->add("시료", 0.5, 0.9, -1), std::invalid_argument);
}

// =============================================================================
// [2] 시료 주문 생성 (OrderService)
// =============================================================================

TEST_F(FV, Order_생성_초기상태_RESERVED) {
    rawSample("S-001", "실리콘 웨이퍼", 0.5, 0.92, 100);
    auto o = orderSvc->create("S-001", "SK하이닉스", 50);
    EXPECT_EQ(o.status, OrderStatus::RESERVED);
    EXPECT_EQ(o.sampleId, "S-001");
    EXPECT_EQ(o.customerName, "SK하이닉스");
    EXPECT_EQ(o.quantity, 50);
}

TEST_F(FV, Order_주문번호_형식_ORD_YYYYMMDD_NNNN) {
    rawSample("S-001", "시료", 0.5, 0.9, 100);
    auto o = orderSvc->create("S-001", "고객", 10);
    std::string pfx = todayPrefix();
    EXPECT_EQ(o.orderId.substr(0, pfx.size()), pfx);
    EXPECT_EQ(o.orderId.size(), pfx.size() + 4);
}

TEST_F(FV, Order_연속생성_번호_순차증가) {
    rawSample("S-001", "시료", 0.5, 0.9, 100);
    auto o1 = orderSvc->create("S-001", "고객A", 10);
    auto o2 = orderSvc->create("S-001", "고객B", 10);
    auto o3 = orderSvc->create("S-001", "고객C", 10);
    EXPECT_LT(o1.orderId, o2.orderId);
    EXPECT_LT(o2.orderId, o3.orderId);
    EXPECT_NE(o1.orderId, o2.orderId);
    EXPECT_NE(o2.orderId, o3.orderId);
}

TEST_F(FV, Order_BUGFIX_DB중복ID있어도_신규주문_고유번호_생성) {
    // 실제 발생한 버그: 카운트 방식으로 ID를 생성하면 중복 ID가 있을 때
    // 이미 존재하는 ID를 재발급해서 두 주문이 같은 orderId를 갖게 됨
    std::string pfx = todayPrefix();
    rawSample("S-001", "시료", 0.5, 0.9, 100);

    // 0001 ~ 0003 중 0002가 두 번 (중복) 존재하는 상황
    rawOrder(pfx + "0001", "S-001", "고객A", 10, OrderStatus::RESERVED);
    rawOrder(pfx + "0002", "S-001", "고객B", 10, OrderStatus::REJECTED);
    rawOrder(pfx + "0002", "S-001", "고객C", 10, OrderStatus::RESERVED); // 중복!
    rawOrder(pfx + "0003", "S-001", "고객D", 10, OrderStatus::RESERVED);

    // 카운트 방식: 4개 있으므로 0005 → 이미 없어서 OK
    // 최대값 방식: max=3 이므로 0004 → 중복 없이 정확
    auto newOrder = orderSvc->create("S-001", "신규고객", 5);

    // 반드시 기존 ID와 중복이 없어야 함
    auto& orders = repo->orders();
    int dupCount = 0;
    for (size_t i = 0; i < orders.size(); ++i)
        for (size_t j = i + 1; j < orders.size(); ++j)
            if (orders[i].orderId == orders[j].orderId) ++dupCount;
    EXPECT_EQ(dupCount, 1); // 기존 의도적 중복(0002×2) 만 남고 신규는 충돌 없음

    // 신규 주문의 ID가 목록에 한 번만 등장
    int newIdCount = 0;
    for (const auto& o : orders)
        if (o.orderId == newOrder.orderId) ++newIdCount;
    EXPECT_EQ(newIdCount, 1);
}

TEST_F(FV, Order_미등록시료_예외) {
    EXPECT_THROW(orderSvc->create("S-999", "고객", 10), std::invalid_argument);
}

TEST_F(FV, Order_빈고객명_예외) {
    rawSample("S-001", "시료", 0.5, 0.9, 100);
    EXPECT_THROW(orderSvc->create("S-001", "", 10), std::invalid_argument);
}

TEST_F(FV, Order_수량0_예외) {
    rawSample("S-001", "시료", 0.5, 0.9, 100);
    EXPECT_THROW(orderSvc->create("S-001", "고객", 0), std::invalid_argument);
}

TEST_F(FV, Order_음수수량_예외) {
    rawSample("S-001", "시료", 0.5, 0.9, 100);
    EXPECT_THROW(orderSvc->create("S-001", "고객", -5), std::invalid_argument);
}

// =============================================================================
// [3] 주문 승인/거절 (ApprovalService)
// =============================================================================

TEST_F(FV, Approval_재고충분_CONFIRMED_전이_재고차감) {
    rawSample("S-001", "시료", 0.5, 0.92, 200);
    rawOrder("ORD-0001", "S-001", "고객", 100, OrderStatus::RESERVED);

    auto r = approvalSvc->approve("ORD-0001");
    EXPECT_EQ(r.decision, ApprovalService::ApprovalResult::Decision::CONFIRMED);
    EXPECT_EQ(r.shortage, 0);
    EXPECT_EQ(repo->orders()[0].status, OrderStatus::CONFIRMED);
    EXPECT_EQ(repo->samples()[0].stock, 100); // 200 - 100
    EXPECT_TRUE(repo->productionQueue().empty());
}

TEST_F(FV, Approval_재고부족_PRODUCING_전이_생산큐등록) {
    rawSample("S-001", "시료", 0.8, 0.92, 30);
    rawOrder("ORD-0001", "S-001", "고객", 200, OrderStatus::RESERVED);

    auto r = approvalSvc->approve("ORD-0001");
    EXPECT_EQ(r.decision, ApprovalService::ApprovalResult::Decision::PRODUCING);
    EXPECT_EQ(r.shortage, 170);
    EXPECT_EQ(r.actualProductQty, 206); // ceil(170/(0.92*0.9))
    EXPECT_NEAR(r.totalProductionTime, 164.8, 0.01);
    EXPECT_EQ(repo->orders()[0].status, OrderStatus::PRODUCING);
    ASSERT_EQ(repo->productionQueue().size(), 1u);
    EXPECT_EQ(repo->productionQueue()[0].orderId, "ORD-0001");
    EXPECT_GT(repo->productionQueue()[0].startedAtEpoch, 0LL);
}

TEST_F(FV, Approval_재고_정확히_같을때_CONFIRMED_경계값) {
    rawSample("S-001", "시료", 0.5, 0.9, 100);
    rawOrder("ORD-0001", "S-001", "고객", 100, OrderStatus::RESERVED);
    auto r = approvalSvc->approve("ORD-0001");
    EXPECT_EQ(r.decision, ApprovalService::ApprovalResult::Decision::CONFIRMED);
    EXPECT_EQ(repo->samples()[0].stock, 0);
}

TEST_F(FV, Approval_재고0에서_전량_PRODUCING) {
    rawSample("S-001", "시료", 0.5, 0.9, 0);
    rawOrder("ORD-0001", "S-001", "고객", 50, OrderStatus::RESERVED);
    auto r = approvalSvc->approve("ORD-0001");
    EXPECT_EQ(r.decision, ApprovalService::ApprovalResult::Decision::PRODUCING);
    EXPECT_EQ(r.shortage, 50);
}

TEST_F(FV, Approval_거절_REJECTED_재고불변) {
    rawSample("S-001", "시료", 0.5, 0.92, 100);
    rawOrder("ORD-0001", "S-001", "고객", 50, OrderStatus::RESERVED);
    approvalSvc->reject("ORD-0001");
    EXPECT_EQ(repo->orders()[0].status, OrderStatus::REJECTED);
    EXPECT_EQ(repo->samples()[0].stock, 100);
    EXPECT_TRUE(repo->productionQueue().empty());
}

TEST_F(FV, Approval_실생산량공식_예시1_부족170_수율0_92) {
    // ceil(170 / (0.92 * 0.9)) = ceil(205.33) = 206
    EXPECT_EQ(ApprovalService::calcActualProductQty(170, 0.92), 206);
}

TEST_F(FV, Approval_실생산량공식_예시2_부족150_수율0_88) {
    // ceil(150 / (0.88 * 0.9)) = ceil(189.39) = 190
    EXPECT_EQ(ApprovalService::calcActualProductQty(150, 0.88), 190);
}

TEST_F(FV, Approval_실생산량공식_부족1_수율1_0) {
    // ceil(1 / (1.0 * 0.9)) = ceil(1.11) = 2
    EXPECT_EQ(ApprovalService::calcActualProductQty(1, 1.0), 2);
}

TEST_F(FV, Approval_총생산시간_206ea_avgTime0_8) {
    // 206 * 0.8 = 164.8
    EXPECT_NEAR(ApprovalService::calcTotalProductionTime(206, 0.8), 164.8, 0.001);
}

TEST_F(FV, Approval_동일시료_연속3건_독립재고체크) {
    rawSample("S-001", "시료", 0.5, 0.9, 200);
    rawOrder("ORD-0001", "S-001", "고객A", 100, OrderStatus::RESERVED);
    rawOrder("ORD-0002", "S-001", "고객B", 100, OrderStatus::RESERVED);
    rawOrder("ORD-0003", "S-001", "고객C", 100, OrderStatus::RESERVED);

    auto r1 = approvalSvc->approve("ORD-0001"); // 재고 200→100 CONFIRMED
    auto r2 = approvalSvc->approve("ORD-0002"); // 재고 100→0  CONFIRMED
    auto r3 = approvalSvc->approve("ORD-0003"); // 재고 0     PRODUCING

    EXPECT_EQ(r1.decision, ApprovalService::ApprovalResult::Decision::CONFIRMED);
    EXPECT_EQ(r2.decision, ApprovalService::ApprovalResult::Decision::CONFIRMED);
    EXPECT_EQ(r3.decision, ApprovalService::ApprovalResult::Decision::PRODUCING);
    EXPECT_EQ(r3.shortage, 100);
    EXPECT_EQ(repo->samples()[0].stock, 0);
    EXPECT_EQ(repo->productionQueue().size(), 1u);
}

TEST_F(FV, Approval_effectiveStock_생산진행분_포함) {
    rawSample("S-001", "시료", 0.5, 0.92, 30);
    // 0% 진행 중인 생산 큐 (방금 시작)
    long long now = static_cast<long long>(std::time(nullptr));
    rawQueue("ORD-PROD", "S-001", 70, 100, 1000.0, now);

    // 진행률 ≈ 0%이므로 기여분 거의 0 → effectiveStock ≈ 30
    int eff = approvalSvc->effectiveStock("S-001");
    EXPECT_GE(eff, 30);
    EXPECT_LT(eff, 35); // 방금 시작했으니 1ea 미만 기여
}

TEST_F(FV, Approval_effectiveStock_생산완료직전_최대반영) {
    rawSample("S-001", "시료", 0.5, 0.92, 30);
    // 매우 오래 전에 시작 → 100% 진행
    long long past = static_cast<long long>(std::time(nullptr)) - 100000LL;
    rawQueue("ORD-PROD", "S-001", 70, 100, 1.0, past);

    // 100% 진행 → 100ea 기여 + 기존 30 = 130
    int eff = approvalSvc->effectiveStock("S-001");
    EXPECT_EQ(eff, 130);
}

TEST_F(FV, Approval_BUGFIX_REJECTED주문과_같은ID_RESERVED주문_승인_성공) {
    // 실제 발생한 버그: REJECTED 주문이 RESERVED 주문보다 먼저 저장되어 있을 때
    // findOrder가 REJECTED를 먼저 찾아 "RESERVED만 처리 가능" 오류 발생
    rawSample("S-001", "시료", 0.5, 0.92, 200);
    rawOrder("ORD-20260612-0011", "S-001", "인텔코리아", 500, OrderStatus::REJECTED); // 먼저 삽입
    rawOrder("ORD-20260612-0011", "S-001", "SK하이닉스", 50, OrderStatus::RESERVED);  // 나중 삽입

    // 수정 후: RESERVED 선호 → 정상 승인
    EXPECT_NO_THROW({
        auto r = approvalSvc->approve("ORD-20260612-0011");
        EXPECT_EQ(r.decision, ApprovalService::ApprovalResult::Decision::CONFIRMED);
    });

    // SK하이닉스 주문(RESERVED)만 CONFIRMED로 바뀌어야 함
    int confirmedCount = 0, rejectedCount = 0;
    for (const auto& o : repo->orders()) {
        if (o.status == OrderStatus::CONFIRMED) ++confirmedCount;
        if (o.status == OrderStatus::REJECTED)  ++rejectedCount;
    }
    EXPECT_EQ(confirmedCount, 1);
    EXPECT_EQ(rejectedCount,  1); // 인텔코리아는 여전히 REJECTED
}

TEST_F(FV, Approval_BUGFIX_중복ID_두RESERVED_순차승인) {
    // 같은 orderId를 가진 두 RESERVED 주문 — 수정 전에는 같은 주문을 두 번 승인
    rawSample("S-001", "시료", 0.5, 0.92, 500);
    rawOrder("ORD-0010", "S-001", "삼성전자 파운드리", 200, OrderStatus::RESERVED);
    rawOrder("ORD-0010", "S-001", "인텔", 1, OrderStatus::RESERVED);

    // 첫 번째 승인: 삼성전자 파운드리(RESERVED→CONFIRMED)
    EXPECT_NO_THROW(approvalSvc->approve("ORD-0010"));

    // 한 건은 CONFIRMED, 한 건은 아직 RESERVED
    int confirmed = 0, reserved = 0;
    for (const auto& o : repo->orders()) {
        if (o.status == OrderStatus::CONFIRMED) ++confirmed;
        if (o.status == OrderStatus::RESERVED)  ++reserved;
    }
    EXPECT_EQ(confirmed, 1);
    EXPECT_EQ(reserved, 1);

    // 두 번째 승인: 남은 RESERVED
    EXPECT_NO_THROW(approvalSvc->approve("ORD-0010"));
    int totalConfirmed = 0;
    for (const auto& o : repo->orders())
        if (o.status == OrderStatus::CONFIRMED) ++totalConfirmed;
    EXPECT_EQ(totalConfirmed, 2);
}

TEST_F(FV, Approval_없는주문_승인_예외) {
    EXPECT_THROW(approvalSvc->approve("ORD-XXXX"), std::invalid_argument);
}

TEST_F(FV, Approval_RESERVED아닌_주문_승인_예외) {
    rawSample("S-001", "시료", 0.5, 0.9, 100);
    rawOrder("ORD-0001", "S-001", "고객", 50, OrderStatus::CONFIRMED);
    EXPECT_THROW(approvalSvc->approve("ORD-0001"), std::logic_error);
}

TEST_F(FV, Approval_없는주문_거절_예외) {
    EXPECT_THROW(approvalSvc->reject("ORD-XXXX"), std::invalid_argument);
}

TEST_F(FV, Approval_RESERVED아닌_주문_거절_예외) {
    rawSample("S-001", "시료", 0.5, 0.9, 100);
    rawOrder("ORD-0001", "S-001", "고객", 50, OrderStatus::PRODUCING);
    EXPECT_THROW(approvalSvc->reject("ORD-0001"), std::logic_error);
}

TEST_F(FV, Approval_reservedOrders_RESERVED만_반환) {
    rawSample("S-001", "시료", 0.5, 0.9, 100);
    rawOrder("ORD-0001", "S-001", "A", 10, OrderStatus::RESERVED);
    rawOrder("ORD-0002", "S-001", "B", 10, OrderStatus::CONFIRMED);
    rawOrder("ORD-0003", "S-001", "C", 10, OrderStatus::REJECTED);
    rawOrder("ORD-0004", "S-001", "D", 10, OrderStatus::PRODUCING);
    rawOrder("ORD-0005", "S-001", "E", 10, OrderStatus::RESERVED);

    auto list = approvalSvc->reservedOrders();
    ASSERT_EQ(list.size(), 2u);
    EXPECT_EQ(list[0]->orderId, "ORD-0001");
    EXPECT_EQ(list[1]->orderId, "ORD-0005");
}

// =============================================================================
// [4] 생산라인 (ProductionService)
// =============================================================================

TEST_F(FV, Production_빈큐_list_빈목록) {
    EXPECT_TRUE(productionSvc->list().empty());
}

TEST_F(FV, Production_list_FIFO_순서_유지) {
    rawSample("S-001", "시료1", 0.5, 0.9, 0);
    rawSample("S-002", "시료2", 0.5, 0.9, 0);
    rawOrder("ORD-A", "S-001", "고객", 100, OrderStatus::PRODUCING);
    rawOrder("ORD-B", "S-002", "고객", 50,  OrderStatus::PRODUCING);
    rawQueue("ORD-A", "S-001", 100, 122, 61.0);
    rawQueue("ORD-B", "S-002", 50,  62,  31.0);

    auto items = productionSvc->list();
    ASSERT_EQ(items.size(), 2u);
    EXPECT_EQ(items[0].orderId, "ORD-A"); // 먼저 들어온 것이 첫 번째
    EXPECT_EQ(items[1].orderId, "ORD-B");
}

TEST_F(FV, Production_진행률_시작직후_0근접) {
    rawSample("S-001", "시료", 0.5, 0.9, 0);
    rawOrder("ORD-A", "S-001", "고객", 100, OrderStatus::PRODUCING);
    long long now = static_cast<long long>(std::time(nullptr));
    rawQueue("ORD-A", "S-001", 100, 122, 1000.0, now);

    auto items = productionSvc->list();
    ASSERT_EQ(items.size(), 1u);
    EXPECT_LT(items[0].progressPct, 1.0);
}

TEST_F(FV, Production_진행률_완료경과시_100_캡) {
    rawSample("S-001", "시료", 0.5, 0.9, 0);
    rawOrder("ORD-A", "S-001", "고객", 100, OrderStatus::PRODUCING);
    long long past = static_cast<long long>(std::time(nullptr)) - 100000LL;
    rawQueue("ORD-A", "S-001", 100, 122, 0.001, past);

    auto items = productionSvc->list();
    ASSERT_EQ(items.size(), 1u);
    EXPECT_NEAR(items[0].progressPct, 100.0, 0.001);
}

TEST_F(FV, Production_autoComplete_미완료_건너뜀) {
    rawSample("S-001", "시료", 0.5, 0.9, 0);
    rawOrder("ORD-A", "S-001", "고객", 100, OrderStatus::PRODUCING);
    long long now = static_cast<long long>(std::time(nullptr));
    rawQueue("ORD-A", "S-001", 100, 122, 1000.0, now); // 아직 한참 남음

    productionSvc->autoComplete(); // 아무것도 안 해야 함
    EXPECT_EQ(repo->orders()[0].status, OrderStatus::PRODUCING);
    EXPECT_EQ(repo->productionQueue().size(), 1u);
}

TEST_F(FV, Production_autoComplete_완료조건_CONFIRMED_전이) {
    rawSample("S-001", "시료", 0.5, 0.9, 30);
    rawOrder("ORD-A", "S-001", "고객", 200, OrderStatus::PRODUCING);
    long long past = static_cast<long long>(std::time(nullptr)) - 100000LL;
    rawQueue("ORD-A", "S-001", 170, 206, 1.0, past); // 오래 전 시작 → 완료

    productionSvc->autoComplete();
    EXPECT_EQ(repo->orders()[0].status, OrderStatus::CONFIRMED);
    EXPECT_TRUE(repo->productionQueue().empty());
}

TEST_F(FV, Production_autoComplete_연속여러건_순차완료) {
    rawSample("S-001", "시료1", 0.5, 0.9, 30);
    rawSample("S-002", "시료2", 0.5, 0.9, 0);
    rawOrder("ORD-A", "S-001", "고객", 100, OrderStatus::PRODUCING);
    rawOrder("ORD-B", "S-002", "고객", 50,  OrderStatus::PRODUCING);
    rawOrder("ORD-C", "S-001", "고객", 20,  OrderStatus::PRODUCING);
    long long past = static_cast<long long>(std::time(nullptr)) - 100000LL;
    rawQueue("ORD-A", "S-001", 70, 86,  1.0, past);
    rawQueue("ORD-B", "S-002", 50, 62,  1.0, past);
    rawQueue("ORD-C", "S-001", 0,  20,  1.0, past);

    productionSvc->autoComplete(); // 3건 모두 완료
    EXPECT_TRUE(repo->productionQueue().empty());
    for (const auto& o : repo->orders())
        EXPECT_EQ(o.status, OrderStatus::CONFIRMED);
}

TEST_F(FV, Production_autoComplete_재고_정확히_갱신) {
    // approve PRODUCING 시 실물재고 0으로 예약됨
    // stock=0, actual=206, shortage=170 → 0 + (206-170) = 36
    rawSample("S-001", "시료", 0.5, 0.9, 0);
    rawOrder("ORD-A", "S-001", "고객", 200, OrderStatus::PRODUCING);
    long long past = static_cast<long long>(std::time(nullptr)) - 100000LL;
    rawQueue("ORD-A", "S-001", 170, 206, 1.0, past);

    productionSvc->autoComplete();
    EXPECT_EQ(repo->samples()[0].stock, 36);
}

TEST_F(FV, Production_FIFO_두번째항목_직접완료_예외) {
    rawSample("S-001", "시료1", 0.5, 0.9, 0);
    rawSample("S-002", "시료2", 0.5, 0.9, 0);
    rawOrder("ORD-A", "S-001", "고객", 100, OrderStatus::PRODUCING);
    rawOrder("ORD-B", "S-002", "고객", 50,  OrderStatus::PRODUCING);
    rawQueue("ORD-A", "S-001", 100, 122, 96.8);
    rawQueue("ORD-B", "S-002", 50,  62,  48.8);

    EXPECT_THROW(productionSvc->complete("ORD-B"), std::logic_error); // FIFO 위반
    EXPECT_NO_THROW(productionSvc->complete("ORD-A"));                // 첫 번째는 가능
}

TEST_F(FV, Production_complete_큐제거_확인) {
    rawSample("S-001", "시료", 0.5, 0.9, 30);
    rawOrder("ORD-A", "S-001", "고객", 100, OrderStatus::PRODUCING);
    rawQueue("ORD-A", "S-001", 70, 86, 86.0);

    productionSvc->complete("ORD-A");
    EXPECT_TRUE(repo->productionQueue().empty());
}

// =============================================================================
// [5] 출고 처리 (ReleaseService)
// =============================================================================

TEST_F(FV, Release_CONFIRMED_RELEASED_전이) {
    rawSample("S-001", "시료", 0.5, 0.9, 100);
    rawOrder("ORD-0001", "S-001", "고객", 50, OrderStatus::CONFIRMED);

    releaseSvc->release("ORD-0001");
    EXPECT_EQ(repo->orders()[0].status, OrderStatus::RELEASED);
}

TEST_F(FV, Release_confirmedOrders_CONFIRMED만_반환) {
    rawSample("S-001", "시료", 0.5, 0.9, 100);
    rawOrder("ORD-0001", "S-001", "A", 10, OrderStatus::CONFIRMED);
    rawOrder("ORD-0002", "S-001", "B", 10, OrderStatus::RESERVED);
    rawOrder("ORD-0003", "S-001", "C", 10, OrderStatus::RELEASED);
    rawOrder("ORD-0004", "S-001", "D", 10, OrderStatus::CONFIRMED);

    auto list = releaseSvc->confirmedOrders();
    ASSERT_EQ(list.size(), 2u);
    EXPECT_EQ(list[0]->orderId, "ORD-0001");
    EXPECT_EQ(list[1]->orderId, "ORD-0004");
}

TEST_F(FV, Release_없는주문_예외) {
    EXPECT_THROW(releaseSvc->release("ORD-XXXX"), std::invalid_argument);
}

TEST_F(FV, Release_CONFIRMED아닌주문_예외) {
    rawSample("S-001", "시료", 0.5, 0.9, 100);
    rawOrder("ORD-0001", "S-001", "고객", 50, OrderStatus::RESERVED);
    EXPECT_THROW(releaseSvc->release("ORD-0001"), std::logic_error);
}

TEST_F(FV, Release_PRODUCING주문_출고_예외) {
    rawSample("S-001", "시료", 0.5, 0.9, 100);
    rawOrder("ORD-0001", "S-001", "고객", 50, OrderStatus::PRODUCING);
    EXPECT_THROW(releaseSvc->release("ORD-0001"), std::logic_error);
}

// =============================================================================
// [6] 모니터링 (MonitorService)
// =============================================================================

TEST_F(FV, Monitor_주문건수_REJECTED_제외_PRD명세) {
    rawSample("S-001", "시료", 0.5, 0.9, 100);
    rawOrder("ORD-0001", "S-001", "A", 10, OrderStatus::RESERVED);
    rawOrder("ORD-0002", "S-001", "B", 10, OrderStatus::CONFIRMED);
    rawOrder("ORD-0003", "S-001", "C", 10, OrderStatus::PRODUCING);
    rawOrder("ORD-0004", "S-001", "D", 10, OrderStatus::RELEASED);
    rawOrder("ORD-0005", "S-001", "E", 10, OrderStatus::REJECTED);
    rawOrder("ORD-0006", "S-001", "F", 10, OrderStatus::REJECTED); // REJECTED 2건

    auto s = monitorSvc->orderSummary();
    EXPECT_EQ(s.reserved,  1);
    EXPECT_EQ(s.confirmed, 1);
    EXPECT_EQ(s.producing, 1);
    EXPECT_EQ(s.released,  1);
    // REJECTED는 합산되지 않음 (별도 필드 없음)
}

TEST_F(FV, Monitor_재고_여유_상태) {
    rawSample("S-001", "시료", 0.5, 0.9, 200);
    rawOrder("ORD-0001", "S-001", "고객", 50, OrderStatus::RESERVED);

    auto stocks = monitorSvc->stockStatus();
    ASSERT_EQ(stocks.size(), 1u);
    EXPECT_EQ(stocks[0].status, MonitorService::StockStatus::SUFFICIENT);
}

TEST_F(FV, Monitor_재고_부족_상태) {
    rawSample("S-001", "시료", 0.5, 0.9, 30);
    rawOrder("ORD-0001", "S-001", "고객", 200, OrderStatus::RESERVED);

    auto stocks = monitorSvc->stockStatus();
    ASSERT_EQ(stocks.size(), 1u);
    EXPECT_EQ(stocks[0].status, MonitorService::StockStatus::SHORTAGE);
}

TEST_F(FV, Monitor_재고_고갈_상태) {
    rawSample("S-001", "시료", 0.5, 0.9, 0);
    rawOrder("ORD-0001", "S-001", "고객", 10, OrderStatus::RESERVED);

    auto stocks = monitorSvc->stockStatus();
    ASSERT_EQ(stocks.size(), 1u);
    EXPECT_EQ(stocks[0].status, MonitorService::StockStatus::DEPLETED);
}

TEST_F(FV, Monitor_재고_여유_RESERVED_PRODUCING_수요합산) {
    // RESERVED + PRODUCING 두 주문 수량이 재고보다 많으면 SHORTAGE
    rawSample("S-001", "시료", 0.5, 0.9, 100);
    rawOrder("ORD-0001", "S-001", "A", 60, OrderStatus::RESERVED);
    rawOrder("ORD-0002", "S-001", "B", 60, OrderStatus::PRODUCING); // 합계 120 > 100

    auto stocks = monitorSvc->stockStatus();
    ASSERT_EQ(stocks.size(), 1u);
    EXPECT_EQ(stocks[0].status, MonitorService::StockStatus::SHORTAGE);
}

TEST_F(FV, Monitor_CONFIRMED_RELEASED_수요_미포함) {
    // CONFIRMED, RELEASED 주문은 수요 계산에서 제외
    rawSample("S-001", "시료", 0.5, 0.9, 10);
    rawOrder("ORD-0001", "S-001", "A", 200, OrderStatus::CONFIRMED);
    rawOrder("ORD-0002", "S-001", "B", 200, OrderStatus::RELEASED);

    auto stocks = monitorSvc->stockStatus();
    ASSERT_EQ(stocks.size(), 1u);
    EXPECT_EQ(stocks[0].status, MonitorService::StockStatus::SUFFICIENT); // 수요 0, 재고 10
}

// =============================================================================
// [7] 영속성 (Persistence)
// =============================================================================

TEST_F(FV, Persist_저장후_재로드_주문상태보존) {
    rawSample("S-001", "시료", 0.5, 0.92, 30);
    rawOrder("ORD-0001", "S-001", "고객", 200, OrderStatus::RESERVED);
    approvalSvc->approve("ORD-0001");
    repo->save();

    JsonRepository repo2(db);
    repo2.load();
    ASSERT_EQ(repo2.orders().size(), 1u);
    EXPECT_EQ(repo2.orders()[0].status, OrderStatus::PRODUCING);
}

TEST_F(FV, Persist_생산큐_재로드_일치) {
    rawSample("S-001", "시료", 0.5, 0.92, 30);
    rawOrder("ORD-0001", "S-001", "고객", 200, OrderStatus::RESERVED);
    approvalSvc->approve("ORD-0001");
    repo->save();

    JsonRepository repo2(db);
    repo2.load();
    ASSERT_EQ(repo2.productionQueue().size(), 1u);
    EXPECT_EQ(repo2.productionQueue()[0].orderId, "ORD-0001");
    EXPECT_EQ(repo2.productionQueue()[0].shortageQty, 170);
    EXPECT_EQ(repo2.productionQueue()[0].actualProductQty, 206);
}

TEST_F(FV, Persist_시료재고_재로드_일치) {
    rawSample("S-001", "시료", 0.5, 0.92, 100);
    rawOrder("ORD-0001", "S-001", "고객", 60, OrderStatus::RESERVED);
    approvalSvc->approve("ORD-0001"); // 재고 100 - 60 = 40
    repo->save();

    JsonRepository repo2(db);
    repo2.load();
    ASSERT_EQ(repo2.samples().size(), 1u);
    EXPECT_EQ(repo2.samples()[0].stock, 40);
}

TEST_F(FV, Persist_출고후_재로드_RELEASED상태) {
    rawSample("S-001", "시료", 0.5, 0.9, 100);
    rawOrder("ORD-0001", "S-001", "고객", 50, OrderStatus::CONFIRMED);
    releaseSvc->release("ORD-0001");
    repo->save();

    JsonRepository repo2(db);
    repo2.load();
    ASSERT_EQ(repo2.orders().size(), 1u);
    EXPECT_EQ(repo2.orders()[0].status, OrderStatus::RELEASED);
}

// =============================================================================
// [8] 통합 테스트 — 전체 흐름
// =============================================================================

TEST_F(FV, Integration_전체흐름_충분재고_RESERVED_CONFIRMED_RELEASED) {
    // 시료 등록 → 주문 → 승인(재고충분) → 출고
    auto s = sampleSvc->add("실리콘 웨이퍼-8인치", 0.5, 0.92, 200);
    auto o = orderSvc->create(s.id, "SK하이닉스", 100);

    EXPECT_EQ(o.status, OrderStatus::RESERVED);

    auto r = approvalSvc->approve(o.orderId);
    EXPECT_EQ(r.decision, ApprovalService::ApprovalResult::Decision::CONFIRMED);
    EXPECT_EQ(repo->orders()[0].status, OrderStatus::CONFIRMED);

    releaseSvc->release(o.orderId);
    EXPECT_EQ(repo->orders()[0].status, OrderStatus::RELEASED);
    EXPECT_EQ(repo->samples()[0].stock, 100); // 200 - 100
}

TEST_F(FV, Integration_전체흐름_재고부족_PRODUCING_autoComplete_RELEASED) {
    // 시료 등록 → 주문 → 승인(재고부족→PRODUCING) → autoComplete → 출고
    auto s = sampleSvc->add("SiC 파워기판-6인치", 0.8, 0.92, 30);
    auto o = orderSvc->create(s.id, "삼성전자 파운드리", 200);

    auto r = approvalSvc->approve(o.orderId);
    EXPECT_EQ(r.decision, ApprovalService::ApprovalResult::Decision::PRODUCING);
    ASSERT_EQ(repo->productionQueue().size(), 1u);

    // 생산 완료 시뮬레이션: startedAtEpoch을 과거로 조작
    repo->productionQueue()[0].startedAtEpoch =
        static_cast<long long>(std::time(nullptr)) - 100000LL;
    repo->productionQueue()[0].totalProductionTime = 1.0;

    productionSvc->autoComplete();
    EXPECT_EQ(repo->orders()[0].status, OrderStatus::CONFIRMED);
    EXPECT_TRUE(repo->productionQueue().empty());

    releaseSvc->release(o.orderId);
    EXPECT_EQ(repo->orders()[0].status, OrderStatus::RELEASED);
}

TEST_F(FV, Integration_BUGSCENARIO_REJECTED중복ID후_신규RESERVED_승인_성공) {
    // 이전에 발생했던 실제 버그 시나리오 재현
    // 1. DB에 ORD-XXXX REJECTED 존재
    // 2. ID 카운트 오류로 신규 주문이 같은 ORD-XXXX 발급
    // 3. 승인 시 findOrder가 REJECTED를 먼저 찾아 오류 발생 → 수정 후 정상 작동
    rawSample("S-001", "시료", 0.5, 0.92, 500);

    // 기존 REJECTED (예: 이전 세션에서 거절된 주문)
    std::string pfx = todayPrefix();
    rawOrder(pfx + "0011", "S-001", "인텔코리아", 500, OrderStatus::REJECTED);

    // 중복 카운트 상황: 0001~0010 + 중복 0010 = 11개 → generateOrderId 카운트 오류시 0011 발급
    for (int i = 1; i <= 10; ++i) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%04d", i);
        rawOrder(pfx + buf, "S-001", "고객" + std::to_string(i), 10, OrderStatus::CONFIRMED);
    }
    rawOrder(pfx + "0010", "S-001", "중복고객", 10, OrderStatus::CONFIRMED); // 0010 중복

    // 이 상태에서 새 주문 생성 → max방식: max=11 → 0012 발급 (안전)
    auto newOrder = orderSvc->create("S-001", "SK하이닉스", 50);
    EXPECT_NE(newOrder.orderId, pfx + "0011"); // 기존 REJECTED와 충돌 없음

    // 승인 시 아무 오류 없이 처리되어야 함
    EXPECT_NO_THROW(approvalSvc->approve(newOrder.orderId));
}

TEST_F(FV, Integration_여러주문_혼재_상태전이_독립성) {
    // 다른 시료로 여러 주문이 섞여 있어도 각 주문이 독립적으로 상태 전이
    rawSample("S-001", "시료A", 0.5, 0.92, 500);
    rawSample("S-002", "시료B", 0.5, 0.88, 0);

    auto o1 = orderSvc->create("S-001", "고객A", 100); // S-001 재고 충분
    auto o2 = orderSvc->create("S-002", "고객B", 100); // S-002 재고 없음
    auto o3 = orderSvc->create("S-001", "고객C", 100); // S-001 재고 충분

    auto r1 = approvalSvc->approve(o1.orderId);
    auto r2 = approvalSvc->approve(o2.orderId);
    auto r3 = approvalSvc->approve(o3.orderId);

    EXPECT_EQ(r1.decision, ApprovalService::ApprovalResult::Decision::CONFIRMED);
    EXPECT_EQ(r2.decision, ApprovalService::ApprovalResult::Decision::PRODUCING);
    EXPECT_EQ(r3.decision, ApprovalService::ApprovalResult::Decision::CONFIRMED);

    // o1, o3 출고
    releaseSvc->release(o1.orderId);
    EXPECT_EQ(repo->orders()[0].status, OrderStatus::RELEASED);
    EXPECT_EQ(repo->orders()[1].status, OrderStatus::PRODUCING); // o2 영향 없음
    EXPECT_EQ(repo->orders()[2].status, OrderStatus::CONFIRMED); // o3 아직 출고 전
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
