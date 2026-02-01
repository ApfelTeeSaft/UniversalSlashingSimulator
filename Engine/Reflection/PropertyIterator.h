/**
 * UniversalSlashingSimulator - Property Iterator Abstraction
 *
 * Provides version-agnostic iteration over class properties.
 * Handles the UProperty vs FField/FProperty difference:
 * - Pre-4.25: UProperty is UObject-derived, linked via PropertyLink
 * - 4.25+: FField/FProperty is separate hierarchy, linked via Next
 *
 * This is critical for reflection and finding struct offsets.
 */

#pragma once

#include "../../Core/Common.h"
#include "../../Core/Versioning/VersionInfo.h"
#include <string>
#include <functional>

namespace USS
{
    /**
     * Property type categories
     */
    enum class EPropertyType : uint8
    {
        Unknown = 0,

        // Numeric types
        ByteProperty,
        Int8Property,
        Int16Property,
        IntProperty,
        Int64Property,
        UInt16Property,
        UInt32Property,
        UInt64Property,
        FloatProperty,
        DoubleProperty,

        // Boolean
        BoolProperty,

        // String types
        StrProperty,
        NameProperty,
        TextProperty,

        // Object references
        ObjectProperty,
        ClassProperty,
        InterfaceProperty,
        WeakObjectProperty,
        LazyObjectProperty,
        SoftObjectProperty,
        SoftClassProperty,

        // Structs
        StructProperty,

        // Containers
        ArrayProperty,
        MapProperty,
        SetProperty,

        // Delegates
        DelegateProperty,
        MulticastDelegateProperty,
        MulticastInlineDelegateProperty,
        MulticastSparseDelegateProperty,

        // Enum
        EnumProperty,

        // Special
        FieldPathProperty,
    };

    /**
     * Resolved property information
     */
    struct FPropertyInfo
    {
        std::string Name;
        std::string ClassName;          // Property class name (e.g., "IntProperty")
        EPropertyType Type;

        int32 Offset;                   // Offset within struct/class
        int32 ElementSize;              // Size of single element
        int32 ArrayDim;                 // Static array dimension (usually 1)

        uint64 PropertyFlags;           // EPropertyFlags

        void* PropertyPtr;              // Native pointer (UProperty* or FProperty*)
        void* OwnerStruct;              // Owning UStruct*

        // For struct properties
        void* InnerStruct;              // UScriptStruct* for StructProperty

        // For array properties
        void* InnerProperty;            // Inner property for arrays

        // For object properties
        void* PropertyClass;            // UClass* for object references

        FPropertyInfo()
            : Type(EPropertyType::Unknown)
            , Offset(0)
            , ElementSize(0)
            , ArrayDim(1)
            , PropertyFlags(0)
            , PropertyPtr(nullptr)
            , OwnerStruct(nullptr)
            , InnerStruct(nullptr)
            , InnerProperty(nullptr)
            , PropertyClass(nullptr)
        {}

        bool IsValid() const { return PropertyPtr != nullptr; }

        // Common property flag checks
        bool IsEditable() const;
        bool IsBlueprintVisible() const;
        bool IsNative() const;
        bool IsReplicated() const;
        bool IsSaveGame() const;
    };

    /**
     * Property flags (common subset across versions)
     */
    namespace EPropertyFlags
    {
        constexpr uint64 CPF_Edit              = 0x0000000000000001;
        constexpr uint64 CPF_ConstParm         = 0x0000000000000002;
        constexpr uint64 CPF_BlueprintVisible  = 0x0000000000000004;
        constexpr uint64 CPF_ExportObject      = 0x0000000000000008;
        constexpr uint64 CPF_BlueprintReadOnly = 0x0000000000000010;
        constexpr uint64 CPF_Net               = 0x0000000000000020;
        constexpr uint64 CPF_EditFixedSize     = 0x0000000000000040;
        constexpr uint64 CPF_Parm              = 0x0000000000000080;
        constexpr uint64 CPF_OutParm           = 0x0000000000000100;
        constexpr uint64 CPF_ZeroConstructor   = 0x0000000000000200;
        constexpr uint64 CPF_ReturnParm        = 0x0000000000000400;
        constexpr uint64 CPF_DisableEditOnTemplate = 0x0000000000000800;
        constexpr uint64 CPF_Transient         = 0x0000000000002000;
        constexpr uint64 CPF_Config            = 0x0000000000004000;
        constexpr uint64 CPF_DisableEditOnInstance = 0x0000000000010000;
        constexpr uint64 CPF_EditConst         = 0x0000000000020000;
        constexpr uint64 CPF_GlobalConfig      = 0x0000000000040000;
        constexpr uint64 CPF_InstancedReference = 0x0000000000080000;
        constexpr uint64 CPF_DuplicateTransient = 0x0000000000200000;
        constexpr uint64 CPF_SaveGame          = 0x0000000001000000;
        constexpr uint64 CPF_NoClear           = 0x0000000002000000;
        constexpr uint64 CPF_ReferenceParm     = 0x0000000008000000;
        constexpr uint64 CPF_BlueprintAssignable = 0x0000000010000000;
        constexpr uint64 CPF_Deprecated        = 0x0000000020000000;
        constexpr uint64 CPF_RepNotify         = 0x0000000100000000;
        constexpr uint64 CPF_Interp            = 0x0000000200000000;
        constexpr uint64 CPF_NonTransactional  = 0x0000000400000000;
        constexpr uint64 CPF_EditorOnly        = 0x0000000800000000;
        constexpr uint64 CPF_NoDestructor      = 0x0000001000000000;
        constexpr uint64 CPF_AutoWeak          = 0x0000004000000000;
        constexpr uint64 CPF_ContainsInstancedReference = 0x0000008000000000;
        constexpr uint64 CPF_AssetRegistrySearchable = 0x0000010000000000;
        constexpr uint64 CPF_SimpleDisplay     = 0x0000020000000000;
        constexpr uint64 CPF_AdvancedDisplay   = 0x0000040000000000;
        constexpr uint64 CPF_Protected         = 0x0000080000000000;
        constexpr uint64 CPF_BlueprintCallable = 0x0000100000000000;
        constexpr uint64 CPF_BlueprintAuthorityOnly = 0x0000200000000000;
        constexpr uint64 CPF_TextExportTransient = 0x0000400000000000;
        constexpr uint64 CPF_NonPIEDuplicateTransient = 0x0000800000000000;
        constexpr uint64 CPF_ExposeOnSpawn     = 0x0001000000000000;
        constexpr uint64 CPF_PersistentInstance = 0x0002000000000000;
        constexpr uint64 CPF_UObjectWrapper    = 0x0004000000000000;
        constexpr uint64 CPF_HasGetValueTypeHash = 0x0008000000000000;
        constexpr uint64 CPF_NativeAccessSpecifierPublic = 0x0010000000000000;
        constexpr uint64 CPF_NativeAccessSpecifierProtected = 0x0020000000000000;
        constexpr uint64 CPF_NativeAccessSpecifierPrivate = 0x0040000000000000;
        constexpr uint64 CPF_SkipSerialization = 0x0080000000000000;
    }

    /**
     * Property iterator callback
     */
    using FPropertyCallback = std::function<bool(const FPropertyInfo&)>;

    /**
     * Property iterator interface
     */
    USS_INTERFACE IPropertyIterator
    {
    public:
        virtual ~IPropertyIterator() = default;

        /**
         * Iterate all properties of a UStruct
         * @param Struct - UStruct* to iterate
         * @param Callback - Called for each property, return false to stop
         * @param bIncludeSuper - Include inherited properties
         */
        virtual void ForEachProperty(void* Struct, FPropertyCallback Callback, bool bIncludeSuper = true) = 0;

        /**
         * Find property by name
         * @param Struct - UStruct* to search
         * @param PropertyName - Name to find
         * @param OutInfo - Output property info
         * @return true if found
         */
        virtual bool FindProperty(void* Struct, const char* PropertyName, FPropertyInfo& OutInfo) = 0;

        /**
         * Find property by offset
         * @param Struct - UStruct* to search
         * @param Offset - Offset to find
         * @param OutInfo - Output property info
         * @return true if found
         */
        virtual bool FindPropertyByOffset(void* Struct, int32 Offset, FPropertyInfo& OutInfo) = 0;

        /**
         * Get property count
         */
        virtual int32 GetPropertyCount(void* Struct, bool bIncludeSuper = true) = 0;

        /**
         * Get children/property list head
         */
        virtual void* GetPropertyListHead(void* Struct) = 0;

        /**
         * Initialize the iterator
         */
        virtual EResult Initialize() = 0;

        /**
         * Check if initialized
         */
        virtual bool IsInitialized() const = 0;
    };

    /**
     * UProperty-based iterator (Pre-4.25)
     *
     * In this version, properties are UObject-derived:
     * UProperty : UField : UObject
     *
     * UStruct layout (relevant parts):
     * - Children : UField* (first child field)
     * - PropertyLink : UProperty* (optimized property iteration)
     *
     * UProperty layout:
     * - UField base (includes UObject + Next pointer)
     * - ArrayDim : int32
     * - ElementSize : int32
     * - PropertyFlags : uint64
     * - Offset_Internal : int32
     * - ...
     */
    class FUPropertyIterator : public IPropertyIterator
    {
    public:
        FUPropertyIterator();
        ~FUPropertyIterator() override = default;

        void ForEachProperty(void* Struct, FPropertyCallback Callback, bool bIncludeSuper = true) override;
        bool FindProperty(void* Struct, const char* PropertyName, FPropertyInfo& OutInfo) override;
        bool FindPropertyByOffset(void* Struct, int32 Offset, FPropertyInfo& OutInfo) override;
        int32 GetPropertyCount(void* Struct, bool bIncludeSuper = true) override;
        void* GetPropertyListHead(void* Struct) override;
        EResult Initialize() override;
        bool IsInitialized() const override;

    private:
        bool ReadPropertyInfo(void* Property, FPropertyInfo& OutInfo);
        void* GetPropertyLink(void* Struct);
        void* GetNextProperty(void* Property);
        std::string GetPropertyClassName(void* Property);
        EPropertyType ClassifyProperty(const std::string& ClassName);

        // Offsets within UStruct
        int32 m_ChildrenOffset;         // Offset to Children field
        int32 m_PropertyLinkOffset;     // Offset to PropertyLink field
        int32 m_SuperStructOffset;      // Offset to SuperStruct field

        // Offsets within UProperty
        int32 m_UProperty_ArrayDimOffset;
        int32 m_UProperty_ElementSizeOffset;
        int32 m_UProperty_PropertyFlagsOffset;
        int32 m_UProperty_OffsetOffset;
        int32 m_UProperty_NextOffset;   // UField::Next

        bool m_bInitialized;
    };

    /**
     * FField/FProperty-based iterator (4.25+)
     *
     * In this version, properties use a separate hierarchy:
     * FProperty : FField (NOT UObject-derived)
     *
     * UStruct layout (relevant parts):
     * - ChildProperties : FField* (new field chain)
     * - Children : UField* (still exists for functions, etc.)
     *
     * FField layout:
     * - ClassPrivate : FFieldClass*
     * - Owner : FFieldVariant
     * - Next : FField*
     * - NamePrivate : FName
     * - FlagsPrivate : EObjectFlags
     *
     * FProperty layout:
     * - FField base
     * - ArrayDim : int32
     * - ElementSize : int32
     * - PropertyFlags : EPropertyFlags
     * - Offset_Internal : uint16 (changed from int32!)
     * - ...
     */
    class FFFieldPropertyIterator : public IPropertyIterator
    {
    public:
        FFFieldPropertyIterator();
        ~FFFieldPropertyIterator() override = default;

        void ForEachProperty(void* Struct, FPropertyCallback Callback, bool bIncludeSuper = true) override;
        bool FindProperty(void* Struct, const char* PropertyName, FPropertyInfo& OutInfo) override;
        bool FindPropertyByOffset(void* Struct, int32 Offset, FPropertyInfo& OutInfo) override;
        int32 GetPropertyCount(void* Struct, bool bIncludeSuper = true) override;
        void* GetPropertyListHead(void* Struct) override;
        EResult Initialize() override;
        bool IsInitialized() const override;

    private:
        bool ReadPropertyInfo(void* Field, FPropertyInfo& OutInfo);
        void* GetChildProperties(void* Struct);
        void* GetNextField(void* Field);
        std::string GetFieldClassName(void* Field);
        EPropertyType ClassifyProperty(const std::string& ClassName);

        // Offsets within UStruct
        int32 m_ChildPropertiesOffset;  // Offset to ChildProperties (FField*)
        int32 m_SuperStructOffset;      // Offset to SuperStruct

        // Offsets within FField
        int32 m_FField_ClassOffset;
        int32 m_FField_OwnerOffset;
        int32 m_FField_NextOffset;
        int32 m_FField_NameOffset;

        // Offsets within FProperty
        int32 m_FProperty_ArrayDimOffset;
        int32 m_FProperty_ElementSizeOffset;
        int32 m_FProperty_PropertyFlagsOffset;
        int32 m_FProperty_OffsetOffset;

        // FFieldClass contains class name
        int32 m_FFieldClass_NameOffset;

        bool m_bInitialized;
    };

    /**
     * Factory function to create appropriate property iterator
     */
    std::unique_ptr<IPropertyIterator> CreatePropertyIterator();

    /**
     * Global property iterator accessor
     */
    IPropertyIterator& GetPropertyIterator();

}