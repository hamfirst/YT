module;

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <memory>

module YT:ObjectPoolTests;

import :ObjectPool;

namespace YT
{
    namespace
    {
        // Test fixture for ObjectPool tests
        class ObjectPoolTest : public ::testing::Test
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
    }

    // Basic functionality tests
    TEST_F(ObjectPoolTest, DefaultConstruction)
    {
        ObjectPool<TestObject> pool;
    }

    TEST_F(ObjectPoolTest, BasicObjectAllocation)
    {
        TestObject::ResetCounters();
        {
            ObjectPool<TestObject> pool;
            TestObject temp(42);
            auto obj = pool.GetFreeObject([&temp] { return std::move(temp); });
            EXPECT_TRUE(obj);
            EXPECT_EQ((*obj).GetValue(), 42);
            EXPECT_EQ(TestObject::GetConstructorCount(), 2); // One for temp, one for move
            EXPECT_EQ(TestObject::GetDestructorCount(), 0);
        }
        EXPECT_EQ(TestObject::GetDestructorCount(), 2);
    }

    TEST_F(ObjectPoolTest, ObjectReuse)
    {
        TestObject::ResetCounters();
        {
            ObjectPool<TestObject> pool;
            {
                TestObject temp1(42);
                auto obj1 = pool.GetFreeObject([&temp1] { return std::move(temp1); });
                EXPECT_EQ((*obj1).GetValue(), 42);
            }
            {
                TestObject temp2(24);
                auto obj2 = pool.GetFreeObject([&temp2] { return std::move(temp2); });
                EXPECT_EQ((*obj2).GetValue(), 42); // We didn't make a new object
            }
            EXPECT_EQ(TestObject::GetConstructorCount(), 3); // Two temps, one move
            EXPECT_EQ(TestObject::GetDestructorCount(), 2); // Two temps destroyed
        }
        EXPECT_EQ(TestObject::GetDestructorCount(), 3);
    }

    // PooledObject tests
    TEST_F(ObjectPoolTest, PooledObjectDefaultConstruction)
    {
        PooledObject<TestObject> obj;
        EXPECT_FALSE(obj);
    }

    TEST_F(ObjectPoolTest, PooledObjectMoveConstruction)
    {
        TestObject::ResetCounters();
        {
            ObjectPool<TestObject> pool;
            auto obj1 = pool.GetFreeObject([] { return TestObject(42); });
            auto obj2 = std::move(obj1);
            EXPECT_FALSE(obj1);
            EXPECT_TRUE(obj2);
            EXPECT_EQ((*obj2).GetValue(), 42);
        }
        EXPECT_EQ(TestObject::GetDestructorCount(), 1);
    }

    TEST_F(ObjectPoolTest, PooledObjectMoveAssignment)
    {
        TestObject::ResetCounters();
        {
            ObjectPool<TestObject> pool;
            auto obj1 = pool.GetFreeObject([] { return TestObject(42); });
            auto obj2 = pool.GetFreeObject([] { return TestObject(24); });
            obj2 = std::move(obj1);
            EXPECT_FALSE(obj1);
            EXPECT_TRUE(obj2);
            EXPECT_EQ((*obj2).GetValue(), 42);
        }
        EXPECT_EQ(TestObject::GetDestructorCount(), 2);
    }

    TEST_F(ObjectPoolTest, PooledObjectSelfMoveAssignment)
    {
        TestObject::ResetCounters();
        {
            ObjectPool<TestObject> pool;
            auto obj = pool.GetFreeObject([] { return TestObject(42); });
            obj = std::move(obj);
            EXPECT_TRUE(obj);
            EXPECT_EQ((*obj).GetValue(), 42);
        }
        EXPECT_EQ(TestObject::GetDestructorCount(), 1);
    }

    // Complex object tests
    TEST_F(ObjectPoolTest, StringObject)
    {
        ObjectPool<std::string> pool;
        auto str = pool.GetFreeObject([] { return std::string("test"); });
        EXPECT_TRUE(str);
        EXPECT_EQ(*str, "test");
    }

    TEST_F(ObjectPoolTest, VectorObject)
    {
        ObjectPool<std::vector<int>> pool;
        auto vec = pool.GetFreeObject([] { return std::vector<int>{1, 2, 3}; });
        EXPECT_TRUE(vec);
        EXPECT_EQ(vec->size(), 3);
        EXPECT_EQ((*vec)[0], 1);
    }

    TEST_F(ObjectPoolTest, UniquePtrObject)
    {
        ObjectPool<std::unique_ptr<int>> pool;
        auto ptr = pool.GetFreeObject([] { return std::make_unique<int>(42); });
        EXPECT_TRUE(ptr);
        EXPECT_EQ(**ptr, 42);
    }

    // Multiple object tests
    TEST_F(ObjectPoolTest, MultipleObjects)
    {
        TestObject::ResetCounters();
        {
            ObjectPool<TestObject> pool;
            std::vector<PooledObject<TestObject>> objects;
            for (int i = 0; i < 10; ++i)
            {
                TestObject temp(i);
                objects.push_back(pool.GetFreeObject([&temp] { return std::move(temp); }));
            }
            for (size_t i = 0; i < objects.size(); ++i)
            {
                EXPECT_TRUE(objects[i]);
                EXPECT_EQ((*objects[i]).GetValue(), i);
            }
        }
        EXPECT_EQ(TestObject::GetConstructorCount(), 20); // 10 temps, 10 moves
        EXPECT_EQ(TestObject::GetDestructorCount(), 20); // 10 temps, 10 from pool
    }

    // Object reuse tests
    TEST_F(ObjectPoolTest, ObjectReuseAfterDestruction)
    {
        TestObject::ResetCounters();
        {
            ObjectPool<TestObject> pool;
            {
                TestObject temp1(42);
                auto obj1 = pool.GetFreeObject([&temp1] { return std::move(temp1); });
                EXPECT_EQ((*obj1).GetValue(), 42);
            }
            {
                TestObject temp2(42);
                auto obj2 = pool.GetFreeObject([&temp2] { return std::move(temp2); });
                EXPECT_EQ((*obj2).GetValue(), 42);
            }
            {
                TestObject temp3(12);
                auto obj3 = pool.GetFreeObject([&temp3] { return std::move(temp3); });
                EXPECT_EQ((*obj3).GetValue(), 42);
            }
        }
        EXPECT_EQ(TestObject::GetConstructorCount(), 4); // Three temps, one move
        EXPECT_EQ(TestObject::GetDestructorCount(), 4); // Three temps, one pooled
    }

    // Edge cases
    TEST_F(ObjectPoolTest, EmptyPooledObjectOperations)
    {
        PooledObject<TestObject> obj;
        EXPECT_FALSE(obj);
        // These should not crash
        obj = std::move(obj);
        PooledObject<TestObject> obj2(std::move(obj));
        EXPECT_FALSE(obj);
        EXPECT_FALSE(obj2);
    }

    // Reinitializer tests
    TEST_F(ObjectPoolTest, ReinitializerBasic)
    {
        TestObject::ResetCounters();
        {
            ObjectPool<TestObject> pool;
            {
                TestObject temp1(42);
                auto obj1 = pool.GetFreeObject([&temp1] { return std::move(temp1); }, [](TestObject& obj) { obj = TestObject(24); });
                EXPECT_EQ((*obj1).GetValue(), 42);
            }
            {
                TestObject temp2(100);
                auto obj2 = pool.GetFreeObject([&temp2] { return std::move(temp2); }, [](TestObject& obj) { obj = TestObject(24); });
                EXPECT_EQ((*obj2).GetValue(), 24); // Reinitialized value
            }
            EXPECT_EQ(TestObject::GetConstructorCount(), 4); // One pool, two moves, one reinit
            EXPECT_EQ(TestObject::GetDestructorCount(), 3); // Three temps destroyed
        }
        EXPECT_EQ(TestObject::GetDestructorCount(), 4); // Plus one from pool cleanup
    }

    TEST_F(ObjectPoolTest, ReinitializerMultipleReuse)
    {
        TestObject::ResetCounters();
        {
            ObjectPool<TestObject> pool;
            {
                TestObject temp1(42);
                auto obj1 = pool.GetFreeObject([&temp1] { return std::move(temp1); }, [](TestObject& obj) { obj = TestObject(24); });
                EXPECT_EQ((*obj1).GetValue(), 42);
            }
            {
                TestObject temp2(100);
                auto obj2 = pool.GetFreeObject([&temp2] { return std::move(temp2); }, [](TestObject& obj) { obj = TestObject(24); });
                EXPECT_EQ((*obj2).GetValue(), 24);
            }
            {
                TestObject temp3(200);
                auto obj3 = pool.GetFreeObject([&temp3] { return std::move(temp3); }, [](TestObject& obj) { obj = TestObject(36); });
                EXPECT_EQ((*obj3).GetValue(), 36);
            }
            EXPECT_EQ(TestObject::GetConstructorCount(), 6); // One pool, Three temps, two reinit
            EXPECT_EQ(TestObject::GetDestructorCount(), 5); // Three temps, two reinit destroyed
        }
        EXPECT_EQ(TestObject::GetDestructorCount(), 6); // Plus one from pool cleanup
    }

    TEST_F(ObjectPoolTest, ReinitializerWithComplexType)
    {
        ObjectPool<std::vector<int>> pool;
        {
            auto vec1 = pool.GetFreeObject(
                [] { return std::vector<int>{1, 2, 3}; },
                [](std::vector<int>& vec) { vec = {4, 5, 6}; }
            );
            EXPECT_EQ(vec1->size(), 3);
            EXPECT_EQ((*vec1)[0], 1);
        }
        {
            auto vec2 = pool.GetFreeObject(
                [] { return std::vector<int>{7, 8, 9}; },
                [](std::vector<int>& vec) { vec = {4, 5, 6}; }
            );
            EXPECT_EQ(vec2->size(), 3);
            EXPECT_EQ((*vec2)[0], 4); // Reinitialized value
        }
    }

    TEST_F(ObjectPoolTest, ReinitializerWithString)
    {
        ObjectPool<std::string> pool;
        {
            auto str1 = pool.GetFreeObject(
                [] { return std::string("original"); },
                [](std::string& str) { str = "reinitialized"; }
            );
            EXPECT_EQ(*str1, "original");
        }
        {
            auto str2 = pool.GetFreeObject(
                [] { return std::string("new"); },
                [](std::string& str) { str = "reinitialized"; }
            );
            EXPECT_EQ(*str2, "reinitialized");
        }
    }

    TEST_F(ObjectPoolTest, ReinitializerWithUniquePtr)
    {
        ObjectPool<std::unique_ptr<int>> pool;
        {
            auto ptr1 = pool.GetFreeObject(
                [] { return std::make_unique<int>(42); },
                [](std::unique_ptr<int>& ptr) { ptr = std::make_unique<int>(24); }
            );
            EXPECT_EQ(**ptr1, 42);
        }
        {
            auto ptr2 = pool.GetFreeObject(
                [] { return std::make_unique<int>(100); },
                [](std::unique_ptr<int>& ptr) { ptr = std::make_unique<int>(24); }
            );
            EXPECT_EQ(**ptr2, 24);
        }
    }
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
