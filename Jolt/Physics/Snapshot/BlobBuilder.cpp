#include <Jolt/Jolt.h>
#include <Jolt/Physics/Snapshot/BlobBuilder.h>

JPH_NAMESPACE_BEGIN

BlobDataRef BlobBuilder::Allocate(size_t size, uint alignment)
{
    if (size > mChunkSize)
    {
        size = AlignUp(size, 16);
        size_t allocIndex = mAllocations.size();
        char *mem = static_cast<char*>(AlignedAllocate(size, alignment));
        memset(mem, 0, size);
        mAllocations.push_back(BlobAllocation(size, mem));
        return BlobDataRef(allocIndex, 0);
    }

    auto alloc = EnsureEnoughRoomInChunk(size, alignment);
    auto offset = static_cast<int>(alloc.size);

    memset(alloc.p + alloc.size, 0, size);
    alloc.size += size;
    mAllocations[mCurrentChunkInIndex.value()] = alloc;
    return BlobDataRef(mCurrentChunkInIndex.value(), offset);
}

BlobAllocation BlobBuilder::EnsureEnoughRoomInChunk(size_t size, uint alignment)
{
    if (!mCurrentChunkInIndex.has_value())
        return AllocateNewChunk();

    auto alloc = mAllocations[mCurrentChunkInIndex.value()];
    size_t startOffset = AlignUp(alloc.size, alignment);
    if (startOffset + size > mChunkSize)
        return AllocateNewChunk();

    memset(alloc.p + alloc.size, 0, startOffset - alloc.size);
    alloc.size = startOffset;

    return alloc;
}

BlobAllocation BlobBuilder::AllocateNewChunk()
{
    // align size of last chunk to 16 bytes so chunks can be concatenated without breaking alignment
    if (mCurrentChunkInIndex.has_value())
    {
        AlignChunk(mCurrentChunkInIndex.value());
    }

    mCurrentChunkInIndex = mAllocations.size();
    auto alloc = BlobAllocation(
        0,
        (char*)AlignedAllocate(mChunkSize, 16)
    );
    mAllocations.push_back(alloc);
    return alloc;
}

void BlobBuilder::AlignChunk(size_t chunkIndex)
{
    auto chunk = mAllocations[chunkIndex];
    auto oldSize = chunk.size;
    chunk.size = AlignUp(chunk.size, 16);
    mAllocations[chunkIndex] = chunk;
    memset(chunk.p + oldSize, 0, chunk.size - oldSize);
}

void* BlobBuilder::AllocationToPointer(const BlobDataRef blobDataRef)
{
    return mAllocations[blobDataRef.allocIndex].p + blobDataRef.offset;
}

size_t BlobBuilder::GetBlobByteCount()
{
    if (mCurrentChunkInIndex.has_value())
    {
        AlignChunk(mCurrentChunkInIndex.value());
    }

    size_t count = 0;
    for (const auto & mAllocation : mAllocations)
    {
        count += mAllocation.size;
    }
    return count + sizeof(BlobHeader);
}

void BlobBuilder::CreateBlobBytes(void* outBytes, size_t outByteCount)
{
    if (mCurrentChunkInIndex.has_value())
    {
        AlignChunk(mCurrentChunkInIndex.value());
    }

    Array<size_t> offsets(mAllocations.size() + 1);
    Array<SortedIndex> sortedAllocations(mAllocations.size());
    Array<SortedIndex> sortedPatches(mPatches.size());

    offsets[0] = 0;
    for (int i = 0; i < mAllocations.size(); ++i)
    {
        offsets[i + 1] = offsets[i] + mAllocations[i].size;
        sortedAllocations[i] = SortedIndex(mAllocations[i].p, i);
    }
    QuickSort(sortedAllocations.begin(), sortedAllocations.end(), SortedIndex::Compare);

    for (int i = 0; i < mPatches.size(); ++i)
        sortedPatches[i] = SortedIndex(reinterpret_cast<char*>(mPatches[i].offsetPtr), i);
    QuickSort(sortedPatches.begin(), sortedPatches.end(), SortedIndex::Compare);

    size_t dataSize = offsets[mAllocations.size()];
    JPH_ASSERT(outByteCount >= dataSize);

    char* data = static_cast<char*>(outBytes) + sizeof(BlobHeader);

    for (int i = 0; i < mAllocations.size(); ++i)
        memcpy(data + offsets[i], mAllocations[i].p, mAllocations[i].size);

    uint iAlloc = 0;
    auto allocStart = mAllocations[sortedAllocations[0].index].p;
    auto allocEnd = allocStart + mAllocations[sortedAllocations[0].index].size;

    for (int i = 0; i < mPatches.size(); ++i)
    {
        int patchIndex = sortedPatches[i].index;
        int* offsetPtr = reinterpret_cast<int*>(sortedPatches[i].p);

        while (reinterpret_cast<char*>(offsetPtr) >= allocEnd)
        {
            ++iAlloc;
            allocStart = mAllocations[sortedAllocations[iAlloc].index].p;
            allocEnd = allocStart + mAllocations[sortedAllocations[iAlloc].index].size;
        }

        auto patch = mPatches[patchIndex];

        size_t offsetPtrInData = offsets[sortedAllocations[iAlloc].index]
            + static_cast<int>(reinterpret_cast<char*>(offsetPtr) - allocStart);
        size_t targetPtrInData = offsets[patch.target.allocIndex] + patch.target.offset;

        *reinterpret_cast<int*>(data + offsetPtrInData) = static_cast<int>(targetPtrInData - offsetPtrInData);
        if (patch.length != 0)
        {
            *reinterpret_cast<int*>(data + offsetPtrInData + sizeof(int)) = static_cast<int>(patch.length);
        }
    }

    auto header = static_cast<BlobHeader*>(outBytes);
    header->length = dataSize;
    header->hash = HashBytes(data, dataSize);
}

JPH_NAMESPACE_END
