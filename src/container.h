// start from here:
// - test remove with a test struct
// - write FloatArray, IntArray

#define ARR_GET(type, arr, index) *((type *)(arr_get(&arr, index)))

struct Array
{
    uint8_t *data;
    size_t elem_size;
    size_t count;
    size_t capacity;
};

static Array arr_create(size_t elem_size, size_t capacity)
{
    Array arr;
    arr.data = (uint8_t *)malloc(elem_size * capacity);
    arr.elem_size = elem_size;
    arr.count = 0;
    arr.capacity = capacity;
    return arr;
}

static void *arr_get(Array *arr, size_t index)
{
    assert(index < arr->count);
    return (void *)(arr->data + (index * arr->elem_size));
}

static void *_arr_slot_at(Array *arr, size_t index)
{
    return (void *)(arr->data + (index * arr->elem_size));
}

static void arr_add(Array *arr, void *elem_ptr)
{
    if (arr->count == arr->capacity)
    {
        printf("array capacity full, add failed\n"); // TODO @INCOMPLETE: Realloc a bigger space
        return;
    }

    void *arr_end_addr = _arr_slot_at(arr, arr->count);

    memcpy(arr_end_addr, elem_ptr, arr->elem_size);

    arr->count++;
}

static void arr_remove(Array *arr, void *elem_ptr)
{
    for (size_t i = 0; i < arr->count; i++)
    {
        if (memcmp(_arr_slot_at(arr, i), &elem_ptr, arr->elem_size) == 0)
        {
            for (size_t j = 0; j < arr->count - 1; j++) // Shift the rest of the array
            {
                memcpy(_arr_slot_at(arr, j), _arr_slot_at(arr, j + 1), arr->elem_size);
            }

            arr->count--;
            return;
        }
    }
}

static void arr_deinit(Array *arr)
{
    free(arr->data);
}
