//
// Copyright Microsoft.
//

#ifndef IFC_OPERATORS
#define IFC_OPERATORS

#include <concepts>
#include "ifc/underlying.hxx"

// -- This header file defines symbolic notation for operations needed
// -- in the elaboration (semantics) of C++ program fragments.  These operations
// -- are primarily concerned with the (parameterized) C++ abstract machine; they
// -- are intended to capture the essence of what makes C++, C++ in spite of evolution.
// -- Syntactic construct are handled elsewhere.  In essence, these semantic constructs
// -- are applicable to any programming language constructs that capture the essence of C++.


// Note: These probably belong to a different namespace.  Will move when all comes together.
namespace ifc {
    // The many sort of operators.
    // Note: Valid values of this type must fit in 4 bits.  See Operator class below.
    enum class OperatorSort : uint8_t {
        Niladic,        // no argument
        Monadic,        // one argument
        Dyadic,         // two arguments
        Triadic,        // three arguments
        Storage  = 0xE, // storage allocation and deallocation
        Variadic = 0xF, // any number of arguments
    };

    // The set of operators taking exactly no argument.
    // Note: Valid values of this type must fit in 12 bits.  See Operator class below.
    enum class NiladicOperator : uint16_t {
        Unknown,  // also serves as a placeholder for undefined
        Phantom,  // no expression -- not the same as Nil, which has type void
        Constant, // scalar or string literal constants, constants of class types
        Nil,      // void() -- regularize void! (or not; but be ready)

        Msvc = 0x0400,
        MsvcConstantObject, // MSVC internal representation of a class type object that is a constant
        MsvcLambda,

        Last
    };

    // The set of operators taking exactly one argument.
    // Note: Valid values of this type must fit in 12 bits.  See Operator class below.
    // clang-format off
    enum class MonadicOperator : uint16_t {
        Unknown,
        Plus,                                               // +x                   -- unethtical on non-literals, operator at source level
        Negate,                                             // -x
        Deref,                                              // *p
        Address,                                            // &x
        Complement,                                         // ~x
        Not,                                                // !x
        PreIncrement,                                       // ++x
        PreDecrement,                                       // --x
        PostIncrement,                                      // x++
        PostDecrement,                                      // x--
        Truncate,                                           // (int)3.14            -- abstract machine, not an operator at source level
        Ceil,                                               // ceil(3.14)           -- abstract machine, not an operator at source level
        Floor,                                              // floor(3.14)          -- abstract machine, not an operator at source level
        Paren,                                              // (x)                  -- abstract machine, syntactic at source level
        Brace,                                              // { x }                -- abstract machine, syntactic at source level
        Alignas,                                            // alignas(n)
        Alignof,                                            // alignof(x)
        Sizeof,                                             // sizeof x
        Cardinality,                                        // sizeof... (x)
        Typeid,                                             // typeid(x)
        Noexcept,                                           // noexcept(x)
        Requires,                                           // requires x
        CoReturn,                                           // co_return x
        Await,                                              // co_await x
        Yield,                                              // co_yield x
        Throw,                                              // throw x
        New,                                                // new T
        Delete,                                             // delete p
        DeleteArray,                                        // delete[] p
        Expand,                                             // x...                 -- pack expansion, not an operator at source level
        Read,                                               // lvalue-to-rvalue conversion -- abstract machine
        Materialize,                                        // temporary materialization -- abstract machine
        PseudoDtorCall,                                     // p->~T(), with T a scalar type
        LookupGlobally,                                     // ::x
        Artificial,                                         // Compiler-generated expression wrapper

        Msvc = 0x0400,
        MsvcAssume,                                         // __assume(x)
        MsvcAlignof,                                        // __builtin_alignof(x)
        MsvcUuidof,                                         // __uuidof(x)
        MsvcIsClass,                                        // __is_class(T)
        MsvcIsUnion,                                        // __is_union(T)
        MsvcIsEnum,                                         // __is_enum(T)
        MsvcIsPolymorphic,                                  // __is_polymorphic(T)
        MsvcIsEmpty,                                        // __is_empty(T)
        MsvcIsTriviallyCopyConstructible,                   // __is_trivially_copy_constructible(T)
        MsvcIsTriviallyCopyAssignable,                      // __is_trivially_copy_assignable(T)
        MsvcIsTriviallyDestructible,                        // __is_trivially_destructible(T)
        MsvcHasVirtualDestructor,                           // __has_virtual_destructor(T)
        MsvcIsNothrowCopyConstructible,                     // __is_nothrow_copy_constructible(T)
        MsvcIsNothrowCopyAssignable,                        // __is_nothrow_copy_assignable(T)
        MsvcIsPod,                                          // __is_pod(T)
        MsvcIsAbstract,                                     // __is_abstract(T)
        MsvcIsTrivial,                                      // __is_trivial(T)
        MsvcIsTriviallyCopyable,                            // __is_trivially_copyable(T)
        MsvcIsStandardLayout,                               // __is_standard_layout(T)
        MsvcIsLiteralType,                                  // __is_literal_type(T)
        MsvcIsTriviallyMoveConstructible,                   // __is_trivially_move_constructible(T)
        MsvcHasTrivialMoveAssign,                           // __has_trivial_move_assign(T)
        MsvcIsTriviallyMoveAssignable,                      // __is_trivially_move_assignable(T)
        MsvcIsNothrowMoveAssignable,                        // __is_nothrow_move_assign(T)
        MsvcUnderlyingType,                                 // __underlying_type(T)
        MsvcIsDestructible,                                 // __is_destructible(T)
        MsvcIsNothrowDestructible,                          // __is_nothrow_destructible(T)
        MsvcHasUniqueObjectRepresentations,                 // __has_unique_object_representations(T)
        MsvcIsAggregate,                                    // __is_aggregate(T)
        MsvcBuiltinAddressOf,                               // __builtin_addressof(x)
        MsvcIsRefClass,                                     // __is_ref_class(T)
        MsvcIsValueClass,                                   // __is_value_class(T)
        MsvcIsSimpleValueClass,                             // __is_simple_value_class(T)
        MsvcIsInterfaceClass,                               // __is_interface_class(T)
        MsvcIsDelegate,                                     // __is_delegate(f)
        MsvcIsFinal,                                        // __is_final(T)
        MsvcIsSealed,                                       // __is_sealed(T)
        MsvcHasFinalizer,                                   // __has_finalizer(T)
        MsvcHasCopy,                                        // __has_copy(T)
        MsvcHasAssign,                                      // __has_assign(T)
        MsvcHasUserDestructor,                              // __has_user_destructor(T)

        MsvcConfusion = 0x0FE0,                             // FIXME: Anything below is confusion to be removed
        MsvcConfusedExpand,                                 // The parser is confused about pack expansion
        MsvcConfusedDependentSizeof,                        // The parser is confused about dependent 'sizeof <expr>'
        MsvcConfusedPopState,                               // An EH state represented directly in a unary expression for invoking destructors after invoking a ctor.
        MsvcConfusedDtorAction,                             // An EH state represented directly in a unary expression for invoking destructors directly.
        MsvcConfusedVtorDisplacement,                       // The compiler generated expression representing an offset amount to a virtual base pointer address during initialization.
        MsvcConfusedDependentExpression,                    // At times the old YACC parser will 'give up' parsing a dependent expression and simply return a constant with a dummy bit set.
                                                            // This operator attempts to catch these offending dependent expression values.
        MsvcConfusedSubstitution,                           // Represents a template parameter substitution 'P -> expr' where 'expr' could be a type or a non-type argument replacement.
        MsvcConfusedAggregateReturn,                        // Decorates a return statement which returns an aggregate class type with a user-defined destructor.

        Last
    };
    // clang-format on

    // The set of operators taking exactly two arguments.
    // Note: Valid values of this type must fit in 12 bits.  See Operator class below.
    // clang-format off
    enum class DyadicOperator : uint16_t {
        Unknown,
        Plus,                               // x + y
        Minus,                              // x - y
        Mult,                               // x * y
        Slash,                              // x / y
        Modulo,                             // x % y
        Remainder,                          // x rem y              -- abstract machine, not an operator at source level
        Bitand,                             // x & y
        Bitor,                              // x | y
        Bitxor,                             // x ^ y
        Lshift,                             // x << y
        Rshift,                             // x >> y
        Equal,                              // x == y
        NotEqual,                           // x != y
        Less,                               // x < y
        LessEqual,                          // x <= y
        Greater,                            // x > y
        GreaterEqual,                       // x >= y
        Compare,                            // x <=> y
        LogicAnd,                           // x && y
        LogicOr,                            // x || y
        Assign,                             // x = y
        PlusAssign,                         // x += y
        MinusAssign,                        // x -= y
        MultAssign,                         // x *= y
        SlashAssign,                        // x /= y
        ModuloAssign,                       // X %= y
        BitandAssign,                       // x &= y
        BitorAssign,                        // x |= y
        BitxorAssign,                       // x ^= y
        LshiftAssign,                       // x <<= y
        RshiftAssign,                       // x >>= y
        Comma,                              // x, y
        Dot,                                // p.x
        Arrow,                              // p->x
        DotStar,                            // p.*x
        ArrowStar,                          // p->*x
        Curry,                              //                      -- abstract machine, bind the first parameter of a function taking more arguments
        Apply,                              // x(y, z)              -- abstract machine, apply a callable to an argument list (conceptually a tuple)
        Index,                              // x[y]                 -- indexing, prepared for possible generalization to multi-indices
        DefaultAt,                          // new(p) T
        New,                                // new T(x)
        NewArray,                           // new T[n]
        Destruct,                           // x.~T()
        DestructAt,                         // p->~T()
        Cleanup,                            //                      -- abstract machine, evaluate first operand then run second parameter as cleanup
        Qualification,                      // cv-qualification     -- abstract machine
        Promote,                            // integral or floating point promotion     -- abstract machine
        Demote,                             // integral or floating point conversion    -- abstract machine
        Coerce,                             //                      -- abstract machine implicit conversions that are not promotions or demotions
        Rewrite,                            //                      -- abstract machine, rewrite an expression into another
        Bless,                              // bless<T>(p)          -- abstract machine: valid T-object proclamation at p, no ctor run
        Cast,                               // (T)x
        ExplicitConversion,                 // T(x)
        ReinterpretCast,                    // reinterpret_cast<T>(x)
        StaticCast,                         // static_cast<T>(x)
        ConstCast,                          // const_cast<T>(x)
        DynamicCast,                        // dynamic_cast<T>(x)
        Narrow,                             //                      -- abstract machine, checked base to derived class conversion
        Widen,                              //                      -- abstract machine, derived to base class conversion
        Pretend,                            //                      -- abstract machine, generalization of bitcast and reinterpret cast
        Closure,                            //                      -- abstract machine, (env, func) pair formation; useful for lambdas
        ZeroInitialize,                     //                      -- abstract machine, zero-initialize an object or subject
        ClearStorage,                       //                      -- abstract machine, clear a storage span
        Select,                             // x :: y

        Msvc = 0x0400,
        MsvcTryCast,                        // WinTR try cast
        MsvcCurry,                          // MSVC bound member function extension
        MsvcVirtualCurry,                   // same as MsvcCurry, except the binding requires dynamic dispatch
        MsvcAlign,                          //                      -- alignment adjustment
        MsvcBitSpan,                        //                      -- abstract machine, span of bits
        MsvcBitfieldAccess,                 //                      -- abstract machine access to bitfield
        MsvcObscureBitfieldAccess,          //                      -- same as above with declaration annotation
        MsvcInitialize,                     //
        MsvcBuiltinOffsetOf,                // __builtin_offsetof(T, x)
        MsvcIsBaseOf,                       // __is_base_of(U, V)
        MsvcIsConvertibleTo,                // __is_convertible_to(U, V)
        MsvcIsTriviallyAssignable,          // __is_trivially_assignable(U, V)
        MsvcIsNothrowAssignable,            // __is_nothrow_assignable(U, V)
        MsvcIsAssignable,                   // __is_assignable(U, V)
        MsvcIsAssignableNocheck,            // __is_assignable_no_precondition_check(U, V)
        MsvcBuiltinBitCast,                 // __builtin_bit_cast(T, x)
        MsvcBuiltinIsLayoutCompatible,      // __builtin_is_layout_compatible(U, V)
        MsvcBuiltinIsPointerInterconvertibleBaseOf,         // __builtin_is_pointer_interconvertible_base_of(U, V)
        MsvcBuiltinIsPointerInterconvertibleWithClass,      // __builtin_is_pointer_interconvertible_with_class(U, V)
        MsvcBuiltinIsCorrespondingMember,   // __builtin_is_corresponding_member(x, y)
        MsvcIntrinsic,                      //                      -- abstract machine, misc intrinsic with no regular function declaration
        MsvcSaturatedArithmetic,            // An MSVC intrinsic for an abstract machine saturated arithemtic operation.
        MsvcBuiltinAllocationAnnotation,    // An MSVC intrinsic used to propagate debugging information to the runtime.  __allocation_annotation(x, T)

        Last
    };
    // clang-format on

    // The set of operators taking exactly three arguments.
    // Note: Valid values of this type must fit in 12 bits.  See Operator class below.
    // clang-format off
    enum class TriadicOperator : uint16_t {
        Unknown,
        Choice,                             // x ? : y: z
        ConstructAt,                        // new(p) T(x)
        Initialize,                         //                      -- abstract machine, initialize an object with operation and argument

        Msvc = 0x0400,

        MsvcConfusion = 0x0FE0,             // FIXME: Anything below is confusion to be removed
        MsvcConfusedPushState,              // An EH state tree representing a call to a ctor, dtor, and msvc-specific EH state flags.
        MsvcConfusedChoice,                 // Like 'Choice' above, but specific to codegen.

        Last
    };
    // clang-format on

    // The set of operators for dynamic storage manipulation, not classified by arity.
    // Note: Valid values of this type must fit in 12 bits.  See Operator class below.
    enum class StorageOperator : uint16_t {
        Unknown,
        AllocateSingle,   // operator new
        AllocateArray,    // operator new[]
        DeallocateSingle, // operator delete
        DeallocateArray,  // operator delete[]

        Msvc = 0x0400,

        Last
    };

    // The set of operators taking any number of arguments.
    // Note: Valid values of this type must fit in 12 bits.  See Operator class below.
    enum class VariadicOperator : uint16_t {
        Unknown,
        Collection, // x, y, z  -- collection of expressions, not the comma expression; no order of evaluation
        Sequence,   // Like Collection, but with a left-to-right sequencing order of evaluation

        Msvc = 0x0400,
        MsvcHasTrivialConstructor,    // __has_trivial_constructor(U, V, W, ...)
        MsvcIsConstructible,          // __is_constructible(U, V, W, ...)
        MsvcIsNothrowConstructible,   // __is_nothrow_constructible(U, V, W, ...)
        MsvcIsTriviallyConstructible, // __is_trivially_constructible(U, V, W, ...)

        Last
    };

    // length of OperatorSort::Variadic, which must be last
    inline constexpr auto sort_precision = ifc::bit_length(ifc::to_underlying(OperatorSort::Variadic));
    static_assert(sort_precision == 4);

    inline constexpr auto index_precision = 16 - sort_precision;

    template<typename T>
    concept CategoryPrecisionRequirement = (ifc::bit_length(ifc::to_underlying(T::Last) - 1u) <= index_precision);

    template<typename T>
    // clang-format off
    concept CategoryTypeRequirement =
        std::same_as<T, NiladicOperator>
        or std::same_as<T, MonadicOperator>
        or std::same_as<T, DyadicOperator>
        or std::same_as<T, TriadicOperator>
        or std::same_as<T, StorageOperator>
        or std::same_as<T, VariadicOperator>;
    // clang-format on

    // Conceptual denotation of C++ abstract machine operation
    template<typename T>
    concept OperatorCategory = CategoryTypeRequirement<T> and CategoryPrecisionRequirement<T>;

    // Mapping from arity-graded operators to their sorts
    constexpr OperatorSort operator_sort(NiladicOperator)
    {
        return OperatorSort::Niladic;
    }
    constexpr OperatorSort operator_sort(MonadicOperator)
    {
        return OperatorSort::Monadic;
    }
    constexpr OperatorSort operator_sort(DyadicOperator)
    {
        return OperatorSort::Dyadic;
    }
    constexpr OperatorSort operator_sort(TriadicOperator)
    {
        return OperatorSort::Triadic;
    }
    constexpr OperatorSort operator_sort(StorageOperator)
    {
        return OperatorSort::Storage;
    }
    constexpr OperatorSort operator_sort(VariadicOperator)
    {
        return OperatorSort::Variadic;
    }

    // Universal representation of arity-graded operators
    struct Operator {
        enum class Index : uint16_t {};
        constexpr Operator() : tag{}, value{} {}
        template<OperatorCategory Category>
        constexpr Operator(Category c) : tag(ifc::to_underlying(operator_sort(c))), value(ifc::to_underlying(c))
        {}
        constexpr OperatorSort sort() const
        {
            return OperatorSort(tag);
        }
        constexpr Index index() const
        {
            return Index(value);
        }

        auto operator<=>(const Operator&) const = default;

    private:
        uint16_t tag : sort_precision;
        uint16_t value : index_precision;
    };

    static_assert(sizeof(Operator) == sizeof(uint16_t));
} // namespace ifc

#endif // IFC_OPERATORS
