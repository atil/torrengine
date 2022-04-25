template <typename T>
struct Array {
    T *data;
    usize count;
    usize capacity;
};

template <typename T>
static Array<T> arr_new(usize capacity) {
    Array arr;
    arr.data = (T *)malloc(sizeof(T) * capacity);
    arr.count = 0;
    arr.capacity = capacity;
    return arr;
}

template <typename T>
static void arr_add(Array<T> *arr, T elem) {
    arr->data[arr->count] = elem;
    arr->count++;
}

template <typename T>
static T *arr_get_ref(Array<T> *arr, usize index) {
    return &data[usize];
}

template <typename T>
static void arr_deinit(Array<T> *arr) {

    // Note that we don't call any destructors here. We assume that this is a POD array
    free(arr->data);
    free(arr);
}