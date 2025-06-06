// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

class EnumeratorDecl {
    static partition_name = "decl.enumerator";

    constructor(reader) {
        this.identity = new IdentityTextOffset(reader);
        this.type = new TypeIndex(reader);
        this.initializer = new ExprIndex(reader);
        this.basic_spec = new BasicSpecifiers(reader);
        this.access = new Access(reader);
    }
}

class VariableDecl {
    static partition_name = "decl.variable";

    constructor(reader) {
        this.identity = new IdentityNameIndex(reader);
        this.type = new TypeIndex(reader);
        this.home_scope = new DeclIndex(reader);
        this.initializer = new ExprIndex(reader);
        this.alignment = new ExprIndex(reader);
        this.obj_spec = new ObjectTraits(reader);
        this.basic_spec = new BasicSpecifiers(reader);
        this.access = new Access(reader);
        this.properties = new ReachableProperties(reader);
    }
}

class ParameterSort {
    static Values = {
        Object:   0, // Function parameter
        Type:     1, // Type template parameter
        NonType:  2, // Non-type template parameter
        Template: 3, // Template template parameter
        Count:    4
    };

    constructor(reader) {
        this.value = reader.read_uint8();
    }
}

// This is a strong type representing a default argument decl.
class DefaultIndex {
    constructor(reader) {
        const pointed = reader.read_uint32(reader);
        if (pointed != 0)
        {
            this.index = index_from_raw(DeclIndex, DeclIndex.Sort.DefaultArgument, pointed - 1);
        }
        else
        {
            this.index = index_from_raw(DeclIndex, DeclIndex.Sort.VendorExtension, 0)
        }
    }
}

class ParameterDecl {
    static partition_name = "decl.parameter";

    constructor(reader) {
        this.identity = new IdentityTextOffset(reader);
        this.type = new TypeIndex(reader);
        this.type_constraint = new ExprIndex(reader);
        this.initializer = new DefaultIndex(reader);
        this.level = reader.read_uint32();
        this.position = reader.read_uint32();
        this.sort = new ParameterSort(reader);
        this.properties = new ReachableProperties(reader);
        this.pad1 = new StructPadding(reader);
        this.pad2 = new StructPadding(reader);
    }
}

class FieldDecl {
    static partition_name = "decl.field";

    constructor(reader) {
        this.identity = new IdentityTextOffset(reader);
        this.type = new TypeIndex(reader);
        this.home_scope = new DeclIndex(reader);
        this.initializer = new ExprIndex(reader);
        this.alignment = new ExprIndex(reader);
        this.obj_spec = new ObjectTraits(reader);
        this.basic_spec = new BasicSpecifiers(reader);
        this.access = new Access(reader);
        this.properties = new ReachableProperties(reader);
    }
}

class BitfieldDecl {
    static partition_name = "decl.bitfield";

    constructor(reader) {
        this.identity = new IdentityTextOffset(reader);
        this.type = new TypeIndex(reader);
        this.home_scope = new DeclIndex(reader);
        this.width = new ExprIndex(reader);
        this.initializer = new ExprIndex(reader);
        this.obj_spec = new ObjectTraits(reader);
        this.basic_spec = new BasicSpecifiers(reader);
        this.access = new Access(reader);
        this.properties = new ReachableProperties(reader);
    }
}

class ScopeDecl {
    static partition_name = "decl.scope";

    constructor(reader) {
        this.identity = new IdentityNameIndex(reader);
        this.type = new TypeIndex(reader);
        this.base = new TypeIndex(reader);
        this.initializer = new ScopeIndex(reader);
        this.home_scope = new DeclIndex(reader);
        this.alignment = new ExprIndex(reader);
        this.pack_size = reader.read_uint16();
        this.basic_spec = new BasicSpecifiers(reader);
        this.scope_spec = new ScopeTraits(reader);
        this.access = new Access(reader);
        this.properties = new ReachableProperties(reader);
    }
}

class EnumerationDecl {
    static partition_name = "decl.enum";

    constructor(reader) {
        this.identity = new IdentityTextOffset(reader);
        this.type = new TypeIndex(reader);
        this.base = new TypeIndex(reader);
        this.initializer = new Sequence(EnumeratorDecl, reader);
        this.home_scope = new DeclIndex(reader);
        this.alignment = new ExprIndex(reader);
        this.basic_spec = new BasicSpecifiers(reader);
        this.access = new Access(reader);
        this.properties = new ReachableProperties(reader);
    }
}

class AliasDecl {
    static partition_name = "decl.alias";

    constructor(reader) {
        this.identity = new IdentityTextOffset(reader);
        this.type = new TypeIndex(reader);
        this.home_scope = new DeclIndex(reader);
        this.aliasee = new TypeIndex(reader);
        this.basic_spec = new BasicSpecifiers(reader);
        this.access = new Access(reader);
    }
}

class ParameterizedEntity {
    constructor(reader) {
        this.decl = new DeclIndex(reader);
        this.head = new SentenceIndex(reader);
        this.body = new SentenceIndex(reader);
        this.attributes = new SentenceIndex(reader);
    }
}

class TemploidDecl {
    static partition_name = "decl.temploid";

    constructor(reader) {
        this.entity = new ParameterizedEntity(reader);
        this.chart = new ChartIndex(reader);
        this.properties = new ReachableProperties(reader);
    }
}

class TemplateDecl {
    static partition_name = "decl.template";

    constructor(reader) {
        this.identity = new IdentityNameIndex(reader);
        this.home_scope = new DeclIndex(reader);
        this.chart = new ChartIndex(reader);
        this.entity = new ParameterizedEntity(reader);
        this.type = new TypeIndex(reader);
        this.basic_spec = new BasicSpecifiers(reader);
        this.access = new Access(reader);
        this.properties = new ReachableProperties(reader);
    }
}

class PartialSpecializationDecl {
    static partition_name = "decl.partial-specialization";

    constructor(reader) {
        this.identity = new IdentityNameIndex(reader);
        this.home_scope = new DeclIndex(reader);
        this.chart = new ChartIndex(reader);
        this.entity = new ParameterizedEntity(reader);
        this.specialization_info = reader.read_uint32();
        this.basic_spec = new BasicSpecifiers(reader);
        this.access = new Access(reader);
        this.properties = new ReachableProperties(reader);
    }
}

class SpecializationSort {
    static Values = {
        Implicit:      0, // An implicit specialization.
        Explicit:      1, // An explicit specialization.
        Instantiation: 2  // An explicit instantiation.
    };

    constructor(reader) {
        this.value = reader.read_uint8();
    }
}

class SpecializationDecl {
    static partition_name = "decl.specialization";

    constructor(reader) {
        this.specialization_info = reader.read_uint32();
        this.decl = new DeclIndex(reader);
        this.sort = new SpecializationSort(reader);
        this.pad1 = new StructPadding(reader);
        this.pad2 = new StructPadding(reader);
        this.pad3 = new StructPadding(reader);
    }
}

class DefaultArgumentDecl {
    static partition_name = "decl.default-arg";

    constructor(reader) {
        this.locus = new SourceLocation(reader);
        this.type = new TypeIndex(reader);
        this.home_scope = new DeclIndex(reader);
        this.initializer = new ExprIndex(reader);
        this.basic_spec = new BasicSpecifiers(reader);
        this.access = new Access(reader);
        this.properties = new ReachableProperties(reader);
        this.pad1 = new StructPadding(reader);
    }
}

class ConceptDecl {
    static partition_name = "decl.concept";

    constructor(reader) {
        this.identity = new IdentityTextOffset(reader);
        this.home_scope = new DeclIndex(reader);
        this.type = new TypeIndex(reader);
        this.chart = new ChartIndex(reader);
        this.constraint = new ExprIndex(reader);
        this.basic_spec = new BasicSpecifiers(reader);
        this.access = new Access(reader);
        this.head = new SentenceIndex(reader);
        this.body = new SentenceIndex(reader);
    }
}

class FunctionDecl {
    static partition_name = "decl.function";

    constructor(reader) {
        this.identity = new IdentityNameIndex(reader);
        this.type = new TypeIndex(reader);
        this.home_scope = new DeclIndex(reader);
        this.chart = new ChartIndex(reader);
        this.traits = new FunctionTraits(reader);
        this.basic_spec = new BasicSpecifiers(reader);
        this.access = new Access(reader);
        this.properties = new ReachableProperties(reader);
    }
}

class NonStaticMemberFunctionDecl {
    static partition_name = "decl.method";

    constructor(reader) {
        this.identity = new IdentityNameIndex(reader);
        this.type = new TypeIndex(reader);
        this.home_scope = new DeclIndex(reader);
        this.chart = new ChartIndex(reader);
        this.traits = new FunctionTraits(reader);
        this.basic_spec = new BasicSpecifiers(reader);
        this.access = new Access(reader);
        this.properties = new ReachableProperties(reader);
    }
}

class ConstructorDecl {
    static partition_name = "decl.constructor";

    constructor(reader) {
        this.identity = new IdentityTextOffset(reader);
        this.type = new TypeIndex(reader);
        this.home_scope = new DeclIndex(reader);
        this.chart = new ChartIndex(reader);
        this.traits = new FunctionTraits(reader);
        this.basic_spec = new BasicSpecifiers(reader);
        this.access = new Access(reader);
        this.properties = new ReachableProperties(reader);
    }
}

class InheritedConstructorDecl {
    static partition_name = "decl.inherited-constructor";

    constructor(reader) {
        this.identity = new IdentityTextOffset(reader);
        this.type = new TypeIndex(reader);
        this.home_scope = new DeclIndex(reader);
        this.chart = new ChartIndex(reader);
        this.traits = new FunctionTraits(reader);
        this.basic_spec = new BasicSpecifiers(reader);
        this.access = new Access(reader);
        this.base_ctor = new DeclIndex(reader);
    }
}

class DestructorDecl {
    static partition_name = "decl.destructor";

    constructor(reader) {
        this.identity = new IdentityTextOffset(reader);
        this.home_scope = new DeclIndex(reader);
        this.eh_spec = new NoexceptSpecification(reader);
        this.traits = new FunctionTraits(reader);
        this.basic_spec = new BasicSpecifiers(reader);
        this.access = new Access(reader);
        this.convention = new CallingConvention(reader);
        this.properties = new ReachableProperties(reader);
    }
}

class ModuleReference {
    constructor(reader) {
        this.owner = new TextOffset(reader);
        this.partition = new TextOffset(reader);
    }
}

class ReferenceDecl {
    static partition_name = "decl.reference";

    constructor(reader) {
        this.translation_unit = new ModuleReference(reader);
        this.local_index = new DeclIndex(reader);
    }
}

class UsingDecl {
    static partition_name = "decl.using-declaration";

    constructor(reader) {
        this.identity = new IdentityTextOffset(reader);
        this.home_scope = new DeclIndex(reader);
        this.resolution = new DeclIndex(reader);
        this.parent = new ExprIndex(reader);
        this.name = new TextOffset(reader);
        this.basic_spec = new BasicSpecifiers(reader);
        this.access = new Access(reader);
        this.is_hidden = reader.read_uint8();
    }
}

class FriendDecl {
    static partition_name = "decl.friend";

    constructor(reader) {
        this.index = new ExprIndex(reader);
    }
}

class ExpansionDecl {
    static partition_name = "decl.expansion";

    constructor(reader) {
        this.locus = new SourceLocation(reader);
        this.operand = new DeclIndex(reader);
    }
}

class GuideTraits {
    static Values = {
        None:     0,
        Explicit: 1 << 0  // The deduction guide is declared as 'explicit'.
    };

    constructor(reader) {
        this.value = reader.read_uint8();
    }
}

class DeductionGuideDecl {
    static partition_name = "decl.deduction-guide";

    constructor(reader) {
        this.identity = new IdentityTextOffset(reader);
        this.home_scope = new DeclIndex(reader);
        this.source = reader.read_uint32(); // ChartIndex
        this.target = new ExprIndex(reader);
        this.traits = new GuideTraits(reader);
        this.basic_spec = new BasicSpecifiers(reader);
    }
}

class BarrenDecl {
    static partition_name = "decl.barren";

    constructor(reader) {
        this.directive = new DirIndex(reader);
        this.basic_spec = new BasicSpecifiers(reader);
        this.access = new Access(reader);
    }
}

class TupleDecl {
    static partition_name = "decl.tuple";

    constructor(reader) {
        this.seq = new HeapSequence(HeapSort.Values.Decl, reader);
    }
}

class SyntacticDecl {
    static partition_name = "decl.syntax-tree";

    constructor(reader) {
        this.syntax = new SyntaxIndex(reader);
    }
}

class IntrinsicDecl {
    static partition_name = "decl.intrinsic";

    constructor(reader) {
        this.identity = new IdentityTextOffset(reader);
        this.type = new TypeIndex(reader);
        this.home_scope = new DeclIndex(reader);
        this.basic_spec = new BasicSpecifiers(reader);
        this.access = new Access(reader);
    }
}

class PropertyDecl {
    static partition_name = "decl.property";

    constructor(reader) {
        this.data_member = new DeclIndex(reader);
    }
}

class SegmentDecl {
    static partition_name = "decl.segment";

    constructor(reader) {
        this.name = new TextOffset(reader);
        this.class_id = new TextOffset(reader);
        this.seg_spec = new SegmentTraits(reader);
        this.type = new SegmentType(reader);
    }
}

class VendorDecl {
    static partition_name = "decl.vendor-extension";

    constructor(reader) {
        this.index = new VendorIndex(reader);
    }
}

function symbolic_for_decl_sort(sort) {
    switch (sort) {
    case DeclIndex.Sort.Enumerator:
        return EnumeratorDecl;
    case DeclIndex.Sort.Variable:
        return VariableDecl;
    case DeclIndex.Sort.Parameter:
        return ParameterDecl;
    case DeclIndex.Sort.Field:
        return FieldDecl;
    case DeclIndex.Sort.Bitfield:
        return BitfieldDecl;
    case DeclIndex.Sort.Scope:
        return ScopeDecl;
    case DeclIndex.Sort.Enumeration:
        return EnumerationDecl;
    case DeclIndex.Sort.Alias:
        return AliasDecl;
    case DeclIndex.Sort.Temploid:
        return TemploidDecl;
    case DeclIndex.Sort.Template:
        return TemplateDecl;
    case DeclIndex.Sort.PartialSpecialization:
        return PartialSpecializationDecl;
    case DeclIndex.Sort.Specialization:
        return SpecializationDecl;
    case DeclIndex.Sort.DefaultArgument:
        return DefaultArgumentDecl;
    case DeclIndex.Sort.Concept:
        return ConceptDecl;
    case DeclIndex.Sort.Function:
        return FunctionDecl;
    case DeclIndex.Sort.Method:
        return NonStaticMemberFunctionDecl;
    case DeclIndex.Sort.Constructor:
        return ConstructorDecl;
    case DeclIndex.Sort.InheritedConstructor:
        return InheritedConstructorDecl;
    case DeclIndex.Sort.Destructor:
        return DestructorDecl;
    case DeclIndex.Sort.Reference:
        return ReferenceDecl;
    case DeclIndex.Sort.Using:
        return UsingDecl;
    case DeclIndex.Sort.Friend:
        return FriendDecl;
    case DeclIndex.Sort.Expansion:
        return ExpansionDecl;
    case DeclIndex.Sort.DeductionGuide:
        return DeductionGuideDecl
    case DeclIndex.Sort.Barren:
        return BarrenDecl;
    case DeclIndex.Sort.Tuple:
        return TupleDecl;
    case DeclIndex.Sort.SyntaxTree:
        return SyntacticDecl;
    case DeclIndex.Sort.Intrinsic:
        return IntrinsicDecl;
    case DeclIndex.Sort.Property:
        return PropertyDecl;
    case DeclIndex.Sort.OutputSegment:
        return SegmentDecl;
    case DeclIndex.Sort.VendorExtension:
        return VendorDecl;
    case DeclIndex.Sort.UnusedSort0:
    default:
        console.error(`Bad sort: ${sort}`);
        return null;
    }
}