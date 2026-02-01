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
 */

#pragma once

#include "../../Core/Common.h"
#include "../../Core/Memory/Memory.h"
#include <string>

namespace USS
{
    /**
     * Read-only view of an FString in memory
     * Used for reading FString data returned by engine functions
     */
    class FStringView
    {
    public:
        FStringView() : m_Data(nullptr), m_Num(0), m_Max(0) {}

        FStringView(const wchar_t* Data, int32 Num, int32 Max)
            : m_Data(Data), m_Num(Num), m_Max(Max) {}

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
        {}

        explicit FString(const std::wstring& Str)
            : FString(Str.c_str())
        {}

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

        FString(FString&& Other) noexcept
            : m_Data(Other.m_Data)
            , m_Num(Other.m_Num)
            , m_Max(Other.m_Max)
        {
            Other.m_Data = nullptr;
            Other.m_Num = 0;
            Other.m_Max = 0;
        }

        ~FString()
        {
            Free();
        }

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

        FString& operator=(FString&& Other) noexcept
        {
            if (this != &Other)
            {
                Free();
                m_Data = Other.m_Data;
                m_Num = Other.m_Num;
                m_Max = Other.m_Max;
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

        FStringView AsView() const
        {
            return FStringView(m_Data, m_Num, m_Max);
        }

        std::string ToString() const
        {
            return AsView().ToString();
        }

        std::wstring ToWString() const
        {
            return AsView().ToWString();
        }

        // Get pointer to the FString struct data (for passing to UE functions)
        // Returns pointer to this object's internal layout which matches FString
        void* GetStructPtr() { return &m_Data; }
        const void* GetStructPtr() const { return &m_Data; }

    private:
        void Allocate(int32 Capacity)
        {
            Free();
            m_Data = new wchar_t[Capacity];
            m_Max = Capacity;
            m_Num = 0;
        }

        void Free()
        {
            if (m_Data)
            {
                delete[] m_Data;
                m_Data = nullptr;
            }
            m_Num = 0;
            m_Max = 0;
        }

        // Memory layout must match UE's FString
        wchar_t* m_Data;    // AllocatorInstance.Data
        int32 m_Num;        // ArrayNum
        int32 m_Max;        // ArrayMax
    };

    static_assert(sizeof(FString) == sizeof(void*) + sizeof(int32) * 2,
        "FString size mismatch - layout may differ from UE");

}