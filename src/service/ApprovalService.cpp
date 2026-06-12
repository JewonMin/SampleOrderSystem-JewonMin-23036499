#include "ApprovalService.h"
#include <ctime>
#include <cmath>
#include <algorithm>
#include <cstdio>

ApprovalService::ApprovalService(JsonRepository& repo) : m_repo(repo) {}

// ── 조회 ────────────────────────────────────────────────────────────────────

std::vector<const Order*> ApprovalService::reservedOrders() const {
    std::vector<const Order*> result;
    for (const auto& o : m_repo.orders())
        if (o.status == OrderStatus::RESERVED) result.push_back(&o);
    return result;
}

int ApprovalService::effectiveStock(const std::string& sampleId) const {
    int stock = 0;
    for (const auto& s : m_repo.samples())
        if (s.id == sampleId) { stock = s.stock; break; }

    // 진행 중인 생산 기여분: 경과시간 / 총생산시간 비율만큼 생산된 것으로 간주
    long long now = static_cast<long long>(std::time(nullptr));
    for (const auto& e : m_repo.productionQueue()) {
        if (e.sampleId != sampleId) continue;
        if (e.totalProductionTime <= 0) continue;
        long long elapsed = now - e.startedAtEpoch;
        double elapsedMin = elapsed / 60.0;
        double progress = std::min(1.0, elapsedMin / e.totalProductionTime);
        stock += static_cast<int>(e.actualProductQty * progress);
    }
    return stock;
}

// ── 정적 공식 ────────────────────────────────────────────────────────────────

int ApprovalService::calcActualProductQty(int shortage, double yield) {
    if (yield <= 0 || yield > 1)
        throw std::invalid_argument("수율은 0 초과 1 이하여야 합니다.");
    return static_cast<int>(std::ceil(static_cast<double>(shortage) / (yield * 0.9)));
}

double ApprovalService::calcTotalProductionTime(int actualQty, double avgProductionTime) {
    return avgProductionTime * actualQty;
}

// ── 승인 ─────────────────────────────────────────────────────────────────────

ApprovalService::ApprovalResult ApprovalService::approve(const std::string& orderId) {
    Order* order = findOrder(orderId);
    if (!order)
        throw std::invalid_argument("주문을 찾을 수 없습니다: " + orderId);
    if (order->status != OrderStatus::RESERVED)
        throw std::logic_error("RESERVED 상태의 주문만 승인할 수 있습니다.");

    Sample* sample = findSample(order->sampleId);
    if (!sample)
        throw std::logic_error("시료를 찾을 수 없습니다: " + order->sampleId);

    int effStock = effectiveStock(order->sampleId);
    ApprovalResult result;
    result.stockAtApproval = effStock;

    if (effStock >= order->quantity) {
        // 재고 충분 → CONFIRMED, 재고 차감
        result.decision            = ApprovalResult::Decision::CONFIRMED;
        result.shortage            = 0;
        result.actualProductQty    = 0;
        result.totalProductionTime = 0.0;

        order->status  = OrderStatus::CONFIRMED;
        sample->stock  = std::max(0, sample->stock - order->quantity);
    } else {
        // 재고 부족 → PRODUCING, 생산 큐 등록
        int    shortage   = order->quantity - effStock;
        int    actualQty  = calcActualProductQty(shortage, sample->yield);
        double totalTime  = calcTotalProductionTime(actualQty, sample->avgProductionTime);

        result.decision            = ApprovalResult::Decision::PRODUCING;
        result.shortage            = shortage;
        result.actualProductQty    = actualQty;
        result.totalProductionTime = totalTime;

        order->status = OrderStatus::PRODUCING;
        sample->stock = 0; // 기존 실물 재고를 이 주문에 예약 (완료 시 shortage 기반 정산)

        std::time_t now = std::time(nullptr);
        struct tm t{};
        localtime_s(&t, &now);
        char timeBuf[32];
        std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &t);

        ProductionEntry entry;
        entry.orderId             = orderId;
        entry.sampleId            = order->sampleId;
        entry.shortageQty         = shortage;
        entry.actualProductQty    = actualQty;
        entry.totalProductionTime = totalTime;
        entry.startedAtEpoch      = static_cast<long long>(now);
        entry.startedAt           = timeBuf;
        m_repo.productionQueue().push_back(entry);
    }

    return result;
}

// ── 거절 ─────────────────────────────────────────────────────────────────────

void ApprovalService::reject(const std::string& orderId) {
    Order* order = findOrder(orderId);
    if (!order)
        throw std::invalid_argument("주문을 찾을 수 없습니다: " + orderId);
    if (order->status != OrderStatus::RESERVED)
        throw std::logic_error("RESERVED 상태의 주문만 거절할 수 있습니다.");

    order->status = OrderStatus::REJECTED;
}

// ── 내부 헬퍼 ────────────────────────────────────────────────────────────────

Order* ApprovalService::findOrder(const std::string& orderId) {
    for (auto& o : m_repo.orders())
        if (o.orderId == orderId && o.status == OrderStatus::RESERVED) return &o;
    for (auto& o : m_repo.orders())
        if (o.orderId == orderId) return &o;
    return nullptr;
}

Sample* ApprovalService::findSample(const std::string& sampleId) {
    for (auto& s : m_repo.samples())
        if (s.id == sampleId) return &s;
    return nullptr;
}
