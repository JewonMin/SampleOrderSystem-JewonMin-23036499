# SampleOrderSystem

반도체 시료 생산주문 관리 시스템 — Windows 콘솔 기반 C++17 MVC 애플리케이션.

## 요구 환경

- Windows 10/11
- Visual Studio 2022 (C++ 빌드 도구 포함)

## 빌드

프로젝트 루트에서 실행:

```bat
build.bat
```

**빌드만 실행** (기본값 — 빠른 개발 사이클):
```bat
build.bat
```

**빌드 + 전체 테스트 실행**:
```bat
build.bat --test
```

| 명령 | 동작 |
|---|---|
| `build.bat` | 앱 빌드만 수행 |
| `build.bat --test` | 빌드 후 Phase 0~8 테스트 전체 실행 |

- 첫 실행 시 `nlohmann/json` 헤더와 Google Test 라이브러리를 자동으로 다운로드합니다.
- Google Test 다운로드는 `--test` 옵션을 사용할 때만 수행됩니다.

## 실행

```bat
SampleOrderSystem.exe
```

## 프로젝트 구조

```
src/
  model/          도메인 모델
  repository/     JSON 영속성 (JsonRepository)
  service/        비즈니스 로직
  view/           콘솔 UI (MVC View)
tests/            Google Test 기반 단위·통합 테스트 (Phase 0~8)
db/               런타임 데이터 (JSON)
docs/             기능 설계 문서
```

## 주요 기능

| 메뉴 | 설명 |
|---|---|
| [1] 시료 관리 | 시료 등록·조회·검색 |
| [2] 시료 주문 | 주문 생성 (RESERVED 상태) |
| [3] 주문 승인/거절 | RESERVED → CONFIRMED / REJECTED |
| [4] 모니터링 | 주문 현황 및 재고 상태 요약 |
| [5] 생산라인 조회 | FIFO 생산 큐 라이브 현황, 자동 완료 처리 |
| [6] 출고 처리 | CONFIRMED → RELEASED |
