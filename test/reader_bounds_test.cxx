#include <vector>
#include <cstddef>
#include <cstring>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "ifc/file.hxx"
#include "ifc/reader.hxx"

using namespace ifc;

// Mock implementation of ifc_assert that we can detect
static bool assertion_called = false;
static std::string assertion_message;

void ifc_assert(const char* text, const char* file, int line)
{
    assertion_called = true;
    assertion_message = text;
    // Don't actually abort, just record that assertion was called
}

namespace {
    // Helper to create a minimal buffer that simulates IFC file contents
    class MockInputIfc {
    public:
        MockInputIfc(size_t size) : buffer_(size, std::byte{0}) {
            // Create minimal valid IFC structure in buffer
            ifc_ = InputIfc{gsl::span<const std::byte>{buffer_.data(), buffer_.size()}};
        }
        
        InputIfc& get() { return ifc_; }
        const std::vector<std::byte>& contents() const { return buffer_; }
        
    private:
        std::vector<std::byte> buffer_;
        InputIfc ifc_;
    };
    
    // Helper to test view_entry_at with controlled buffer sizes
    template<typename T>
    void test_view_entry_at_bounds(size_t buffer_size, ByteOffset offset, bool should_assert) {
        assertion_called = false;
        assertion_message.clear();
        
        MockInputIfc mock_ifc(buffer_size);
        Reader reader(mock_ifc.get());
        
        try {
            // This should trigger our bounds check
            const T& result = reader.view_entry_at<T>(offset);
            (void)result; // Suppress unused variable warning
            
            // If we got here and should_assert was true, the test failed
            if (should_assert) {
                FAIL("Expected assertion but none occurred");
            }
        } catch (...) {
            // Any exception is fine - we're testing the assertion logic
        }
        
        if (should_assert) {
            CHECK(assertion_called);
            CHECK_MESSAGE(assertion_message.find("byte_offset < contents.size()") != std::string::npos ||
                          assertion_message.find("contents.size() - byte_offset") != std::string::npos,
                          "Assertion message should contain bounds check logic");
        } else {
            CHECK_FALSE(assertion_called);
        }
    }
}

TEST_CASE("Reader::view_entry_at bounds checking") {
    SUBCASE("Valid access within bounds") {
        // Buffer with 16 bytes, accessing 4-byte int at offset 0 - should work
        test_view_entry_at_bounds<uint32_t>(16, ByteOffset{0}, false);
        
        // Buffer with 16 bytes, accessing 4-byte int at offset 12 - should work (12 + 4 = 16)
        test_view_entry_at_bounds<uint32_t>(16, ByteOffset{12}, false);
    }
    
    SUBCASE("Invalid access - offset at boundary") {
        // Buffer with 16 bytes, accessing 4-byte int at offset 16 - should fail
        // This is the exact boundary case: offset == buffer_size
        test_view_entry_at_bounds<uint32_t>(16, ByteOffset{16}, true);
    }
    
    SUBCASE("Invalid access - offset beyond boundary") {
        // Buffer with 16 bytes, accessing 4-byte int at offset 20 - should fail
        test_view_entry_at_bounds<uint32_t>(16, ByteOffset{20}, true);
    }
    
    SUBCASE("Invalid access - insufficient space for object") {
        // Buffer with 16 bytes, accessing 4-byte int at offset 14 - should fail
        // 14 + 4 = 18, but buffer is only 16 bytes
        test_view_entry_at_bounds<uint32_t>(16, ByteOffset{14}, true);
        
        // Buffer with 16 bytes, accessing 4-byte int at offset 15 - should fail  
        // 15 + 4 = 19, but buffer is only 16 bytes
        test_view_entry_at_bounds<uint32_t>(16, ByteOffset{15}, true);
    }
    
    SUBCASE("Edge case - accessing larger objects") {
        // Buffer with 16 bytes, accessing 8-byte double at offset 8 - should work
        test_view_entry_at_bounds<uint64_t>(16, ByteOffset{8}, false);
        
        // Buffer with 16 bytes, accessing 8-byte double at offset 9 - should fail
        // 9 + 8 = 17, but buffer is only 16 bytes
        test_view_entry_at_bounds<uint64_t>(16, ByteOffset{9}, true);
    }
    
    SUBCASE("Boundary case - zero-sized buffer") {
        // Empty buffer, any access should fail
        test_view_entry_at_bounds<uint8_t>(0, ByteOffset{0}, true);
    }
    
    SUBCASE("Single byte buffer") {
        // 1-byte buffer, accessing 1 byte at offset 0 - should work
        test_view_entry_at_bounds<uint8_t>(1, ByteOffset{0}, false);
        
        // 1-byte buffer, accessing 1 byte at offset 1 - should fail
        test_view_entry_at_bounds<uint8_t>(1, ByteOffset{1}, true);
        
        // 1-byte buffer, accessing 4 bytes at offset 0 - should fail
        test_view_entry_at_bounds<uint32_t>(1, ByteOffset{0}, true);
    }
}

TEST_CASE("Demonstrate the original bug scenario") {
    SUBCASE("Original assertion was insufficient") {
        // This test demonstrates why the original assertion was insufficient
        // Original: IFCASSERT(byte_offset < contents.size());
        // This would pass for buffer_size=16, offset=15, sizeof(uint32_t)=4
        // because 15 < 16, but 15+4 > 16 causing buffer overrun
        
        const size_t buffer_size = 16;
        const ByteOffset offset{15};  // This is < buffer_size
        
        // With original assertion: 15 < 16 would be true, allowing the access
        // But accessing 4 bytes starting at offset 15 reads bytes 15,16,17,18
        // in a 16-byte buffer (indices 0-15), causing overrun at indices 16,17,18
        
        // Our new assertion prevents this:
        test_view_entry_at_bounds<uint32_t>(buffer_size, offset, true);
    }
    
    SUBCASE("New assertion correctly prevents overruns") {
        // New assertion: byte_offset < contents.size() && (contents.size() - byte_offset) >= sizeof(T)
        // For buffer_size=16, offset=15, sizeof(uint32_t)=4:
        // - 15 < 16 is true
        // - (16 - 15) >= 4 is false (1 >= 4 is false)
        // So overall assertion is false, preventing the overrun
        
        assertion_called = false;
        
        MockInputIfc mock_ifc(16);
        Reader reader(mock_ifc.get());
        
        // This should now properly fail with our improved bounds check
        test_view_entry_at_bounds<uint32_t>(16, ByteOffset{15}, true);
        
        CHECK(assertion_called);
    }
}