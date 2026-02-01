/**
 * UniversalSlashingSimulator - Name Pool Abstraction
 *
 * Provides a version-agnostic interface for accessing names (FName).
 * Implementations differ between:
 * - Pre-4.23: GNames array (TStaticIndirectArrayThreadSafeRead)
 * - 4.23+: FNamePool with packed entries
 */

#pragma once

#include "../../Core/Common.h"

namespace USS
{
    struct FResolvedName
    {
        const char* AnsiName;
        const wchar_t* WideName;
        bool bIsWide;
        int32 Length;

        FResolvedName()
            : AnsiName(nullptr)
            , WideName(nullptr)
            , bIsWide(false)
            , Length(0)
        {}

        std::string ToString() const
        {
            if (bIsWide && WideName)
            {
                std::string Result;
                for (int32 i = 0; i < Length && WideName[i]; ++i)
                {
                    Result += static_cast<char>(WideName[i]);
                }
                return Result;
            }
            else if (AnsiName)
            {
                return std::string(AnsiName, Length);
            }
            return "";
        }
    };

    struct FNameCompact
    {
        int32 ComparisonIndex;
        int32 Number;

        FNameCompact() : ComparisonIndex(0), Number(0) {}
        FNameCompact(int32 Index, int32 Num) : ComparisonIndex(Index), Number(Num) {}

        bool operator==(const FNameCompact& Other) const
        {
            return ComparisonIndex == Other.ComparisonIndex && Number == Other.Number;
        }
    };

    USS_INTERFACE INamePool
    {
    public:
        virtual ~INamePool() = default;

        // Get name by comparison index
        virtual bool GetName(int32 ComparisonIndex, FResolvedName& OutName) const = 0;

        // Get name as string (convenience)
        virtual std::string GetNameString(int32 ComparisonIndex) const = 0;

        // Check if index is valid
        virtual bool IsValidIndex(int32 Index) const = 0;

        // Get total number of names
        virtual int32 Num() const = 0;

        // Initialize from memory address
        virtual EResult Initialize(uintptr Address) = 0;

        // Check if initialized
        virtual bool IsInitialized() const = 0;
    };

    // GNames array implementation (Pre-4.23)
    class FGNamesArray : public INamePool
    {
    public:
        FGNamesArray();
        ~FGNamesArray() override = default;

        bool GetName(int32 ComparisonIndex, FResolvedName& OutName) const override;
        std::string GetNameString(int32 ComparisonIndex) const override;
        bool IsValidIndex(int32 Index) const override;
        int32 Num() const override;
        EResult Initialize(uintptr Address) override;
        bool IsInitialized() const override;

    private:
        // Internal layout:
        // Chunked indirect array with 0x4000 elements per chunk
        // Access: GNames.Objects[index / 0x4000][index % 0x4000]
        //
        // FNameEntry layout (pre-4.23):
        // int32 Index;           // 0x00 - Upper bits: index, LSB: wide flag
        // char  Pad[4];          // 0x04
        // FNameEntry* HashNext;  // 0x08
        // union {
        //     char AnsiName[1024];
        //     wchar_t WideName[1024];
        // };                     // 0x10

        static constexpr int32 ElementsPerChunk = 0x4000;  // 16384
        static constexpr int32 NameOffset = 0x10;  // Offset to name data

        uintptr m_BaseAddress;
        uintptr m_ChunksPtr;
        int32 m_NumElements;
        bool m_bInitialized;
    };

    // FNamePool implementation (4.23+)
    class FNamePoolImpl : public INamePool
    {
    public:
        FNamePoolImpl();
        ~FNamePoolImpl() override = default;

        bool GetName(int32 ComparisonIndex, FResolvedName& OutName) const override;
        std::string GetNameString(int32 ComparisonIndex) const override;
        bool IsValidIndex(int32 Index) const override;
        int32 Num() const override;
        EResult Initialize(uintptr Address) override;
        bool IsInitialized() const override;

    private:
        // FNamePool layout (4.23+):
        // struct FNameEntryAllocator {
        //     void* Lock;              // 0x00
        //     uint32 CurrentBlock;     // 0x08
        //     uint32 CurrentByteCursor;// 0x0C
        //     void* Blocks[8192];      // 0x10 - Block pointers
        // }
        //
        // ComparisonIndex encoding:
        // BlockIndex = ComparisonIndex >> 16
        // NameOffset = (ComparisonIndex & 0xFFFF) * 2
        //
        // FNameEntry layout (4.23+):
        // struct FNameEntryHeader {
        //     uint16 bIsWide : 1;
        //     uint16 Len : 15;
        // };
        // Followed by packed name data

        static constexpr int32 MaxBlocks = 8192;
        static constexpr uintptr BlocksOffset = 0x10;

        uintptr m_BaseAddress;
        int32 m_NumBlocks;
        bool m_bInitialized;
    };

    std::unique_ptr<INamePool> CreateNamePool();

}