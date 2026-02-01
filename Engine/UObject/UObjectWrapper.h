/**
 * UniversalSlashingSimulator - UObject Wrapper
 *
 * Provides version-agnostic access to Unreal Engine objects.
 * Wraps raw pointers and provides safe access to object members
 * using the offset resolver.
 *
 * This is the foundation for all object manipulation in USS.
 */

#pragma once

#include "../../Core/Common.h"
#include "../CoreTypes/NamePool.h"
#include "../CoreTypes/OffsetResolver.h"

namespace USS
{
    // Forward declarations
    class UClassWrapper;
    class UStructWrapper;
    class UFunctionWrapper;

    // FName wrapper for version-agnostic name access
    class FNameWrapper
    {
    public:
        FNameWrapper() : m_ComparisonIndex(0), m_Number(0) {}
        FNameWrapper(int32 Index, int32 Number) : m_ComparisonIndex(Index), m_Number(Number) {}

        // Get the name string
        std::string GetName() const;

        // Get name with number suffix if non-zero
        std::string GetFullName() const;

        // Check if valid
        bool IsValid() const { return m_ComparisonIndex > 0; }

        // Comparison
        bool operator==(const FNameWrapper& Other) const
        {
            return m_ComparisonIndex == Other.m_ComparisonIndex &&
                   m_Number == Other.m_Number;
        }

        bool operator!=(const FNameWrapper& Other) const
        {
            return !(*this == Other);
        }

        // Raw access
        int32 GetComparisonIndex() const { return m_ComparisonIndex; }
        int32 GetNumber() const { return m_Number; }

    private:
        int32 m_ComparisonIndex;
        int32 m_Number;
    };

    // UObject wrapper for version-agnostic object access
    class UObjectWrapper
    {
    public:
        UObjectWrapper() : m_pObject(nullptr) {}
        explicit UObjectWrapper(void* Object) : m_pObject(Object) {}

        // Check if valid
        bool IsValid() const;
        explicit operator bool() const { return IsValid(); }

        // Get raw pointer
        void* GetRaw() const { return m_pObject; }

        // Get raw pointer as specific type
        template<typename T>
        T* GetAs() const { return static_cast<T*>(m_pObject); }

        // Object properties
        int32 GetObjectFlags() const;
        int32 GetInternalIndex() const;
        UClassWrapper GetClass() const;
        FNameWrapper GetFName() const;
        UObjectWrapper GetOuter() const;

        // Get object name as string
        std::string GetName() const;

        // Get full path name (Outer.Outer.Name)
        std::string GetFullName() const;

        // Get path name (/Path/To/Object)
        std::string GetPathName() const;

        // Get class name (name of the UClass)
        // NOTE: Named GetObjectClassName to avoid Windows macro conflict
        std::string GetObjectClassName() const;

        // Alias for GetObjectClassName (avoid Windows GetClassName macro)
        std::string GetClassNameStr() const { return GetObjectClassName(); }

        // Type checking
        bool IsA(const UClassWrapper& Class) const;
        bool IsA(const char* ClassName) const;

        // Comparison
        bool operator==(const UObjectWrapper& Other) const { return m_pObject == Other.m_pObject; }
        bool operator!=(const UObjectWrapper& Other) const { return m_pObject != Other.m_pObject; }

    protected:
        void* m_pObject;
    };

    // UClass wrapper
    class UClassWrapper : public UObjectWrapper
    {
    public:
        UClassWrapper() : UObjectWrapper() {}
        explicit UClassWrapper(void* Class) : UObjectWrapper(Class) {}

        // Get super class
        UClassWrapper GetSuperClass() const;

        // Get class default object
        UObjectWrapper GetDefaultObject() const;

        // Check if this class is a child of another
        bool IsChildOf(const UClassWrapper& Parent) const;
    };

    // UStruct wrapper for struct iteration
    class UStructWrapper : public UObjectWrapper
    {
    public:
        UStructWrapper() : UObjectWrapper() {}
        explicit UStructWrapper(void* Struct) : UObjectWrapper(Struct) {}

        // Get super struct
        UStructWrapper GetSuperStruct() const;

        // Get struct size
        int32 GetPropertiesSize() const;

        // Get minimum alignment
        int32 GetMinAlignment() const;

        // Property iteration (version-aware)
        // Pre-4.25: Iterates UProperty via Children
        // 4.25+: Iterates FProperty via ChildProperties
        class PropertyIterator
        {
        public:
            PropertyIterator(void* First, bool bIsFField);

            bool IsValid() const { return m_pCurrent != nullptr; }
            void Next();

            // Property info
            std::string GetName() const;
            int32 GetOffset() const;
            int32 GetElementSize() const;

            void* GetRaw() const { return m_pCurrent; }

        private:
            void* m_pCurrent;
            bool m_bIsFField;
        };

        PropertyIterator GetProperties() const;
    };

    // UFunction wrapper
    class UFunctionWrapper : public UStructWrapper
    {
    public:
        UFunctionWrapper() : UStructWrapper() {}
        explicit UFunctionWrapper(void* Function) : UStructWrapper(Function) {}

        // Get function flags
        uint32 GetFunctionFlags() const;

        // Get number of parameters
        uint8 GetNumParms() const;

        // Get parameters size
        uint16 GetParmsSize() const;

        // Get native function pointer
        void* GetNativeFunc() const;
    };

}