# 프로젝트 규칙
- 언어: C++
- 테스트 프레임워크: Google Test
- 각 Phase 구현 전 반드시 설계 문서 먼저 작성할 것
- 코드 생성 후 바로 커밋하지 말 것 (인간 검토 후 커밋)
- PLAN.md는 Repository에 포함하지 않는다
- 구현 완료 후 반드시 Verify(Test + Compliance) 수행할 것

# 주안점
- CLAUDE.md, PRD.md는 프로젝트에 반드시 포함되어야 함
- Harness 도입
- Test 반드시 실행
- Clean Code
- Commit 이력 많이 남기기

# Harness 규칙
- 디렉터리 구조: src/(비즈니스 로직), tests/(테스트 코드) 분리 유지
- 모든 테스트는 Google Test 기반 Fixture(TEST_F) 로 작성할 것
  - SetUp() / TearDown() 으로 테스트 간 상태 격리 보장
- build.bat 은 빌드 후 테스트 자동 실행까지 포함할 것
  - 테스트 미통과 시 빌드 성공으로 간주하지 않음
- 기능 구현 단위마다 대응하는 테스트를 함께 작성할 것 (테스트 없는 기능 커밋 금지)
- 테스트 이름은 의도를 표현할 것 (예: 주문_생성_성공, 재고_부족_시_주문_실패)
