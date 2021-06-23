// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

class Phases {
    static Values = {
        Unknown:        0,
        Reading:        1 << 0,
        Lexing:         1 << 1,
        Preprocessing:  1 << 2,
        Parsing:        1 << 3,
        Importing:      1 << 4,
        NameResolution: 1 << 5,
        Typing:         1 << 6,
        Evaluation:     1 << 7,
        Instantiation:  1 << 8,
        Analysis:       1 << 9,
        CodeGeneration: 1 << 10,
        Linking:        1 << 11,
        Loading:        1 << 12,
        Execution:      1 << 13,
    };

    constructor(reader) {
        this.value = reader.read_uint32();
    }
}

class EmptyDir {
    static partition_name = "dir.empty";

    constructor(reader) {
        this.locus = new SourceLocation(reader);
    }
}

class AttributeDir {
    static partition_name = "dir.attribute";

    constructor(reader) {
        this.locus = new SourceLocation(reader);
        // TODO: AttrIndex
        this.attr = reader.read_uint32();
    }
}

class PragmaDir {
    static partition_name = "dir.pragma";

    constructor(reader) {
        this.locus = new SourceLocation(reader);
        this.words = new SentenceIndex(reader);
    }
}

class UsingDir {
    static partition_name = "dir.using";

    constructor(reader) {
        this.locus = new SourceLocation(reader);
        this.nominated = new ExprIndex(reader);
        this.resolution = new DeclIndex(reader);
    }
}

class UsingDeclarationDir {
    static partition_name = "dir.decl-use";

    constructor(reader) {
        this.locus = new SourceLocation(reader);
        this.path = new ExprIndex(reader);
        this.result = new DeclIndex(reader);
    }
}

class ExprDir {
    static partition_name = "dir.expr";

    constructor(reader) {
        this.locus = new SourceLocation(reader);
        this.expr = new ExprIndex(reader);
        this.phases = new Phases(reader);
    }
}

class StructuredBindingDir {
    static partition_name = "dir.struct-binding";

    constructor(reader) {
        this.locus = new SourceLocation(reader);
        this.bindings = new Sequence(DeclIndex, reader);
        this.names = new Sequence(TextOffset, reader);
    }
}

class SpecifiersSpreadDir {
    static partition_name = "dir.specifiers-spread";

    constructor(reader) {
        this.locus = new SourceLocation(reader);
    }
}

class TupleDir {
    static partition_name = "dir.tuple";

    constructor(reader) {
        this.seq = new HeapSequence(HeapSort.Values.Dir);
    }
}

function symbolic_for_dir_sort(sort) {
    switch (sort) {
    case DirIndex.Sort.Empty:
        return EmptyDir;
    case DirIndex.Sort.Attribute:
        return AttributeDir;
    case DirIndex.Sort.Pragma:
        return PragmaDir;
    case DirIndex.Sort.Using:
        return UsingDir;
    case DirIndex.Sort.DeclUse:
        return UsingDeclarationDir;
    case DirIndex.Sort.Expr:
        return ExprDir;
    case DirIndex.Sort.StructuredBinding:
        return StructuredBindingDir;
    case DirIndex.Sort.SpecifiersSpread:
        return SpecifiersSpreadDir;
    case DirIndex.Sort.Tuple:
        return TupleDir;
    case DirIndex.Sort.Unused0:
    case DirIndex.Sort.Unused1:
    case DirIndex.Sort.Unused2:
    case DirIndex.Sort.Unused3:
    case DirIndex.Sort.Unused4:
    case DirIndex.Sort.Unused5:
    case DirIndex.Sort.Unused6:
    case DirIndex.Sort.Unused7:
    case DirIndex.Sort.Unused8:
    case DirIndex.Sort.Unused9:
    case DirIndex.Sort.Unused10:
    case DirIndex.Sort.Unused11:
    case DirIndex.Sort.Unused12:
    case DirIndex.Sort.Unused13:
    case DirIndex.Sort.Unused14:
    case DirIndex.Sort.Unused15:
    case DirIndex.Sort.Unused16:
    case DirIndex.Sort.Unused17:
    case DirIndex.Sort.Unused18:
    case DirIndex.Sort.Unused19:
    case DirIndex.Sort.Unused20:
    case DirIndex.Sort.Unused21:
    case DirIndex.Sort.VendorExtension:
    case DirIndex.Sort.Count:
        console.error(`Bad sort: ${sort}`);
        return null;
    }
}
