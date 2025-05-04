module;

#include <gtest/gtest.h>
#include <atomic>
#include <string>

export module YT:DelegateTests;

import :Delegate;
import :Types;

namespace YT
{
    class DelegateTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            // Reset any static state before each test
        }

        void TearDown() override
        {
            // Clean up after each test
        }
    };

    // Test basic void delegate functionality
    TEST_F(DelegateTest, BasicVoidDelegate)
    {
        Delegate<void()> delegate;
        std::atomic<int> counter = 0;

        // Bind a simple lambda
        auto handle = delegate.BindNoValidityCheck([&counter]() { counter++; });

        // Execute the delegate
        delegate.Execute();
        EXPECT_EQ(counter, 1);

        // Execute again
        delegate.Execute();
        EXPECT_EQ(counter, 2);

        // Unbind and verify it's no longer called
        delegate.Unbind(handle);
        delegate.Execute();
        EXPECT_EQ(counter, 2);
    }

    // Test delegate with return value
    TEST_F(DelegateTest, ReturnValueDelegate)
    {
        Delegate<int()> delegate;
        int value = 42;

        // Bind a lambda that returns a value
        auto handle = delegate.BindNoValidityCheck([&value]() { return value; });

        // Execute and verify return value
        int result = delegate.Execute();
        EXPECT_EQ(result, value);

        // Change the value and verify
        value = 100;
        result = delegate.Execute();
        EXPECT_EQ(result, value);
    }

    // Test delegate with arguments
    TEST_F(DelegateTest, ArgumentDelegate)
    {
        Delegate<int(int, int)> delegate;

        // Bind a lambda that takes arguments
        auto handle = delegate.BindNoValidityCheck([](int a, int b) { return a + b; });

        // Execute with arguments
        int result = delegate.Execute(10, 20);
        EXPECT_EQ(result, 30);

        // Try different arguments
        result = delegate.Execute(5, 7);
        EXPECT_EQ(result, 12);
    }

    // Test validity checking
    TEST_F(DelegateTest, ValidityCheck)
    {
        Delegate<void()> delegate;
        std::atomic<int> counter = 0;

        bool is_valid = true;

        // Bind with validity check
        auto handle = delegate.BindWithValidityCheck(
            [&is_valid]() { return is_valid; },
            [&counter]() { counter++; }
        );

        // Execute while valid
        delegate.Execute();
        EXPECT_EQ(counter, 1);

        // Make invalid and execute
        is_valid = false;
        delegate.Execute();
        EXPECT_EQ(counter, 1); // Should not increment

        // Make valid again - the delegate has been removed
        is_valid = true;
        delegate.Execute();
        EXPECT_EQ(counter, 1);
    }

    struct TestHandle
    {
        explicit TestHandle(bool value) :
            m_Value(std::make_shared<bool>(value))
        {

        }

        TestHandle(const TestHandle&) = default;
        TestHandle(TestHandle&&) = default;
        TestHandle& operator=(const TestHandle&) = default;
        TestHandle& operator=(TestHandle&&) = default;
        ~TestHandle() = default;

        void SetValue(bool value)
        {
            *m_Value = value;
        }

        operator bool() const noexcept
        {
            return *m_Value;
        }

        std::shared_ptr<bool> m_Value;
    };

    // Test handle-based validity checking
    TEST_F(DelegateTest, HandleValidityCheck)
    {
        Delegate<void()> delegate;
        std::atomic<int> counter = 0;
        TestHandle handle_valid(true);

        // Bind with handle-based validity check
        auto handle = delegate.BindWithHandle(
            handle_valid,
            [&counter]() { counter++; }
        );

        // Execute while handle is valid
        delegate.Execute();
        EXPECT_EQ(counter, 1);

        // Make handle invalid and execute
        handle_valid.SetValue(false);
        delegate.Execute();
        EXPECT_EQ(counter, 1); // Should not increment
    }

    // Test multiple bindings
    TEST_F(DelegateTest, MultipleBindings)
    {
        Delegate<void()> delegate;
        std::atomic<int> counter1 = 0;
        std::atomic<int> counter2 = 0;

        // Bind two different callbacks
        auto handle1 = delegate.BindNoValidityCheck([&counter1]() { counter1++; });
        auto handle2 = delegate.BindNoValidityCheck([&counter2]() { counter2++; });

        // Execute and verify both are called
        delegate.Execute();
        EXPECT_EQ(counter1, 1);
        EXPECT_EQ(counter2, 1);

        // Unbind one and verify
        delegate.Unbind(handle1);
        delegate.Execute();
        EXPECT_EQ(counter1, 1); // Should not increment
        EXPECT_EQ(counter2, 2); // Should increment
    }

    // Test unbind all
    TEST_F(DelegateTest, UnbindAll)
    {
        Delegate<void()> delegate;
        std::atomic<int> counter1 = 0;
        std::atomic<int> counter2 = 0;

        // Bind two callbacks
        delegate.BindNoValidityCheck([&counter1]() { counter1++; });
        delegate.BindNoValidityCheck([&counter2]() { counter2++; });

        // Execute and verify
        delegate.Execute();
        EXPECT_EQ(counter1, 1);
        EXPECT_EQ(counter2, 1);

        // Unbind all and verify
        delegate.UnbindAll();
        delegate.Execute();
        EXPECT_EQ(counter1, 1); // Should not increment
        EXPECT_EQ(counter2, 1); // Should not increment
    }

    // Test default return value
    TEST_F(DelegateTest, DefaultReturnValue)
    {
        Delegate<int()> delegate;
        int default_value = 42;

        // Execute with no bindings and default return
        int result = delegate.ExecuteWithDefault(default_value);
        EXPECT_EQ(result, default_value);

        // Bind a callback and verify it overrides default
        auto handle = delegate.BindNoValidityCheck([]() { return 100; });
        result = delegate.ExecuteWithDefault(default_value);
        EXPECT_EQ(result, 100);
    }

    // Test move operations
    TEST_F(DelegateTest, MoveOperations)
    {
        Delegate<void()> delegate1;
        std::atomic<int> counter = 0;

        // Bind a callback to first delegate
        delegate1.BindNoValidityCheck([&counter]() { counter++; });

        // Move to second delegate
        Delegate<void()> delegate2 = std::move(delegate1);

        // Execute moved delegate
        delegate2.Execute();
        EXPECT_EQ(counter, 1);

        // Original delegate should be empty
        delegate1.Execute(); // Should do nothing
        EXPECT_EQ(counter, 1);
    }

    // Test handle operations
    TEST_F(DelegateTest, HandleOperations)
    {
        Delegate<void()> delegate;
        std::atomic<int> counter = 0;

        // Create a handle
        auto handle = delegate.BindNoValidityCheck([&counter]() { counter++; });

        // Test handle bool conversion
        EXPECT_TRUE(handle);
        EXPECT_TRUE(static_cast<bool>(handle));

        // Create empty handle
        DelegateHandle empty_handle;
        EXPECT_FALSE(empty_handle);
        EXPECT_FALSE(static_cast<bool>(empty_handle));
    }

    // Test delegate with complex types
    TEST_F(DelegateTest, ComplexTypes)
    {
        Delegate<std::string(const std::string&, int)> delegate;

        // Bind a lambda that takes complex types
        auto handle = delegate.BindNoValidityCheck(
            [](const std::string& str, int count) {
                std::string result;
                for (int i = 0; i < count; ++i) {
                    result += str;
                }
                return result;
            }
        );

        // Execute with complex arguments
        std::string result = delegate.Execute("test", 3);
        EXPECT_EQ(result, "testtesttest");
    }

    // Test delegate with move-only types
    TEST_F(DelegateTest, MoveOnlyTypes)
    {
        Delegate<std::unique_ptr<int>(std::unique_ptr<int>)> delegate;

        // Bind a lambda that takes move-only types
        auto handle = delegate.BindNoValidityCheck(
            [](std::unique_ptr<int> ptr) {
                *ptr *= 2;
                return ptr;
            }
        );

        // Execute with move-only argument
        auto input = std::make_unique<int>(5);
        auto result = delegate.Execute(std::move(input));
        EXPECT_EQ(*result, 10);
    }
}


int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
