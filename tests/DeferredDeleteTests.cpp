module;

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <memory>

module YT:DeferredDeleteTests;

import :DeferredDelete;

namespace YT
{
    namespace
    {
        // Test fixture for DeferredDelete tests
        class DeferredDeleteTest : public ::testing::Test
        {
        protected:
            void SetUp() override {}
            void TearDown() override {}
        };

        // Simple test class that tracks construction/destruction
        class TestObject
        {
        public:
            TestObject() : m_Value(0) { ++s_ConstructorCount; }
            TestObject(int value) : m_Value(value) { ++s_ConstructorCount; }
            TestObject(const TestObject&) = delete;
            TestObject(TestObject&& other) noexcept : m_Value(other.m_Value) 
            { 
                ++s_ConstructorCount;
                other.m_Value = -1;
            }
            ~TestObject() { ++s_DestructorCount; }

            TestObject& operator=(const TestObject&) = delete;
            TestObject& operator=(TestObject&& other) noexcept
            {
                m_Value = other.m_Value;
                other.m_Value = -1;
                return *this;
            }

            int GetValue() const { return m_Value; }

            static void ResetCounters()
            {
                s_ConstructorCount = 0;
                s_DestructorCount = 0;
            }

            static int GetConstructorCount() { return s_ConstructorCount; }
            static int GetDestructorCount() { return s_DestructorCount; }

        private:
            int m_Value;
            static int s_ConstructorCount;
            static int s_DestructorCount;
        };

        int TestObject::s_ConstructorCount = 0;
        int TestObject::s_DestructorCount = 0;

        // Test class that's too large for DeferredDelete
        struct TooLarge
        {
            char data[DeferredDelete::MaxDataSize + 1];
        };

        // Test class with alignment requirements
        struct alignas(64) AlignedObject
        {
            int value;
        };
    }

    // Basic functionality tests
    TEST_F(DeferredDeleteTest, DefaultConstruction)
    {
        DeferredDelete dd;
        EXPECT_FALSE(dd);
    }

    TEST_F(DeferredDeleteTest, BasicObject)
    {
        TestObject::ResetCounters();
        {
            TestObject obj(42);
            DeferredDelete dd(std::move(obj));
            EXPECT_TRUE(dd);
            EXPECT_EQ(TestObject::GetConstructorCount(), 2); // One for construction, one for move
            EXPECT_EQ(TestObject::GetDestructorCount(), 0);
        }
        EXPECT_EQ(TestObject::GetDestructorCount(), 2);
    }

    TEST_F(DeferredDeleteTest, MoveConstruction)
    {
        TestObject::ResetCounters();
        {
            TestObject obj(42);
            DeferredDelete dd1(std::move(obj));
            DeferredDelete dd2(std::move(dd1));
            EXPECT_FALSE(dd1);
            EXPECT_TRUE(dd2);
            EXPECT_EQ(TestObject::GetConstructorCount(), 3); // Original + obj move + deferred move
            EXPECT_EQ(TestObject::GetDestructorCount(), 1); // From the move constructor
        }
        EXPECT_EQ(TestObject::GetDestructorCount(), 3);
    }

    TEST_F(DeferredDeleteTest, MoveAssignment)
    {
        TestObject::ResetCounters();
        {
            DeferredDelete dd1(TestObject(42));
            DeferredDelete dd2(TestObject(24));
            dd2 = std::move(dd1);
            EXPECT_FALSE(dd1);
            EXPECT_TRUE(dd2);
            EXPECT_EQ(TestObject::GetConstructorCount(), 5); // Two originals + three moves
            EXPECT_EQ(TestObject::GetDestructorCount(), 4); // Two destroyed during move assignment + two temporary
        }
        EXPECT_EQ(TestObject::GetDestructorCount(), 5);
    }

    TEST_F(DeferredDeleteTest, SelfMoveAssignment)
    {
        TestObject::ResetCounters();
        {
            DeferredDelete dd(TestObject(42));
            dd = std::move(dd);
            EXPECT_TRUE(dd);
            EXPECT_EQ(TestObject::GetConstructorCount(), 2);
            EXPECT_EQ(TestObject::GetDestructorCount(), 1);
        }
        EXPECT_EQ(TestObject::GetDestructorCount(), 2);
    }

    // Complex object tests
    TEST_F(DeferredDeleteTest, StringObject)
    {
        DeferredDelete dd(std::string("test"));
        EXPECT_TRUE(dd);
    }

    TEST_F(DeferredDeleteTest, VectorObject)
    {
        DeferredDelete dd(std::vector<int>{1, 2, 3});
        EXPECT_TRUE(dd);
    }

    TEST_F(DeferredDeleteTest, UniquePtrObject)
    {
        DeferredDelete dd(std::make_unique<int>(42));
        EXPECT_TRUE(dd);
    }

    // Compile-time tests
    TEST_F(DeferredDeleteTest, CompileTimeChecks)
    {
        // These should fail to compile
        // DeferredDelete dd1(TooLarge{}); // Too large
        // DeferredDelete dd2(AlignedObject{}); // Bad alignment
        // DeferredDelete dd3(DeferredDelete{}); // Self type
        // DeferredDelete dd4 = dd1; // Copy construction
        // dd1 = dd2; // Copy assignment
    }

    // Edge cases
    TEST_F(DeferredDeleteTest, EmptyMove)
    {
        DeferredDelete dd1;
        DeferredDelete dd2(std::move(dd1));
        EXPECT_FALSE(dd1);
        EXPECT_FALSE(dd2);
    }

    TEST_F(DeferredDeleteTest, EmptyMoveAssignment)
    {
        DeferredDelete dd1;
        DeferredDelete dd2(TestObject(42));
        dd2 = std::move(dd1);
        EXPECT_FALSE(dd1);
        EXPECT_FALSE(dd2);
    }

    TEST_F(DeferredDeleteTest, MoveToEmpty)
    {
        DeferredDelete dd1(TestObject(42));
        DeferredDelete dd2;
        dd2 = std::move(dd1);
        EXPECT_FALSE(dd1);
        EXPECT_TRUE(dd2);
    }

    // Multiple moves
    TEST_F(DeferredDeleteTest, MultipleMoves)
    {
        TestObject::ResetCounters();
        {
            DeferredDelete dd1(TestObject(42));
            DeferredDelete dd2(std::move(dd1));
            DeferredDelete dd3(std::move(dd2));
            EXPECT_FALSE(dd1);
            EXPECT_FALSE(dd2);
            EXPECT_TRUE(dd3);
            EXPECT_EQ(TestObject::GetConstructorCount(), 4); // Temp Object + Original + two moves
            EXPECT_EQ(TestObject::GetDestructorCount(), 3);
        }
        EXPECT_EQ(TestObject::GetDestructorCount(), 4);
    }
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
