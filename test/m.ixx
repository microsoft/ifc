export module m;

export
void glb_void_void_func();

void glb_void_void_func_not_exported();

export
extern void glb_void_void_extern_func();

extern void glb_void_void_extern_func_not_exported();

export
int glb_int_int_func(int);

export
int glb_int_int_char_char_ptr_func(int, char, char*);

export
namespace FunctionTraits
{
    // These exercise inspecting each bit from FunctionTraits.
    void trait_none();

    inline void trait_inline() { }

    constexpr void trait_constexpr() { }

    [[noreturn]] void trait_noreturn() { }

    void trait_deleted() = delete;

    template <typename>
    void trait_constrained() requires true;

    consteval void trait_immediate() { }

    // Class-based traits.
    struct ClassScope
    {
        explicit ClassScope(); // Explicit trait.

        virtual void trait_virtual();

        virtual void trait_pure_virtual() = 0;

        bool operator==(const ClassScope&) const = default; // Defaulted trait.

        virtual void trait_final();
        virtual void trait_override();
    };

    struct DerivedClassScope : ClassScope
    {
        void trait_final() final;
        void trait_override() override;
    };
} // namespace FunctionTraits