/**
 * UniversalSlashingSimulator - UObject Wrapper Implementation
 */

#include "UObjectWrapper.h"
#include "../../Core/Memory/Memory.h"
#include "../../Core/Versioning/VersionResolver.h"
#include "../CoreTypes/OffsetResolver.h"
#include <sstream>

namespace USS
{
    // Static name pool instance (lazy initialized)
    static std::unique_ptr<INamePool> g_pNamePool;

    static INamePool* GetNamePoolInstance()
    {
        if (!g_pNamePool)
        {
            g_pNamePool = CreateNamePool();
            // Note: Initialization happens separately via EngineCore
        }
        return g_pNamePool.get();
    }

    //=========================================================================
    // FNameWrapper Implementation
    //=========================================================================

    std::string FNameWrapper::GetName() const
    {
        if (m_ComparisonIndex <= 0)
            return "None";

        INamePool* Pool = GetNamePoolInstance();
        if (!Pool || !Pool->IsInitialized())
            return "<uninitialized>";

        return Pool->GetNameString(m_ComparisonIndex);
    }

    std::string FNameWrapper::GetFullName() const
    {
        std::string Name = GetName();

        if (m_Number > 0)
        {
            Name += "_";
            Name += std::to_string(m_Number);
        }

        return Name;
    }

    //=========================================================================
    // UObjectWrapper Implementation
    //=========================================================================

    bool UObjectWrapper::IsValid() const
    {
        return m_pObject != nullptr && Memory::IsValidAddress(reinterpret_cast<uintptr>(m_pObject));
    }

    int32 UObjectWrapper::GetObjectFlags() const
    {
        if (!IsValid())
            return 0;

        const auto& Offsets = GetOffsetResolver().GetOffsets();
        int32 Flags = 0;

        Memory::Read<int32>(
            reinterpret_cast<uintptr>(m_pObject) + Offsets.UObject.ObjectFlags,
            Flags
        );

        return Flags;
    }

    int32 UObjectWrapper::GetInternalIndex() const
    {
        if (!IsValid())
            return -1;

        const auto& Offsets = GetOffsetResolver().GetOffsets();
        int32 Index = -1;

        Memory::Read<int32>(
            reinterpret_cast<uintptr>(m_pObject) + Offsets.UObject.InternalIndex,
            Index
        );

        return Index;
    }

    UClassWrapper UObjectWrapper::GetClass() const
    {
        if (!IsValid())
            return UClassWrapper();

        const auto& Offsets = GetOffsetResolver().GetOffsets();
        void* ClassPtr = nullptr;

        Memory::Read<void*>(
            reinterpret_cast<uintptr>(m_pObject) + Offsets.UObject.Class,
            ClassPtr
        );

        return UClassWrapper(ClassPtr);
    }

    FNameWrapper UObjectWrapper::GetFName() const
    {
        if (!IsValid())
            return FNameWrapper();

        const auto& Offsets = GetOffsetResolver().GetOffsets();
        uintptr NameAddr = reinterpret_cast<uintptr>(m_pObject) + Offsets.UObject.Name;

        int32 ComparisonIndex = 0;
        int32 Number = 0;

        Memory::Read<int32>(NameAddr + 0, ComparisonIndex);
        Memory::Read<int32>(NameAddr + 4, Number);

        return FNameWrapper(ComparisonIndex, Number);
    }

    UObjectWrapper UObjectWrapper::GetOuter() const
    {
        if (!IsValid())
            return UObjectWrapper();

        const auto& Offsets = GetOffsetResolver().GetOffsets();
        void* OuterPtr = nullptr;

        Memory::Read<void*>(
            reinterpret_cast<uintptr>(m_pObject) + Offsets.UObject.Outer,
            OuterPtr
        );

        return UObjectWrapper(OuterPtr);
    }

    std::string UObjectWrapper::GetName() const
    {
        return GetFName().GetFullName();
    }

    std::string UObjectWrapper::GetFullName() const
    {
        if (!IsValid())
            return "";

        UClassWrapper Class = GetClass();
        std::string ClassName = Class.IsValid() ? Class.GetName() : "Unknown";

        return ClassName + " " + GetPathName();
    }

    std::string UObjectWrapper::GetPathName() const
    {
        if (!IsValid())
            return "";

        std::string Path;
        UObjectWrapper Current = GetOuter();

        // Build path from outermost to innermost
        std::vector<std::string> Parts;
        Parts.push_back(GetName());

        while (Current.IsValid())
        {
            Parts.push_back(Current.GetName());
            Current = Current.GetOuter();
        }

        // Reverse and join
        for (auto It = Parts.rbegin(); It != Parts.rend(); ++It)
        {
            if (!Path.empty())
                Path += ".";
            Path += *It;
        }

        return Path;
    }

    std::string UObjectWrapper::GetObjectClassName() const
    {
        if (!IsValid())
            return "";

        UClassWrapper Class = GetClass();
        if (!Class.IsValid())
            return "";

        return Class.GetName();
    }

    bool UObjectWrapper::IsA(const UClassWrapper& Class) const
    {
        if (!IsValid() || !Class.IsValid())
            return false;

        UClassWrapper MyClass = GetClass();
        return MyClass.IsChildOf(Class);
    }

    bool UObjectWrapper::IsA(const char* ClassName) const
    {
        if (!IsValid() || !ClassName)
            return false;

        UClassWrapper MyClass = GetClass();
        while (MyClass.IsValid())
        {
            if (MyClass.GetName() == ClassName)
                return true;
            MyClass = MyClass.GetSuperClass();
        }

        return false;
    }

    //=========================================================================
    // UClassWrapper Implementation
    //=========================================================================

    UClassWrapper UClassWrapper::GetSuperClass() const
    {
        if (!IsValid())
            return UClassWrapper();

        const auto& Offsets = GetOffsetResolver().GetOffsets();
        void* SuperPtr = nullptr;

        Memory::Read<void*>(
            reinterpret_cast<uintptr>(m_pObject) + Offsets.UStruct.SuperStruct,
            SuperPtr
        );

        return UClassWrapper(SuperPtr);
    }

    UObjectWrapper UClassWrapper::GetDefaultObject() const
    {
        if (!IsValid())
            return UObjectWrapper();

        const auto& Offsets = GetOffsetResolver().GetOffsets();
        void* CDOPtr = nullptr;

        Memory::Read<void*>(
            reinterpret_cast<uintptr>(m_pObject) + Offsets.UClass.ClassDefaultObject,
            CDOPtr
        );

        return UObjectWrapper(CDOPtr);
    }

    bool UClassWrapper::IsChildOf(const UClassWrapper& Parent) const
    {
        if (!IsValid() || !Parent.IsValid())
            return false;

        if (m_pObject == Parent.m_pObject)
            return true;

        UClassWrapper Super = GetSuperClass();
        while (Super.IsValid())
        {
            if (Super.m_pObject == Parent.m_pObject)
                return true;
            Super = Super.GetSuperClass();
        }

        return false;
    }

    //=========================================================================
    // UStructWrapper Implementation
    //=========================================================================

    UStructWrapper UStructWrapper::GetSuperStruct() const
    {
        if (!IsValid())
            return UStructWrapper();

        const auto& Offsets = GetOffsetResolver().GetOffsets();
        void* SuperPtr = nullptr;

        Memory::Read<void*>(
            reinterpret_cast<uintptr>(m_pObject) + Offsets.UStruct.SuperStruct,
            SuperPtr
        );

        return UStructWrapper(SuperPtr);
    }

    int32 UStructWrapper::GetPropertiesSize() const
    {
        if (!IsValid())
            return 0;

        const auto& Offsets = GetOffsetResolver().GetOffsets();
        int32 Size = 0;

        Memory::Read<int32>(
            reinterpret_cast<uintptr>(m_pObject) + Offsets.UStruct.PropertiesSize,
            Size
        );

        return Size;
    }

    int32 UStructWrapper::GetMinAlignment() const
    {
        if (!IsValid())
            return 1;

        const auto& Offsets = GetOffsetResolver().GetOffsets();
        int32 Alignment = 1;

        Memory::Read<int32>(
            reinterpret_cast<uintptr>(m_pObject) + Offsets.UStruct.MinAlignment,
            Alignment
        );

        return Alignment;
    }

    UStructWrapper::PropertyIterator UStructWrapper::GetProperties() const
    {
        if (!IsValid())
            return PropertyIterator(nullptr, false);

        const auto& Version = GetVersionResolver().GetVersionInfo();
        const auto& Offsets = GetOffsetResolver().GetOffsets();

        void* FirstProperty = nullptr;
        bool bIsFField = Version.bUseFField;

        if (bIsFField && Offsets.UStruct.ChildProperties != 0)
        {
            // UE 4.25+: Use ChildProperties (FField*)
            Memory::Read<void*>(
                reinterpret_cast<uintptr>(m_pObject) + Offsets.UStruct.ChildProperties,
                FirstProperty
            );
        }
        else
        {
            // Pre-4.25: Use Children (UField*), filter to properties
            Memory::Read<void*>(
                reinterpret_cast<uintptr>(m_pObject) + Offsets.UStruct.Children,
                FirstProperty
            );
        }

        return PropertyIterator(FirstProperty, bIsFField);
    }

    //=========================================================================
    // PropertyIterator Implementation
    //=========================================================================

    UStructWrapper::PropertyIterator::PropertyIterator(void* First, bool bIsFField)
        : m_pCurrent(First)
        , m_bIsFField(bIsFField)
    {
    }

    void UStructWrapper::PropertyIterator::Next()
    {
        if (!m_pCurrent)
            return;

        const auto& Offsets = GetOffsetResolver().GetOffsets();
        void* NextPtr = nullptr;

        if (m_bIsFField)
        {
            Memory::Read<void*>(
                reinterpret_cast<uintptr>(m_pCurrent) + Offsets.FField.Next,
                NextPtr
            );
        }
        else
        {
            Memory::Read<void*>(
                reinterpret_cast<uintptr>(m_pCurrent) + Offsets.UField.Next,
                NextPtr
            );
        }

        m_pCurrent = NextPtr;
    }

    std::string UStructWrapper::PropertyIterator::GetName() const
    {
        if (!m_pCurrent)
            return "";

        const auto& Offsets = GetOffsetResolver().GetOffsets();

        int32 ComparisonIndex = 0;
        int32 Number = 0;

        if (m_bIsFField)
        {
            uintptr NameAddr = reinterpret_cast<uintptr>(m_pCurrent) + Offsets.FField.NamePrivate;
            Memory::Read<int32>(NameAddr + 0, ComparisonIndex);
            Memory::Read<int32>(NameAddr + 4, Number);
        }
        else
        {
            // UProperty is a UObject, name at standard offset
            uintptr NameAddr = reinterpret_cast<uintptr>(m_pCurrent) + Offsets.UObject.Name;
            Memory::Read<int32>(NameAddr + 0, ComparisonIndex);
            Memory::Read<int32>(NameAddr + 4, Number);
        }

        return FNameWrapper(ComparisonIndex, Number).GetFullName();
    }

    int32 UStructWrapper::PropertyIterator::GetOffset() const
    {
        if (!m_pCurrent)
            return -1;

        const auto& Offsets = GetOffsetResolver().GetOffsets();
        int32 Offset = 0;

        if (m_bIsFField)
        {
            Memory::Read<int32>(
                reinterpret_cast<uintptr>(m_pCurrent) + Offsets.FProperty.Offset,
                Offset
            );
        }
        else
        {
            Memory::Read<int32>(
                reinterpret_cast<uintptr>(m_pCurrent) + Offsets.UProperty.Offset,
                Offset
            );
        }

        return Offset;
    }

    int32 UStructWrapper::PropertyIterator::GetElementSize() const
    {
        if (!m_pCurrent)
            return 0;

        const auto& Offsets = GetOffsetResolver().GetOffsets();
        int32 Size = 0;

        if (m_bIsFField)
        {
            Memory::Read<int32>(
                reinterpret_cast<uintptr>(m_pCurrent) + Offsets.FProperty.ElementSize,
                Size
            );
        }
        else
        {
            Memory::Read<int32>(
                reinterpret_cast<uintptr>(m_pCurrent) + Offsets.UProperty.ElementSize,
                Size
            );
        }

        return Size;
    }

    //=========================================================================
    // UFunctionWrapper Implementation
    //=========================================================================

    uint32 UFunctionWrapper::GetFunctionFlags() const
    {
        if (!IsValid())
            return 0;

        const auto& Offsets = GetOffsetResolver().GetOffsets();
        uint32 Flags = 0;

        Memory::Read<uint32>(
            reinterpret_cast<uintptr>(m_pObject) + Offsets.UFunction.FunctionFlags,
            Flags
        );

        return Flags;
    }

    uint8 UFunctionWrapper::GetNumParms() const
    {
        if (!IsValid())
            return 0;

        const auto& Offsets = GetOffsetResolver().GetOffsets();
        uint8 NumParms = 0;

        Memory::Read<uint8>(
            reinterpret_cast<uintptr>(m_pObject) + Offsets.UFunction.NumParms,
            NumParms
        );

        return NumParms;
    }

    uint16 UFunctionWrapper::GetParmsSize() const
    {
        if (!IsValid())
            return 0;

        const auto& Offsets = GetOffsetResolver().GetOffsets();
        uint16 Size = 0;

        Memory::Read<uint16>(
            reinterpret_cast<uintptr>(m_pObject) + Offsets.UFunction.ParmsSize,
            Size
        );

        return Size;
    }

    void* UFunctionWrapper::GetNativeFunc() const
    {
        if (!IsValid())
            return nullptr;

        const auto& Offsets = GetOffsetResolver().GetOffsets();
        void* Func = nullptr;

        Memory::Read<void*>(
            reinterpret_cast<uintptr>(m_pObject) + Offsets.UFunction.Func,
            Func
        );

        return Func;
    }

}