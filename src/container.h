template <typename T>
struct Array {
    T *data;
    usize count;
    usize capacity;

    Array() = delete;

    explicit Array(usize cap) {
        // NOTE @BUGFIX: If a type can be an array element, then it needs a default ctor because of this:
        data = new T[cap];

        count = 0;
        capacity = cap;
    }

    void add(const T &elem) {
        // NOTE @BUGFIX: This argument used to be "add(T elem)" e.g. the value itself
        // When "elem" keeps some resources (e.g. memory), it's released at the end of this function,
        // since it's passed by value and that copy dies at the end of this scope
        // The caller of this function shouldn't have to care about whether T has a dtor or not. This is a point
        // where we differ from "handmade" C++
        assert(count < capacity); // TODO @INCOMPLETE: Realloc when it's full
        data[count] = elem;
        count++;
    }

    void replace(const T &elem, usize index) {
        assert(index < count);
        data[index].~T();
        data[index] = elem;
    }

    void remove(T *elem) {
        // This destroys elem completely
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

    void clear() {
        for (usize i = 0; i < count; i++) {
            data[i].~T();
        }
        count = 0;
    }

    ~Array() {
        delete[] data;
    }
    // Array(const &Array rhs) = delete;

    T *operator[](usize index) {
        return &(data[index]);
    }
};

//
// String
//

struct String { // TODO @INCOMPLETE: Empty string constant
    char *data;
    usize len;

    String() : data(nullptr), len(0) {
    }

    explicit String(char *chars) {
        len = 0;
        for (usize i = 0; chars[i] != 0; i++, len++)
            ;
        data = (char *)malloc(sizeof(char) * (len + 1));
        memcpy(data, chars, len);
        data[len] = 0;
    }

    void operator=(const String &rvalue) { // Assigning a string means deep copy
        // NOTE @BUGFIX: When adding a String to an Array<String>, we do a deep copy, so that when add() function's
        // parameter is out of scope (and the destructor is called), it won't release the resources of the in-array
        // string
        len = rvalue.len;
        if (data != nullptr) {
            free(data);
        }
        data = (char *)malloc(sizeof(char) * (len + 1));
        memcpy(data, rvalue.data, len + 1); // We know rvalue.data is null-terminated
    }

    ~String() {
        if (data != nullptr) {
            // NOTE @BUGFIX: Having this pointer null is a valid case: When we have an Array<String>,
            // not all new'ed (therefore initialized) memory is used, some of it has still nullptr
            free(data);
        }
        data = (char *)nullptr;
    }
};
