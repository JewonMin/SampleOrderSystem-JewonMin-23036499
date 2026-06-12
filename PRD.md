# PRD — 반도체 시료 생산주문관리 시스템 (SampleOrderSystem)

## 문서 구조

상세 명세는 각 링크 문서를 참고한다.

| 문서 | 내용 |
|------|------|
| [시스템 개요](docs/system-overview.md) | 배경, 등장 역할, 생산 라인, 주문 상태 흐름도 |
| [주문 상태 흐름](docs/order-status.md) | 상태 정의(RESERVED/REJECTED/PRODUCING/CONFIRMED/RELEASED), 전이 조건 |
| [메인 메뉴](docs/features/main-menu.md) | 메뉴 구성(1~6번), 시스템 현황 요약, UI 화면 예시 |
| [시료 관리](docs/features/sample-management.md) | 시료 등록, 목록 조회, 이름 검색 |
| [시료 주문](docs/features/order-placement.md) | 주문 접수 |
| [주문 승인/거절](docs/features/order-approval.md) | 승인/거절 처리 |
| [모니터링](docs/features/monitoring.md) | 상태별 주문 수, 재고 현황 |
| [생산라인](docs/features/production-line.md) | 생산라인 조회 |
| [출고 처리](docs/features/release.md) | 출고 처리 |

> 이하 항목은 assignment.pdf 검토 후 순차적으로 내용 채울 예정

---

## 프로젝트 요약

- **시스템명**: 반도체 시료 생산주문관리 시스템
- **대상 회사**: S-Semi (가상)
- **플랫폼**: 콘솔 기반 (C++)
- **핵심 목표**: 주문 상태 추적, 재고 현황 파악, 공정 중복 방지
