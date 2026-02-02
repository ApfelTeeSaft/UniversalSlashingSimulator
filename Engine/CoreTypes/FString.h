/**
 * UniversalSlashingSimulator - FString Implementation
 *
 * Version-agnostic FString implementation for reading/writing Unreal Engine strings.
 * Supports UE4.16 through UE5.x.
 *
 * FString in UE is essentially TArray<TCHAR> where TCHAR is wchar_t on Windows.
 * The memory layout is consistent across all supported versions:
 *   - Data pointer (TCHAR*)
 *   - ArrayNum (int32) - number of elements including null terminator
 *   - ArrayMax (int32) - allocated capacity
 *
 * OWNERSHIP TRACKING:
 * To safely interop with engine functions that return FString by value, we use
 * the sign bit of m_Max to track memory ownership:
 *   - Positive m_Max: External memory (engine-allocated), DO NOT free
 *   - Negative m_Max: We own the memory, free with delete[]
 * This maintains ABI compatibility while preventing crashes from attempting
 * to delete[] memory allocated by UE's GMalloc.
 */

#pragma once

#include "../../Core/Common.h"
#include "../../Core/Memory/Memory.h"
#include <string>
#include <cstdlib>

namespace USS
{
    // Ownership flag stored in sign bit of m_Max
    constexpr int32 FSTRING_OWNED_FLAG = static_cast<int32>(0x80000000);

    /**
     * Read-only view of an FString in memory
     * Used for reading FString data returned by engine functions
     */
    class FStringView
    {
    public:
        FStringView() : m_Data(nullptr), m_Num(0), m_Max(0) {}

        FStringView(const wchar_t* Data, int32 Num, int32 Max)
            : m_Data(Data), m_Num(Num), m_Max(Max) {
        }

        // Read from memory address (where FString struct is located)
        static FStringView FromAddress(uintptr_t Address)
        {
            if (!Address)
                return FStringView();

            FStringView View;
            Memory::Read<const wchar_t*>(Address, View.m_Data);
            Memory::Read<int32>(Address + sizeof(void*), View.m_Num);
            Memory::Read<int32>(Address + sizeof(void*) + sizeof(int32), View.m_Max);
            return View;
        }

        bool IsValid() const { return m_Data != nullptr && m_Num > 0; }
        bool IsEmpty() const { return m_Num <= 1; }  // 1 = just null terminator

        int32 Len() const { return m_Num > 0 ? m_Num - 1 : 0; }

        const wchar_t* GetData() const { return m_Data; }

        std::wstring ToWString() const
        {
            if (!IsValid() || IsEmpty())
                return std::wstring();

            std::wstring Result;
            Result.reserve(Len());

            for (int32 i = 0; i < Len(); ++i)
            {
                wchar_t Ch = 0;
                Memory::Read<wchar_t>(reinterpret_cast<uintptr_t>(m_Data) + (i * sizeof(wchar_t)), Ch);
                if (Ch == 0) break;
                Result += Ch;
            }

            return Result;
        }

        // Convert to std::string (narrow, ASCII-safe)
        std::string ToString() const
        {
            if (!IsValid() || IsEmpty())
                return std::string();

            std::string Result;
            Result.reserve(Len());

            for (int32 i = 0; i < Len(); ++i)
            {
                wchar_t Ch = 0;
                Memory::Read<wchar_t>(reinterpret_cast<uintptr_t>(m_Data) + (i * sizeof(wchar_t)), Ch);
                if (Ch == 0) break;
                // Convert to narrow char (works for ASCII range)
                Result += static_cast<char>(Ch & 0xFF);
            }

            return Result;
        }

    private:
        const wchar_t* m_Data;
        int32 m_Num;
        int32 m_Max;
    };

    /**
     * FString structure layout (for direct memory access)
     * This matches the in-memory layout of FString across all UE4/5 versions
     */
    struct FStringData
    {
        wchar_t* Data;      // Pointer to TCHAR array
        int32 ArrayNum;     // Number of elements (including null terminator)
        int32 ArrayMax;     // Allocated capacity

        FStringView AsView() const
        {
            return FStringView(Data, ArrayNum, ArrayMax);
        }

        bool IsValid() const { return Data != nullptr && ArrayNum > 0; }

        std::string ToString() const
        {
            return AsView().ToString();
        }

        std::wstring ToWString() const
        {
            return AsView().ToWString();
        }
    };

    /**
     * FString wrapper class for creating and managing FString-compatible data
     * Can be passed to UE functions expecting FString parameters
     *
     * Supports both owned memory (allocated by us) and external memory
     * (allocated by UE engine). This prevents crashes when receiving FString
     * returns from engine functions, as we won't try to delete[] engine memory.
     *
     * Memory ownership is tracked via the sign bit of m_Max:
     *   - When we allocate: m_Max is negative (has FSTRING_OWNED_FLAG set)
     *   - When engine allocates: m_Max is positive (no flag)
     */
    class FString
    {
    public:
        FString() : m_Data(nullptr), m_Num(0), m_Max(0) {}

        explicit FString(const char* Str)
        {
            if (Str && *Str)
            {
                size_t Len = strlen(Str);
                Allocate(static_cast<int32>(Len + 1));

                for (size_t i = 0; i <= Len; ++i)
                {
                    m_Data[i] = static_cast<wchar_t>(Str[i]);
                }
                m_Num = static_cast<int32>(Len + 1);
            }
            else
            {
                m_Data = nullptr;
                m_Num = 0;
                m_Max = 0;
            }
        }

        explicit FString(const wchar_t* Str)
        {
            if (Str && *Str)
            {
                size_t Len = wcslen(Str);
                Allocate(static_cast<int32>(Len + 1));
                memcpy(m_Data, Str, (Len + 1) * sizeof(wchar_t));
                m_Num = static_cast<int32>(Len + 1);
            }
            else
            {
                m_Data = nullptr;
                m_Num = 0;
                m_Max = 0;
            }
        }

        explicit FString(const std::string& Str)
            : FString(Str.c_str())
        {
        }

        explicit FString(const std::wstring& Str)
            : FString(Str.c_str())
        {
        }

        // Copy constructor - always creates our own copy that we own
        FString(const FString& Other)
        {
            if (Other.m_Data && Other.m_Num > 0)
            {
                Allocate(Other.m_Num);
                memcpy(m_Data, Other.m_Data, Other.m_Num * sizeof(wchar_t));
                m_Num = Other.m_Num;
            }
            else
            {
                m_Data = nullptr;
                m_Num = 0;
                m_Max = 0;
            }
        }

        // Move constructor - transfers ownership status
        FString(FString&& Other) noexcept
            : m_Data(Other.m_Data)
            , m_Num(Other.m_Num)
            , m_Max(Other.m_Max)  // Preserves ownership flag
        {
            Other.m_Data = nullptr;
            Other.m_Num = 0;
            Other.m_Max = 0;
        }

        ~FString()
        {
            Free();
        }

        // Copy assignment - creates our own copy that we own
        FString& operator=(const FString& Other)
        {
            if (this != &Other)
            {
                Free();
                if (Other.m_Data && Other.m_Num > 0)
                {
                    Allocate(Other.m_Num);
                    memcpy(m_Data, Other.m_Data, Other.m_Num * sizeof(wchar_t));
                    m_Num = Other.m_Num;
                }
            }
            return *this;
        }

        // Move assignment - transfers ownership status
        FString& operator=(FString&& Other) noexcept
        {
            if (this != &Other)
            {
                Free();
                m_Data = Other.m_Data;
                m_Num = Other.m_Num;
                m_Max = Other.m_Max;  // Preserves ownership flag
                Other.m_Data = nullptr;
                Other.m_Num = 0;
                Other.m_Max = 0;
            }
            return *this;
        }

        bool IsEmpty() const { return m_Num <= 1; }
        bool IsValid() const { return m_Data != nullptr; }
        int32 Len() const { return m_Num > 0 ? m_Num - 1 : 0; }
        const wchar_t* GetData() const { return m_Data; }
        wchar_t* GetData() { return m_Data; }

        // Check if we own the memory (vs engine-allocated)
        bool OwnsMemory() const { return (m_Max & FSTRING_OWNED_FLAG) != 0; }

        // Get actual capacity (strips ownership flag)
        int32 Capacity() const { return m_Max & ~FSTRING_OWNED_FLAG; }

        FStringView AsView() const
        {
            return FStringView(m_Data, m_Num, Capacity());
        }

        std::string ToString() const
        {
            if (!IsValid() || IsEmpty())
                return std::string();

            std::string Result;
            Result.reserve(Len());

            // Direct memory access since we're in-process
            for (int32 i = 0; i < Len(); ++i)
            {
                wchar_t Ch = m_Data[i];
                if (Ch == 0) break;
                Result += static_cast<char>(Ch & 0xFF);
            }

            return Result;
        }

        std::wstring ToWString() const
        {
            if (!IsValid() || IsEmpty())
                return std::wstring();

            return std::wstring(m_Data, Len());
        }

        // Get pointer to the FString struct data (for passing to UE functions)
        // Returns pointer to this object's internal layout which matches FString
        void* GetStructPtr() { return &m_Data; }
        const void* GetStructPtr() const { return &m_Data; }

        /**
         * Release ownership of the memory without freeing it.
         * Use this when passing the string to engine functions that will take ownership,
         * or when you've copied the data and want to prevent double-free.
         */
        void ReleaseOwnership()
        {
            // Clear the ownership flag (make m_Max positive)
            m_Max &= ~FSTRING_OWNED_FLAG;
        }

        /**
         * Create a deep copy of this string that we own.
         * Useful when you receive an engine string and want to keep a copy.
         */
        FString Clone() const
        {
            FString Result;
            if (m_Data && m_Num > 0)
            {
                Result.Allocate(m_Num);
                memcpy(Result.m_Data, m_Data, m_Num * sizeof(wchar_t));
                Result.m_Num = m_Num;
            }
            return Result;
        }

    private:
        void Allocate(int32 Capacity)
        {
            Free();
            m_Data = new wchar_t[Capacity];
            // Set capacity with ownership flag (negative)
            m_Max = Capacity | FSTRING_OWNED_FLAG;
            m_Num = 0;
        }

        void Free()
        {
            // Only free if we own the memory (m_Max is negative / has ownership flag)
            if (m_Data && OwnsMemory())
            {
                delete[] m_Data;
            }
            m_Data = nullptr;
            m_Num = 0;
            m_Max = 0;
        }

        // Memory layout must match UE's FString (3 members, same sizes)
        wchar_t* m_Data;    // AllocatorInstance.Data
        int32 m_Num;        // ArrayNum
        int32 m_Max;        // ArrayMax (sign bit = ownership flag)
    };

    static_assert(sizeof(FString) == sizeof(void*) + sizeof(int32) * 2,
        "FString size mismatch - layout may differ from UE");

}