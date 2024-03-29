#include <cstdint> // C++ doesn't automatically define these. We need the include

typedef uint32_t buffer_handle;
typedef uint32_t texture_handle;
typedef uint32_t shader_handle;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int64_t i64;
typedef int32_t i32;
typedef size_t usize;
typedef float f32;

#define PREVENT_COPY_MOVE(class_name)                                                                        \
    class_name(const class_name &) = delete;                                                                 \
    class_name(class_name &&) = delete;                                                                      \
    class_name &operator=(const class_name &) = delete;                                                      \
    class_name &operator=(class_name &&) = delete

#define DISABLE_WARNINGS                                                                                     \
    _Pragma("warning(disable : 5045)")     /* Spectre thing */                                               \
        _Pragma("warning(disable : 4820)") /* Padding in struct */                                           \
        _Pragma("warning(disable : 4996)") /* Unsafe fopen() etc */                                          \
        _Pragma("warning(push, 0)")

#define ENABLE_WARNINGS _Pragma("warning(pop)")

#define UNREACHABLE(msg)                                                                                     \
    assert((msg, false));                                                                                    \
    exit(1)