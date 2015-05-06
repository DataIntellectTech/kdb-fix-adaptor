#include <iostream>

#include "gtest/gtest.h"

#include "aquaq/FixEngine.h"
#include "aquaq/FixSession.h"

TEST(ExampleTestSuite, ValuesAreEqual)
{
    EXPECT_EQ(1, 1);
}

TEST(ExampleTestSuite, ValuesAreNotEqual)
{
    EXPECT_EQ(1, 2);
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv); 
    return RUN_ALL_TESTS();
}
