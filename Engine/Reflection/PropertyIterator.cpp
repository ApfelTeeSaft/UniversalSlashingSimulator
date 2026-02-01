/**
 * UniversalSlashingSimulator - Property Iterator Implementation
 */

#include "PropertyIterator.h"
#include "../../Core/Memory/Memory.h"
#include "../../Core/Logging/Log.h"
#include "../../Core/Versioning/VersionResolver.h"
#include "../EngineCore.h"

namespace USS
{
    //=========================================================================
    // FPropertyInfo Methods
    //=========================================================================

    bool FPropertyInfo::IsEditable() const
    {
        return (PropertyFlags & EPropertyFlags::CPF_Edit) != 0;
    }

    bool FPropertyInfo::IsBlueprintVisible() const
    {
        return (PropertyFlags & EPropertyFlags::CPF_BlueprintVisible) != 0;
    }

    bool FPropertyInfo::IsNative() const
    {
        return true;
    }

    bool FPropertyInfo::IsReplicated() const
    {
        return (PropertyFlags & EPropertyFlags::CPF_Net) != 0;
    }

    bool FPropertyInfo::IsSaveGame() const
    {
        return (PropertyFlags & EPropertyFlags::CPF_SaveGame) != 0;
    }

    //=========================================================================
    // FUPropertyIterator Implementation (Pre-4.25)
    //=========================================================================

    FUPropertyIterator::FUPropertyIterator()
        : m_ChildrenOffset(0)
        , m_PropertyLinkOffset(0)
        , m_SuperStructOffset(0)
        , m_UProperty_ArrayDimOffset(0)
        , m_UProperty_ElementSizeOffset(0)
        , m_UProperty_PropertyFlagsOffset(0)
        , m_UProperty_OffsetOffset(0)
        , m_UProperty_NextOffset(0)
        , m_bInitialized(false)
    {
    }

    EResult FUPropertyIterator::Initialize()
    {
        if (m_bInitialized)
            return EResult::AlreadyInitialized;

        USS_LOG("Initializing UProperty iterator (Pre-4.25 mode)...");

        // Get offsets from resolver
        auto& Offsets = GetEngineCore().GetOffsetResolver();

        // UStruct offsets
        m_ChildrenOffset = Offsets.GetOffset("UStruct", "Children");
        m_PropertyLinkOffset = Offsets.GetOffset("UStruct", "PropertyLink");
        m_SuperStructOffset = Offsets.GetOffset("UStruct", "SuperStruct");

        // UProperty offsets (derived from UField which is derived from UObject)
        m_UProperty_NextOffset = Offsets.GetOffset("UField", "Next");
        m_UProperty_ArrayDimOffset = Offsets.GetOffset("UProperty", "ArrayDim");
        m_UProperty_ElementSizeOffset = Offsets.GetOffset("UProperty", "ElementSize");
        m_UProperty_PropertyFlagsOffset = Offsets.GetOffset("UProperty", "PropertyFlags");
        m_UProperty_OffsetOffset = Offsets.GetOffset("UProperty", "Offset_Internal");

        // Use defaults if not resolved
        if (m_ChildrenOffset == 0) m_ChildrenOffset = 0x48;      // Typical for UE4.19
        if (m_PropertyLinkOffset == 0) m_PropertyLinkOffset = 0x50;
        if (m_SuperStructOffset == 0) m_SuperStructOffset = 0x40;
        if (m_UProperty_NextOffset == 0) m_UProperty_NextOffset = 0x30;
        if (m_UProperty_ArrayDimOffset == 0) m_UProperty_ArrayDimOffset = 0x38;
        if (m_UProperty_ElementSizeOffset == 0) m_UProperty_ElementSizeOffset = 0x3C;
        if (m_UProperty_PropertyFlagsOffset == 0) m_UProperty_PropertyFlagsOffset = 0x40;
        if (m_UProperty_OffsetOffset == 0) m_UProperty_OffsetOffset = 0x4C;

        USS_LOG("UProperty iterator offsets:");
        USS_LOG("  UStruct::Children = 0x%X", m_ChildrenOffset);
        USS_LOG("  UStruct::PropertyLink = 0x%X", m_PropertyLinkOffset);
        USS_LOG("  UProperty::Next = 0x%X", m_UProperty_NextOffset);
        USS_LOG("  UProperty::ArrayDim = 0x%X", m_UProperty_ArrayDimOffset);
        USS_LOG("  UProperty::Offset = 0x%X", m_UProperty_OffsetOffset);

        m_bInitialized = true;
        return EResult::Success;
    }

    bool FUPropertyIterator::IsInitialized() const
    {
        return m_bInitialized;
    }

    void FUPropertyIterator::ForEachProperty(void* Struct, FPropertyCallback Callback, bool bIncludeSuper)
    {
        if (!m_bInitialized || !Struct || !Callback)
            return;

        // Use PropertyLink for optimized iteration
        void* Property = GetPropertyLink(Struct);

        while (Property)
        {
            FPropertyInfo Info;
            if (ReadPropertyInfo(Property, Info))
            {
                if (!Callback(Info))
                    return;  // Callback requested stop
            }

            Property = GetNextProperty(Property);
        }

        // Include super class properties if requested
        if (bIncludeSuper)
        {
            void* SuperStruct = nullptr;
            if (Memory::Read<void*>(reinterpret_cast<uintptr>(Struct) + m_SuperStructOffset, SuperStruct))
            {
                if (SuperStruct)
                {
                    ForEachProperty(SuperStruct, Callback, true);
                }
            }
        }
    }

    bool FUPropertyIterator::FindProperty(void* Struct, const char* PropertyName, FPropertyInfo& OutInfo)
    {
        if (!m_bInitialized || !Struct || !PropertyName)
            return false;

        bool bFound = false;
        std::string TargetName(PropertyName);

        ForEachProperty(Struct, [&](const FPropertyInfo& Info) -> bool {
            if (Info.Name == TargetName)
            {
                OutInfo = Info;
                bFound = true;
                return false;  // Stop iteration
            }
            return true;  // Continue
        }, true);

        return bFound;
    }

    bool FUPropertyIterator::FindPropertyByOffset(void* Struct, int32 Offset, FPropertyInfo& OutInfo)
    {
        if (!m_bInitialized || !Struct)
            return false;

        bool bFound = false;

        ForEachProperty(Struct, [&](const FPropertyInfo& Info) -> bool {
            if (Info.Offset == Offset)
            {
                OutInfo = Info;
                bFound = true;
                return false;
            }
            return true;
        }, true);

        return bFound;
    }

    int32 FUPropertyIterator::GetPropertyCount(void* Struct, bool bIncludeSuper)
    {
        if (!m_bInitialized || !Struct)
            return 0;

        int32 Count = 0;

        ForEachProperty(Struct, [&](const FPropertyInfo&) -> bool {
            ++Count;
            return true;
        }, bIncludeSuper);

        return Count;
    }

    void* FUPropertyIterator::GetPropertyListHead(void* Struct)
    {
        return GetPropertyLink(Struct);
    }

    bool FUPropertyIterator::ReadPropertyInfo(void* Property, FPropertyInfo& OutInfo)
    {
        if (!Property)
            return false;

        uintptr PropAddr = reinterpret_cast<uintptr>(Property);

        OutInfo.PropertyPtr = Property;

        // Get name from UObject (property is UObject-derived)
        OutInfo.Name = GetEngineCore().GetObjectName(Property);

        // Get class name
        OutInfo.ClassName = GetPropertyClassName(Property);
        OutInfo.Type = ClassifyProperty(OutInfo.ClassName);

        // Read property data
        Memory::Read<int32>(PropAddr + m_UProperty_ArrayDimOffset, OutInfo.ArrayDim);
        Memory::Read<int32>(PropAddr + m_UProperty_ElementSizeOffset, OutInfo.ElementSize);
        Memory::Read<uint64>(PropAddr + m_UProperty_PropertyFlagsOffset, OutInfo.PropertyFlags);
        Memory::Read<int32>(PropAddr + m_UProperty_OffsetOffset, OutInfo.Offset);

        return true;
    }

    void* FUPropertyIterator::GetPropertyLink(void* Struct)
    {
        if (!Struct)
            return nullptr;

        void* PropertyLink = nullptr;
        Memory::Read<void*>(reinterpret_cast<uintptr>(Struct) + m_PropertyLinkOffset, PropertyLink);
        return PropertyLink;
    }

    void* FUPropertyIterator::GetNextProperty(void* Property)
    {
        if (!Property)
            return nullptr;

        void* Next = nullptr;
        Memory::Read<void*>(reinterpret_cast<uintptr>(Property) + m_UProperty_NextOffset, Next);
        return Next;
    }

    std::string FUPropertyIterator::GetPropertyClassName(void* Property)
    {
        if (!Property)
            return "";

        // Get UClass* of the property
        void* Class = GetEngineCore().GetObjectClass(Property);
        if (Class)
        {
            return GetEngineCore().GetObjectName(Class);
        }
        return "";
    }

    EPropertyType FUPropertyIterator::ClassifyProperty(const std::string& ClassName)
    {
        if (ClassName == "ByteProperty") return EPropertyType::ByteProperty;
        if (ClassName == "Int8Property") return EPropertyType::Int8Property;
        if (ClassName == "Int16Property") return EPropertyType::Int16Property;
        if (ClassName == "IntProperty") return EPropertyType::IntProperty;
        if (ClassName == "Int64Property") return EPropertyType::Int64Property;
        if (ClassName == "UInt16Property") return EPropertyType::UInt16Property;
        if (ClassName == "UInt32Property") return EPropertyType::UInt32Property;
        if (ClassName == "UInt64Property") return EPropertyType::UInt64Property;
        if (ClassName == "FloatProperty") return EPropertyType::FloatProperty;
        if (ClassName == "DoubleProperty") return EPropertyType::DoubleProperty;
        if (ClassName == "BoolProperty") return EPropertyType::BoolProperty;
        if (ClassName == "StrProperty") return EPropertyType::StrProperty;
        if (ClassName == "NameProperty") return EPropertyType::NameProperty;
        if (ClassName == "TextProperty") return EPropertyType::TextProperty;
        if (ClassName == "ObjectProperty") return EPropertyType::ObjectProperty;
        if (ClassName == "ClassProperty") return EPropertyType::ClassProperty;
        if (ClassName == "InterfaceProperty") return EPropertyType::InterfaceProperty;
        if (ClassName == "WeakObjectProperty") return EPropertyType::WeakObjectProperty;
        if (ClassName == "LazyObjectProperty") return EPropertyType::LazyObjectProperty;
        if (ClassName == "SoftObjectProperty") return EPropertyType::SoftObjectProperty;
        if (ClassName == "SoftClassProperty") return EPropertyType::SoftClassProperty;
        if (ClassName == "StructProperty") return EPropertyType::StructProperty;
        if (ClassName == "ArrayProperty") return EPropertyType::ArrayProperty;
        if (ClassName == "MapProperty") return EPropertyType::MapProperty;
        if (ClassName == "SetProperty") return EPropertyType::SetProperty;
        if (ClassName == "DelegateProperty") return EPropertyType::DelegateProperty;
        if (ClassName == "MulticastDelegateProperty") return EPropertyType::MulticastDelegateProperty;
        if (ClassName == "MulticastInlineDelegateProperty") return EPropertyType::MulticastInlineDelegateProperty;
        if (ClassName == "MulticastSparseDelegateProperty") return EPropertyType::MulticastSparseDelegateProperty;
        if (ClassName == "EnumProperty") return EPropertyType::EnumProperty;
        if (ClassName == "FieldPathProperty") return EPropertyType::FieldPathProperty;
        return EPropertyType::Unknown;
    }

    //=========================================================================
    // FFFieldPropertyIterator Implementation (4.25+)
    //=========================================================================

    FFFieldPropertyIterator::FFFieldPropertyIterator()
        : m_ChildPropertiesOffset(0)
        , m_SuperStructOffset(0)
        , m_FField_ClassOffset(0)
        , m_FField_OwnerOffset(0)
        , m_FField_NextOffset(0)
        , m_FField_NameOffset(0)
        , m_FProperty_ArrayDimOffset(0)
        , m_FProperty_ElementSizeOffset(0)
        , m_FProperty_PropertyFlagsOffset(0)
        , m_FProperty_OffsetOffset(0)
        , m_FFieldClass_NameOffset(0)
        , m_bInitialized(false)
    {
    }

    EResult FFFieldPropertyIterator::Initialize()
    {
        if (m_bInitialized)
            return EResult::AlreadyInitialized;

        USS_LOG("Initializing FField property iterator (4.25+ mode)...");

        // Get offsets from resolver
        auto& Offsets = GetEngineCore().GetOffsetResolver();

        // UStruct offsets (ChildProperties is new in 4.25)
        m_ChildPropertiesOffset = Offsets.GetOffset("UStruct", "ChildProperties");
        m_SuperStructOffset = Offsets.GetOffset("UStruct", "SuperStruct");

        // FField offsets
        m_FField_ClassOffset = Offsets.GetOffset("FField", "ClassPrivate");
        m_FField_OwnerOffset = Offsets.GetOffset("FField", "Owner");
        m_FField_NextOffset = Offsets.GetOffset("FField", "Next");
        m_FField_NameOffset = Offsets.GetOffset("FField", "NamePrivate");

        // FProperty offsets
        m_FProperty_ArrayDimOffset = Offsets.GetOffset("FProperty", "ArrayDim");
        m_FProperty_ElementSizeOffset = Offsets.GetOffset("FProperty", "ElementSize");
        m_FProperty_PropertyFlagsOffset = Offsets.GetOffset("FProperty", "PropertyFlags");
        m_FProperty_OffsetOffset = Offsets.GetOffset("FProperty", "Offset_Internal");

        // FFieldClass offset
        m_FFieldClass_NameOffset = Offsets.GetOffset("FFieldClass", "Name");

        // Use defaults if not resolved (typical for FN Chapter 2+)
        if (m_ChildPropertiesOffset == 0) m_ChildPropertiesOffset = 0x50;
        if (m_SuperStructOffset == 0) m_SuperStructOffset = 0x40;
        if (m_FField_ClassOffset == 0) m_FField_ClassOffset = 0x00;
        if (m_FField_OwnerOffset == 0) m_FField_OwnerOffset = 0x08;
        if (m_FField_NextOffset == 0) m_FField_NextOffset = 0x20;
        if (m_FField_NameOffset == 0) m_FField_NameOffset = 0x28;
        if (m_FProperty_ArrayDimOffset == 0) m_FProperty_ArrayDimOffset = 0x38;
        if (m_FProperty_ElementSizeOffset == 0) m_FProperty_ElementSizeOffset = 0x3C;
        if (m_FProperty_PropertyFlagsOffset == 0) m_FProperty_PropertyFlagsOffset = 0x40;
        if (m_FProperty_OffsetOffset == 0) m_FProperty_OffsetOffset = 0x4C;
        if (m_FFieldClass_NameOffset == 0) m_FFieldClass_NameOffset = 0x00;

        USS_LOG("FField property iterator offsets:");
        USS_LOG("  UStruct::ChildProperties = 0x%X", m_ChildPropertiesOffset);
        USS_LOG("  FField::Next = 0x%X", m_FField_NextOffset);
        USS_LOG("  FField::NamePrivate = 0x%X", m_FField_NameOffset);
        USS_LOG("  FProperty::Offset = 0x%X", m_FProperty_OffsetOffset);

        m_bInitialized = true;
        return EResult::Success;
    }

    bool FFFieldPropertyIterator::IsInitialized() const
    {
        return m_bInitialized;
    }

    void FFFieldPropertyIterator::ForEachProperty(void* Struct, FPropertyCallback Callback, bool bIncludeSuper)
    {
        if (!m_bInitialized || !Struct || !Callback)
            return;

        // Iterate ChildProperties chain (FField*)
        void* Field = GetChildProperties(Struct);

        while (Field)
        {
            FPropertyInfo Info;
            if (ReadPropertyInfo(Field, Info))
            {
                if (!Callback(Info))
                    return;
            }

            Field = GetNextField(Field);
        }

        // Include super class properties if requested
        if (bIncludeSuper)
        {
            void* SuperStruct = nullptr;
            if (Memory::Read<void*>(reinterpret_cast<uintptr>(Struct) + m_SuperStructOffset, SuperStruct))
            {
                if (SuperStruct)
                {
                    ForEachProperty(SuperStruct, Callback, true);
                }
            }
        }
    }

    bool FFFieldPropertyIterator::FindProperty(void* Struct, const char* PropertyName, FPropertyInfo& OutInfo)
    {
        if (!m_bInitialized || !Struct || !PropertyName)
            return false;

        bool bFound = false;
        std::string TargetName(PropertyName);

        ForEachProperty(Struct, [&](const FPropertyInfo& Info) -> bool {
            if (Info.Name == TargetName)
            {
                OutInfo = Info;
                bFound = true;
                return false;
            }
            return true;
        }, true);

        return bFound;
    }

    bool FFFieldPropertyIterator::FindPropertyByOffset(void* Struct, int32 Offset, FPropertyInfo& OutInfo)
    {
        if (!m_bInitialized || !Struct)
            return false;

        bool bFound = false;

        ForEachProperty(Struct, [&](const FPropertyInfo& Info) -> bool {
            if (Info.Offset == Offset)
            {
                OutInfo = Info;
                bFound = true;
                return false;
            }
            return true;
        }, true);

        return bFound;
    }

    int32 FFFieldPropertyIterator::GetPropertyCount(void* Struct, bool bIncludeSuper)
    {
        if (!m_bInitialized || !Struct)
            return 0;

        int32 Count = 0;

        ForEachProperty(Struct, [&](const FPropertyInfo&) -> bool {
            ++Count;
            return true;
        }, bIncludeSuper);

        return Count;
    }

    void* FFFieldPropertyIterator::GetPropertyListHead(void* Struct)
    {
        return GetChildProperties(Struct);
    }

    bool FFFieldPropertyIterator::ReadPropertyInfo(void* Field, FPropertyInfo& OutInfo)
    {
        if (!Field)
            return false;

        uintptr FieldAddr = reinterpret_cast<uintptr>(Field);

        OutInfo.PropertyPtr = Field;

        // Get name from FField (stored as FName)
        // FName is at m_FField_NameOffset
        int32 NameIndex = 0;
        if (Memory::Read<int32>(FieldAddr + m_FField_NameOffset, NameIndex))
        {
            OutInfo.Name = GetEngineCore().GetNameFromIndex(NameIndex);
        }

        // Get class name from FFieldClass
        OutInfo.ClassName = GetFieldClassName(Field);
        OutInfo.Type = ClassifyProperty(OutInfo.ClassName);

        // Read property data
        Memory::Read<int32>(FieldAddr + m_FProperty_ArrayDimOffset, OutInfo.ArrayDim);
        Memory::Read<int32>(FieldAddr + m_FProperty_ElementSizeOffset, OutInfo.ElementSize);
        Memory::Read<uint64>(FieldAddr + m_FProperty_PropertyFlagsOffset, OutInfo.PropertyFlags);

        // Note: In 4.25+, Offset_Internal might be uint16 instead of int32
        // We read as int32 but only lower 16 bits may be valid
        int32 RawOffset = 0;
        Memory::Read<int32>(FieldAddr + m_FProperty_OffsetOffset, RawOffset);
        OutInfo.Offset = RawOffset;

        return true;
    }

    void* FFFieldPropertyIterator::GetChildProperties(void* Struct)
    {
        if (!Struct)
            return nullptr;

        void* ChildProperties = nullptr;
        Memory::Read<void*>(reinterpret_cast<uintptr>(Struct) + m_ChildPropertiesOffset, ChildProperties);
        return ChildProperties;
    }

    void* FFFieldPropertyIterator::GetNextField(void* Field)
    {
        if (!Field)
            return nullptr;

        void* Next = nullptr;
        Memory::Read<void*>(reinterpret_cast<uintptr>(Field) + m_FField_NextOffset, Next);
        return Next;
    }

    std::string FFFieldPropertyIterator::GetFieldClassName(void* Field)
    {
        if (!Field)
            return "";

        uintptr FieldAddr = reinterpret_cast<uintptr>(Field);

        // Get FFieldClass* from Field
        void* FieldClass = nullptr;
        if (!Memory::Read<void*>(FieldAddr + m_FField_ClassOffset, FieldClass))
            return "";

        if (!FieldClass)
            return "";

        // FFieldClass has Name as FName at offset 0
        int32 NameIndex = 0;
        if (Memory::Read<int32>(reinterpret_cast<uintptr>(FieldClass) + m_FFieldClass_NameOffset, NameIndex))
        {
            return GetEngineCore().GetNameFromIndex(NameIndex);
        }

        return "";
    }

    EPropertyType FFFieldPropertyIterator::ClassifyProperty(const std::string& ClassName)
    {
        // Same classification as UProperty version
        if (ClassName == "ByteProperty") return EPropertyType::ByteProperty;
        if (ClassName == "Int8Property") return EPropertyType::Int8Property;
        if (ClassName == "Int16Property") return EPropertyType::Int16Property;
        if (ClassName == "IntProperty") return EPropertyType::IntProperty;
        if (ClassName == "Int64Property") return EPropertyType::Int64Property;
        if (ClassName == "UInt16Property") return EPropertyType::UInt16Property;
        if (ClassName == "UInt32Property") return EPropertyType::UInt32Property;
        if (ClassName == "UInt64Property") return EPropertyType::UInt64Property;
        if (ClassName == "FloatProperty") return EPropertyType::FloatProperty;
        if (ClassName == "DoubleProperty") return EPropertyType::DoubleProperty;
        if (ClassName == "BoolProperty") return EPropertyType::BoolProperty;
        if (ClassName == "StrProperty") return EPropertyType::StrProperty;
        if (ClassName == "NameProperty") return EPropertyType::NameProperty;
        if (ClassName == "TextProperty") return EPropertyType::TextProperty;
        if (ClassName == "ObjectProperty") return EPropertyType::ObjectProperty;
        if (ClassName == "ClassProperty") return EPropertyType::ClassProperty;
        if (ClassName == "InterfaceProperty") return EPropertyType::InterfaceProperty;
        if (ClassName == "WeakObjectProperty") return EPropertyType::WeakObjectProperty;
        if (ClassName == "LazyObjectProperty") return EPropertyType::LazyObjectProperty;
        if (ClassName == "SoftObjectProperty") return EPropertyType::SoftObjectProperty;
        if (ClassName == "SoftClassProperty") return EPropertyType::SoftClassProperty;
        if (ClassName == "StructProperty") return EPropertyType::StructProperty;
        if (ClassName == "ArrayProperty") return EPropertyType::ArrayProperty;
        if (ClassName == "MapProperty") return EPropertyType::MapProperty;
        if (ClassName == "SetProperty") return EPropertyType::SetProperty;
        if (ClassName == "DelegateProperty") return EPropertyType::DelegateProperty;
        if (ClassName == "MulticastDelegateProperty") return EPropertyType::MulticastDelegateProperty;
        if (ClassName == "MulticastInlineDelegateProperty") return EPropertyType::MulticastInlineDelegateProperty;
        if (ClassName == "MulticastSparseDelegateProperty") return EPropertyType::MulticastSparseDelegateProperty;
        if (ClassName == "EnumProperty") return EPropertyType::EnumProperty;
        if (ClassName == "FieldPathProperty") return EPropertyType::FieldPathProperty;
        return EPropertyType::Unknown;
    }

    //=========================================================================
    // Factory & Global Accessor
    //=========================================================================

    static std::unique_ptr<IPropertyIterator> g_PropertyIterator;

    std::unique_ptr<IPropertyIterator> CreatePropertyIterator()
    {
        const auto& Version = GetVersionResolver().GetVersionInfo();

        if (Version.bUseFField)
        {
            USS_LOG("Creating FFFieldPropertyIterator for UE %s",
                Version.GetEngineVersionString().c_str());
            return std::make_unique<FFFieldPropertyIterator>();
        }
        else
        {
            USS_LOG("Creating FUPropertyIterator for UE %s",
                Version.GetEngineVersionString().c_str());
            return std::make_unique<FUPropertyIterator>();
        }
    }

    IPropertyIterator& GetPropertyIterator()
    {
        if (!g_PropertyIterator)
        {
            g_PropertyIterator = CreatePropertyIterator();
            g_PropertyIterator->Initialize();
        }
        return *g_PropertyIterator;
    }

}