# 주문 상태 흐름

## 개요

모든 주문은 아래 정의된 상태를 보유한다.
`REJECTED`는 정상 흐름 외의 상태로, 모니터링 대상에서 제외한다.

---

## 상태 정의

| 상태 | 의미 |
|------|------|
| `RESERVED` | 주문 접수 |
| `REJECTED` | 주문 거절 (비정상 흐름, 모니터링 제외) |
| `PRODUCING` | 주문 승인 완료 및 재고 부족으로 생산 중 |
| `CONFIRMED` | 주문 승인 완료 및 출고 대기 중 |
| `RELEASED` | 출고 완료 |

---

## 상태 전이

```
RESERVED ──▶ REJECTED       (생산 담당자 거절)
RESERVED ──▶ CONFIRMED      (승인 + 재고 충분)
RESERVED ──▶ PRODUCING      (승인 + 재고 부족)
PRODUCING ──▶ CONFIRMED      (생산 완료)
CONFIRMED ──▶ RELEASED       (출고 처리)
```

### 전이 조건 요약

| From | To | 조건 |
|------|----|------|
| `RESERVED` | `REJECTED` | 생산 담당자가 거절 |
| `RESERVED` | `CONFIRMED` | 생산 담당자 승인 + 재고 충분 |
| `RESERVED` | `PRODUCING` | 생산 담당자 승인 + 재고 부족 |
| `PRODUCING` | `CONFIRMED` | 생산 완료 |
| `CONFIRMED` | `RELEASED` | 출고 처리 |
