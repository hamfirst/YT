module;

#include <gtest/gtest.h>

export module YT:ShelfAllocator2DTests;

import glm;

import :ShelfAllocator2D;

namespace YT::Tests
{
    TEST(ShelfAllocator2D, GetWidthGetHeight)
    {
        ShelfAllocator2D alloc(64, 32);
        EXPECT_EQ(alloc.GetWidth(), 64u);
        EXPECT_EQ(alloc.GetHeight(), 32u);
    }

    TEST(ShelfAllocator2D, BasicAllocation)
    {
        ShelfAllocator2D alloc(256, 256);
        auto pos = alloc.Allocate(16, 16);
        ASSERT_TRUE(pos.has_value());
        EXPECT_FLOAT_EQ(pos->x, 0.f);
        EXPECT_FLOAT_EQ(pos->y, 0.f);
    }

    TEST(ShelfAllocator2D, SequentialOnSameShelf)
    {
        ShelfAllocator2D alloc(100, 50);
        auto a = alloc.Allocate(30, 10);
        auto b = alloc.Allocate(30, 10);
        auto c = alloc.Allocate(40, 10);
        ASSERT_TRUE(a.has_value());
        ASSERT_TRUE(b.has_value());
        ASSERT_TRUE(c.has_value());
        EXPECT_FLOAT_EQ(a->x, 0.f);
        EXPECT_FLOAT_EQ(a->y, 0.f);
        EXPECT_FLOAT_EQ(b->x, 30.f);
        EXPECT_FLOAT_EQ(b->y, 0.f);
        EXPECT_FLOAT_EQ(c->x, 60.f);
        EXPECT_FLOAT_EQ(c->y, 0.f);
    }

    TEST(ShelfAllocator2D, AdvancesToNewShelfWhenRowFull)
    {
        ShelfAllocator2D alloc(50, 20);
        auto a = alloc.Allocate(30, 5);
        auto b = alloc.Allocate(25, 4);
        ASSERT_TRUE(a.has_value());
        ASSERT_TRUE(b.has_value());
        EXPECT_FLOAT_EQ(a->x, 0.f);
        EXPECT_FLOAT_EQ(a->y, 0.f);
        EXPECT_FLOAT_EQ(b->x, 0.f);
        EXPECT_FLOAT_EQ(b->y, 5.f);
    }

    TEST(ShelfAllocator2D, ExactFitSingleBlock)
    {
        ShelfAllocator2D alloc(10, 10);
        auto pos = alloc.Allocate(10, 10);
        ASSERT_TRUE(pos.has_value());
        EXPECT_FLOAT_EQ(pos->x, 0.f);
        EXPECT_FLOAT_EQ(pos->y, 0.f);
    }

    TEST(ShelfAllocator2D, RejectsWhenTooTall)
    {
        ShelfAllocator2D alloc(100, 5);
        auto pos = alloc.Allocate(1, 6);
        EXPECT_FALSE(pos.has_value());
    }

    TEST(ShelfAllocator2D, RejectsWhenWiderThanAtlas)
    {
        ShelfAllocator2D alloc(8, 100);
        auto pos = alloc.Allocate(9, 1);
        EXPECT_FALSE(pos.has_value());
    }

    TEST(ShelfAllocator2D, RejectsWhenNoVerticalRoomForNewShelf)
    {
        ShelfAllocator2D alloc(10, 10);
        ASSERT_TRUE(alloc.Allocate(10, 10).has_value());
        auto second = alloc.Allocate(1, 1);
        EXPECT_FALSE(second.has_value());
    }
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
