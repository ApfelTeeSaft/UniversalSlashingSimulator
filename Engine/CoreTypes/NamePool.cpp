/**
 * UniversalSlashingSimulator - Name Pool Implementation
 */

#include "NamePool.h"
#include "../../Core/Memory/Memory.h"
#include "../../Core/Logging/Log.h"
#include "../../Core/Versioning/VersionResolver.h"

namespace USS
{
    //=========================================================================
    // FGNamesArray Implementation (Pre-4.23)
    //=========================================================================

    FGNamesArray::FGNamesArray()
        : m_BaseAddress(0)
        , m_ChunksPtr(0)
        , m_NumElements(0)
        , m_bInitialized(false)
    {
    }

    EResult FGNamesArray::Initialize(uintptr Address)
    {
        if (m_bInitialized)
            return EResult::AlreadyInitialized;

        if (Address == 0)
            return EResult::Failed;

        m_BaseAddress = Address;

        // GNames is typically a pointer to TNameEntryArray
        // TNameEntryArray layout:
        // FNameEntry** Chunks[128];  // 0x00 - Array of chunk pointers
        // int32 NumElements;         // After chunks
        // int32 NumChunks;

        if (!Memory::Read<uintptr>(Address, m_ChunksPtr))
        {
            USS_ERROR("Failed to read GNames chunks pointer");
            return EResult::Failed;
        }

        // Estimate number of elements by reading chunks
        // Normally, we'd scan until we find null entries
        m_NumElements = 0;

        for (int32 ChunkIdx = 0; ChunkIdx < 128; ++ChunkIdx)
        {
            uintptr ChunkPtr = 0;
            if (!Memory::Read<uintptr>(m_ChunksPtr + (ChunkIdx * sizeof(uintptr)), ChunkPtr))
                break;

            if (ChunkPtr == 0)
                break;

            m_NumElements += ElementsPerChunk;
        }

        USS_LOG("FGNamesArray initialized: ~%d estimated elements", m_NumElements);

        m_bInitialized = true;
        return EResult::Success;
    }

    bool FGNamesArray::GetName(int32 ComparisonIndex, FResolvedName& OutName) const
    {
        if (!IsValidIndex(ComparisonIndex))
            return false;

        int32 ChunkIndex = ComparisonIndex / ElementsPerChunk;
        int32 WithinIndex = ComparisonIndex % ElementsPerChunk;

        uintptr ChunkPtr = 0;
        if (!Memory::Read<uintptr>(m_ChunksPtr + (ChunkIndex * sizeof(uintptr)), ChunkPtr))
            return false;

        if (ChunkPtr == 0)
            return false;

        uintptr EntryPtr = 0;
        if (!Memory::Read<uintptr>(ChunkPtr + (WithinIndex * sizeof(uintptr)), EntryPtr))
            return false;

        if (EntryPtr == 0)
            return false;

        // Read FNameEntry header
        // Index field at 0x00, low bit indicates wide
        int32 IndexValue = 0;
        if (!Memory::Read<int32>(EntryPtr, IndexValue))
            return false;

        OutName.bIsWide = (IndexValue & 1) != 0;

        // Name data starts at offset 0x10
        uintptr NameDataAddr = EntryPtr + NameOffset;

        static char AnsiBuffer[1024];
        static wchar_t WideBuffer[1024];

        if (OutName.bIsWide)
        {
            for (int32 i = 0; i < 1023; ++i)
            {
                wchar_t Char = 0;
                if (!Memory::Read<wchar_t>(NameDataAddr + (i * sizeof(wchar_t)), Char))
                    break;

                WideBuffer[i] = Char;
                if (Char == 0)
                {
                    OutName.Length = i;
                    break;
                }
            }
            WideBuffer[1023] = 0;
            OutName.WideName = WideBuffer;
        }
        else
        {
            for (int32 i = 0; i < 1023; ++i)
            {
                char Char = 0;
                if (!Memory::Read<char>(NameDataAddr + i, Char))
                    break;

                AnsiBuffer[i] = Char;
                if (Char == 0)
                {
                    OutName.Length = i;
                    break;
                }
            }
            AnsiBuffer[1023] = 0;
            OutName.AnsiName = AnsiBuffer;
        }

        return true;
    }

    std::string FGNamesArray::GetNameString(int32 ComparisonIndex) const
    {
        FResolvedName Name;
        if (GetName(ComparisonIndex, Name))
            return Name.ToString();
        return "";
    }

    bool FGNamesArray::IsValidIndex(int32 Index) const
    {
        return m_bInitialized && Index >= 0 && Index < m_NumElements;
    }

    int32 FGNamesArray::Num() const
    {
        return m_NumElements;
    }

    bool FGNamesArray::IsInitialized() const
    {
        return m_bInitialized;
    }

    //=========================================================================
    // FNamePoolImpl Implementation (4.23+)
    //=========================================================================

    FNamePoolImpl::FNamePoolImpl()
        : m_BaseAddress(0)
        , m_NumBlocks(0)
        , m_bInitialized(false)
    {
    }

    EResult FNamePoolImpl::Initialize(uintptr Address)
    {
        if (m_bInitialized)
            return EResult::AlreadyInitialized;

        if (Address == 0)
            return EResult::Failed;

        m_BaseAddress = Address;

        // FNamePool layout:
        // FNameEntryAllocator Entries;
        // ...
        //
        // FNameEntryAllocator:
        // void* Lock;              // 0x00
        // uint32 CurrentBlock;     // 0x08
        // uint32 CurrentByteCursor;// 0x0C
        // void* Blocks[8192];      // 0x10

        uint32 CurrentBlock = 0;
        if (!Memory::Read<uint32>(m_BaseAddress + 0x08, CurrentBlock))
        {
            USS_ERROR("Failed to read CurrentBlock from FNamePool");
            return EResult::Failed;
        }

        m_NumBlocks = CurrentBlock + 1;

        USS_LOG("FNamePoolImpl initialized: %d blocks", m_NumBlocks);

        m_bInitialized = true;
        return EResult::Success;
    }

    bool FNamePoolImpl::GetName(int32 ComparisonIndex, FResolvedName& OutName) const
    {
        if (!m_bInitialized || ComparisonIndex < 0)
            return false;

        // Decode ComparisonIndex
        // BlockIndex = ComparisonIndex >> 16
        // NameOffset = (ComparisonIndex & 0xFFFF) * 2
        int32 BlockIndex = ComparisonIndex >> 16;
        int32 NameOffset = (ComparisonIndex & 0xFFFF) * 2;

        if (BlockIndex >= m_NumBlocks)
            return false;

        uintptr BlockPtr = 0;
        if (!Memory::Read<uintptr>(m_BaseAddress + BlocksOffset + (BlockIndex * sizeof(uintptr)), BlockPtr))
            return false;

        if (BlockPtr == 0)
            return false;

        uintptr EntryAddr = BlockPtr + NameOffset;

        // Read FNameEntryHeader (2 bytes)
        // bIsWide : 1
        // Len : 15
        uint16 Header = 0;
        if (!Memory::Read<uint16>(EntryAddr, Header))
            return false;

        OutName.bIsWide = (Header & 1) != 0;
        OutName.Length = Header >> 1;

        if (OutName.Length <= 0 || OutName.Length > 1023)
            return false;

        uintptr NameDataAddr = EntryAddr + sizeof(uint16);

        static char AnsiBuffer[1024];
        static wchar_t WideBuffer[1024];

        if (OutName.bIsWide)
        {
            for (int32 i = 0; i < OutName.Length && i < 1023; ++i)
            {
                wchar_t Char = 0;
                if (!Memory::Read<wchar_t>(NameDataAddr + (i * sizeof(wchar_t)), Char))
                    break;
                WideBuffer[i] = Char;
            }
            WideBuffer[OutName.Length] = 0;
            OutName.WideName = WideBuffer;
        }
        else
        {
            for (int32 i = 0; i < OutName.Length && i < 1023; ++i)
            {
                char Char = 0;
                if (!Memory::Read<char>(NameDataAddr + i, Char))
                    break;
                AnsiBuffer[i] = Char;
            }
            AnsiBuffer[OutName.Length] = 0;
            OutName.AnsiName = AnsiBuffer;
        }

        return true;
    }

    std::string FNamePoolImpl::GetNameString(int32 ComparisonIndex) const
    {
        FResolvedName Name;
        if (GetName(ComparisonIndex, Name))
            return Name.ToString();
        return "";
    }

    bool FNamePoolImpl::IsValidIndex(int32 Index) const
    {
        // Can't easily validate without scanning
        return m_bInitialized && Index >= 0;
    }

    int32 FNamePoolImpl::Num() const
    {
        // FNamePool doesn't track count directly
        // Would need to scan to count
        return -1;
    }

    bool FNamePoolImpl::IsInitialized() const
    {
        return m_bInitialized;
    }

    //=========================================================================
    // Factory Function
    //=========================================================================

    std::unique_ptr<INamePool> CreateNamePool()
    {
        const auto& Version = GetVersionResolver().GetVersionInfo();

        if (Version.bUseFNamePool)
        {
            USS_LOG("Creating FNamePoolImpl for UE %s",
                Version.GetEngineVersionString().c_str());
            return std::make_unique<FNamePoolImpl>();
        }
        else
        {
            USS_LOG("Creating FGNamesArray for UE %s",
                Version.GetEngineVersionString().c_str());
            return std::make_unique<FGNamesArray>();
        }
    }

}