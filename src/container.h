typedef struct
{
    uint8_t *data;
    size_t elem_size;
    size_t count;
    size_t capacity;
} Array;

static Array arr_create(size_t elem_size, size_t capacity)
{
    Array arr;
    arr.data = (uint8_t *)malloc(elem_size * capacity);
    arr.elem_size = elem_size;
    arr.count = 0;
    arr.capacity = capacity;
    return arr;
}

static void arr_add(Array *arr, uint8_t *elem_ptr)
{
    if (arr->count == arr->capacity)
    {
        printf("array capacity full, add failed\n"); // TODO @INCOMPLETE: Realloc a bigger space
        return;
    }

    memcpy(&arr->data[arr->count], elem_ptr, arr->elem_size);

    arr->count++;
}

static void arr_remove(Array *arr, uint8_t *elem_ptr)
{
    for (size_t i = 0; i < arr->count; i++)
    {
        if (memcmp(&arr->data[i], &elem_ptr, arr->elem_size) == 0)
        {
            for (size_t j = 0; j < arr->count - 1; j++) // Shift the rest of the array
            {
                memcpy(&arr->data[j], &arr->data[j + 1], arr->elem_size);
            }

            return;
        }
    }
}

static uint8_t *arr_get(Array *arr, size_t index)
{
    assert(index < arr->count);
    return &arr->data[index];
}

static void arr_deinit(Array *arr)
{
    free(arr->data);
}
