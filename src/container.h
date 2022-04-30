template <typename T>
struct Array {
    T *data;
    usize count;
    usize capacity;
    void (*deinit_func)(T *);

    explicit Array(usize cap) {
        data = (T *)malloc(sizeof(T) * cap);
        count = 0;
        capacity = cap;
    }

    void add(T elem) {
        assert(count < capacity);
        data[count] = elem;
        count++;
    }

    void remove(T *elem) {
        for (usize i = 0; i < count; i++) {
            if (memcmp(&data[i], elem, sizeof(T)) == 0) {
                for (usize j = i; j < count - 1; j++) {
                    data[j] = data[j + 1];
                }
                count--;
                return;
            }
        }
    }

    ~Array() {
        for (usize i = 0; i < count; i++) {
            data[i].~T();
        }
        free(data);
    }

    T *operator[](usize index) {
        return &(data[index]);
    }
};

//
// String
//

struct String {
    char *data;
    usize len;

    explicit String(char *chars) {
        len = 0;
        for (usize i = 0; chars[i] != 0; i++, len++)
            ;
        data = (char *)malloc(sizeof(char) * (len + 1));
        memcpy(data, chars, len);
        data[len] = 0;
    }

    ~String() {
        free(data);
    }
};
