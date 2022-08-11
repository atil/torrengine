typedef uint32_t buffer_handle_t; // TODO @CLEANUP: Get rid of _t's
typedef uint32_t texture_handle_t;
typedef uint32_t shader_handle_t;

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