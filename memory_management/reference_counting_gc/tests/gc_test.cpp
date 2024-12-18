#include <gtest/gtest.h>
#include "memory_management/reference_counting_gc/include/object_module.h"
#include "base/macros.h"
#include "delete_detector.h"

class Return42 {
    static constexpr size_t RET_42 = 42U;
public:
    size_t Get() const
    {
        return RET_42;
    }
};

TEST(ReferenceCountingGC, DISABLED_SinglePtrUsage)
{
    Object<size_t> obj;
    ASSERT_EQ(obj.UseCount(), 0);
    ASSERT_EQ(obj.Get(), nullptr);

    constexpr size_t VALUE_TO_CREATE = 42U;
    Object<size_t> sizeObj = MakeObject<size_t>(VALUE_TO_CREATE);
    ASSERT_EQ(sizeObj.UseCount(), 1U);
    ASSERT_NE(sizeObj.Get(), nullptr);
    ASSERT_EQ(*sizeObj, VALUE_TO_CREATE);

    Object<Return42> classObj = MakeObject<Return42>();
    ASSERT_EQ(classObj.UseCount(), 1U);
    ASSERT_NE(classObj.Get(), nullptr);
    ASSERT_EQ(classObj->Get(), Return42().Get());
}

TEST(ReferenceCountingGC, DISABLED_CopySemanticUsage)
{
    constexpr size_t VALUE_TO_CREATE = 42U;
    Object<size_t> obj1 = MakeObject<size_t>(VALUE_TO_CREATE);
    {
        Object<size_t> obj2(obj1); // NOLINT(performance-unnecessary-copy-initialization)
        ASSERT_EQ(obj1.UseCount(), 2U);
        ASSERT_EQ(obj1.Get(), obj2.Get());
    }
    ASSERT_EQ(obj1.UseCount(), 1U);
    {
        Object<size_t> obj2;
        obj2 = obj1;
        ASSERT_EQ(obj1.UseCount(), 2U);
        ASSERT_EQ(obj1.Get(), obj2.Get());
    }
    ASSERT_EQ(obj1.UseCount(), 1U);
}

TEST(ReferenceCountingGC, DISABLED_MoveSemanticUsage)
{
    constexpr size_t VALUE_TO_CREATE = 42U;
    Object<size_t> obj1 = MakeObject<size_t>(VALUE_TO_CREATE);
    size_t *ptr = obj1.Get();
    {
        Object<size_t> obj2(std::move(obj1));
        ASSERT_EQ(obj2.UseCount(), 1U);
        ASSERT_EQ(obj2.Get(), ptr);
        obj1 = obj2;
    }
    ASSERT_EQ(obj1.UseCount(), 1U);
    {
        Object<size_t> obj2;
        obj2 = std::move(obj1);
        ASSERT_EQ(obj2.UseCount(), 1U);
        ASSERT_EQ(obj2.Get(), ptr);
        obj1 = obj2;
    }
    ASSERT_EQ(obj1.UseCount(), 1U);

    auto obj2 = obj1;
    ASSERT_EQ(obj1.UseCount(), 2U);
    ASSERT_EQ(obj1.Get(), obj2.Get());
    {
        Object<size_t> obj3;
        obj3 = std::move(obj2);
        ASSERT_EQ(obj3.UseCount(), 2U);
        ASSERT_EQ(obj3.Get(), obj1.Get());
        ASSERT_EQ(obj2.Get(), nullptr); // NOLINT(bugprone-use-after-move, clang-analyzer-cplusplus.Move)
        ASSERT_EQ(obj2.UseCount(), 0U); // NOLINT(bugprone-use-after-move, clang-analyzer-cplusplus.Move)
    }
    ASSERT_EQ(obj1.UseCount(), 1U);
}


TEST(ReferenceCountingGC, DISABLED_GcDeletingTest) {
    DeleteDetector::SetDeleteCount(0U);
    auto obj1 = MakeObject<DeleteDetector>();
    {
        auto obj2 = MakeObject<DeleteDetector>();
        obj2->SetDelete(MakeObject<DeleteDetector>());
        obj1->SetDelete(obj2);
        ASSERT_EQ(obj2.UseCount(), 2U);
    }
    ASSERT_EQ(DeleteDetector::GetDeleteCount(), 0U);
    obj1->SetDelete(Object<DeleteDetector>());
    ASSERT_EQ(DeleteDetector::GetDeleteCount(), 2U);
    obj1->~DeleteDetector();
    ASSERT_EQ(DeleteDetector::GetDeleteCount(), 3U);
}