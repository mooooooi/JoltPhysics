#pragma once

#include<Jolt/Math/Vec3.h>
#include<Jolt/Math/Quat.h>
#include<Jolt/Geometry/Sphere.h>
#include<Jolt/Physics/StateRecorder.h>
#include<optional>

JPH_NAMESPACE_BEGIN

template<typename T>
class BlobArray : NonCopyable
{
public:
    BlobArray() = default;

    inline T & operator [](uint index)
    {
        JPH_ASSERT(index < mLength);
        auto offsetPtr = reinterpret_cast<char*>(&mOffsetPtr);
        auto ptr = reinterpret_cast<T*>(offsetPtr + mOffsetPtr);
        return ptr[index];
    }

    inline const T & operator [](uint index) const
    {
        JPH_ASSERT(index < mLength);
        auto offsetPtr = reinterpret_cast<const char*>(&mOffsetPtr);
        auto ptr = reinterpret_cast<const T*>(offsetPtr + mOffsetPtr);
        return ptr[index];
    }
	
	inline int size() const { return mLength; }
    inline int* GetUnsafeOffsetPtr() { return &mOffsetPtr; }
private:
    int mOffsetPtr;
    int mLength;
};

template<typename T>
class BlobBuilderArray
{
public:
    BlobBuilderArray() = delete;
    inline BlobBuilderArray(T *data, size_t length) : mData(data), mLength(length)
    {

    }

    inline T				    operator [] (size_t index) const
    {
        JPH_ASSERT(index < mLength);
        return mData[index];
    }

    inline T &				    operator [] (size_t index)
    {
        JPH_ASSERT(index < mLength);
        return mData[index];
    }

    inline uint size() const
    {
        return mLength;
    }

    inline T* GetUnsafePtr() { return mData; }

private:
        T *mData;
        size_t mLength;
};

struct BlobAllocation
{
    size_t size;
    char *p;

    BlobAllocation(size_t size, char *p) : size(size), p(p) {}
};

struct BlobDataRef
{
    size_t allocIndex;
    int offset;

    BlobDataRef(size_t allocIndex, int offset) : allocIndex(allocIndex), offset(offset) {}
};

struct OffsetPtrPatch
{
    int *offsetPtr;
    BlobDataRef target;
    size_t length;

    OffsetPtrPatch(int *inOffsetPtr, const BlobDataRef inTarget, size_t inLength)
        : offsetPtr(inOffsetPtr), target(inTarget), length(inLength) {}
};

struct SortedIndex
{
    char* p;
    int index;

    SortedIndex() = default;
    SortedIndex(char* p, int index) : p(p), index(index) {}

    static bool Compare(const SortedIndex &a, const SortedIndex &b)
    {
        return a.p < b.p;
    }
};

struct BlobHeader
{
    uint length;
    unsigned long hash;
};

class BlobBuilder final
{
public:
    BlobBuilder() : BlobBuilder(65536) {}
    explicit BlobBuilder(const int chunkSize) : mCurrentChunkInIndex(std::nullopt), mChunkSize(AlignUp(chunkSize, 16)) {}
    virtual ~BlobBuilder()
    {
        for (const auto & alloc : mAllocations) {
            AlignedFree(alloc.p);
        }
    }

    template<typename T>
    T& ConstructRoot()
    {
        auto alloc = Allocate(sizeof(T), alignof(T));
        return *static_cast<T*>(AllocationToPointer(alloc));
    }

    template<typename T, typename... TArgs, std::enable_if_t<std::is_same<T, TArgs...>::type, bool> = true>
    BlobBuilderArray<T> Construct(BlobArray<T> &blobArray, TArgs... args)
    {
        int len = sizeof...(args);
        BlobBuilderArray<T> constructorBlobArray = Allocate(blobArray, len);

        ((*constructorBlobArray.GetUnsafePtr() = args), ...);

        return constructorBlobArray;
    }

    template<typename T>
    BlobBuilderArray<T> Allocate(BlobArray<T> &blobArray, size_t length, uint alignment = alignof(T))
    {
        JPH_ASSERT(IsPowerOf2(alignment));
        JPH_ASSERT(alignment <= 16);

        int* offsetPtr = blobArray.GetUnsafeOffsetPtr();

        const auto alloc = Allocate(sizeof(T) * length, alignment);

        mPatches.push_back(OffsetPtrPatch(offsetPtr, alloc, length));
        return BlobBuilderArray<T>(static_cast<T*>(AllocationToPointer(alloc)), length);
    }

    size_t GetBlobByteCount();
    void CreateBlobBytes(void* outBytes, size_t outByteCount);

private:
    Array<BlobAllocation> mAllocations;
    Array<OffsetPtrPatch> mPatches;

    std::optional<size_t> mCurrentChunkInIndex;
    size_t mChunkSize;

    BlobDataRef Allocate(size_t size, uint alignment);
    BlobAllocation EnsureEnoughRoomInChunk(size_t size, uint alignment);
    BlobAllocation AllocateNewChunk();
    void AlignChunk(size_t chunkIndex);
    void* AllocationToPointer(BlobDataRef blobDataRef);
};

JPH_NAMESPACE_END
