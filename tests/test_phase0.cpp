#include <gtest/gtest.h>

// Phase 0: 프로젝트 뼈대 검증
// Google Test 연동 및 빌드 파이프라인 확인용
// 비즈니스 로직 테스트는 Phase 1부터 추가됨

TEST(Phase0Test, GoogleTestIntegration) {
    EXPECT_TRUE(true);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
