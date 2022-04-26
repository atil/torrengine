template <typename T>
struct Array {
    T *data;
    usize count;
    usize capacity;

    // This exists because non-member function looks stupid and indexer operator cannot be a non-member
    // and member indexer looks stupid when we give the array as a pointer parameter
    T *at(usize index) {
        return &(data[index]);
    }
};

template <typename T>
static Array<T> arr_new(usize capacity) {
    Array<T> arr;
    arr.data = (T *)malloc(sizeof(T) * capacity);
    arr.count = 0;
    arr.capacity = capacity;
    return arr;
}

template <typename T>
static void arr_add(Array<T> *arr, T elem) {
    assert(arr->count < arr->capacity);
    arr->data[arr->count] = elem;
    arr->count++;
}

template <typename T>
static void arr_remove(Array<T> *arr, T *elem) {
    for (usize i = 0; i < arr->count; i++) {
        if (memcmp(&arr->data[i], elem, sizeof(T)) == 0) {
            for (usize j = i; j < arr->count - 1; j++) {
                arr->data[j] = arr->data[j + 1];
            }
            arr->count--;
            return;
        }
    }
}

template <typename T>
static void arr_deinit(Array<T> *arr) {
    // TODO @ROBUSTNESS: Note that we don't call any destructors here. We assume that this is a POD array
    free(arr->data);
    free(arr);
}
