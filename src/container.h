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

        // TODO @ROBUSTNESS: This runs the assingment operator for T, which requires special handling for structs
        // with pointers in them. Strings for example. Currently we deep-copy them
        data[count] = elem;

        count++;
    }

/*
    // One way of implementing move, C++98 style
    // Cons: introduces inheritence, have to use new, can't use malloc
    // Pros: stays c-with-classes-with-new-instead-of-malloc
    
    void add_move(T *elem) {
        data[count] = elem->move();
    }

    template<class T>
    struct IMoveable {
        virtual T move() = 0;
    };

    struct String : public IMoveable<String> {...} 

    String::move() {
        String s = *this;
        this->data = nullptr;
        return s;
    }
*/
/*
    // Other way, with rvalue references
    // Cons: goes beyond c-with-classes, strange mix of malloc and move, wtf
    // Pros: no inheritence
    
    static T&& move(T *obj) {
        return (T&&) (*obj);
    }
    
    void add_move(T *elem) {
        data[count] = move(elem);
    }

    void operator=(String&& moved_out_of) {
        this->data = moved_out_of->data;
        moved_out_of->data = nullptr;
    }
*/

    void replace(const T &elem, usize index) {
        assert(index < count);
        data[index].~T();
        data[index] = elem;
    }

    void remove(T *elem) {
        // This destroys elem completely
        for (usize i = 0; i < count; i++) {
            if (memcmp(&data[i], elem, sizeof(T)) == 0) { // Shift the rest to keep the order
                // TODO @BUG: If elem keeps a pointer, we need to free that. Call dtor here
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
    Array(const Array &) = delete;

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

    // TODO @ROBUSTNESS: This parameter needs to be null-terminated
    explicit String(char *chars) {
        len = 0;
        for (usize i = 0; chars[i] != 0; i++, len++)
            ;
        data = (char *)malloc(sizeof(char) * (len + 1));
        memcpy(data, chars, len);
        data[len] = 0;
    }

    // TODO @ROBUSTNESS: There are so many strings attach to this operator. Rethink this.
    void operator=(const String &rhs) { // Assigning a string means deep copy
        // NOTE @BUGFIX: When adding a String to an Array<String>, we do a deep copy, so that when add() function's
        // parameter is out of scope (and the destructor is called), it won't release the resources of the in-array
        // string
        len = rhs.len;
        if (data != nullptr) {
            // TODO @ROBUSTNESS: This means we have to zero-initialize this struct _every_ time
            // i.e. can't just malloc and start using it
            free(data);
        }
        data = (char *)malloc(sizeof(char) * (len + 1));
        memcpy(data, rhs.data, len + 1); // We know rvalue.data is null-terminated
    }

    bool operator==(String *rhs) {
        if (len != rhs->len) {
            return false;
        }
        return memcmp(data, rhs->data, len) == 0;
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

//
// Index map
// A simple map specifically tailored for indexes
//

// TODO @TASK: We still can use this for the entities, but we need a proper hashmap
struct IndexMap {
    u32 *keys;
    u32 *values;
    usize capacity;
    const u32 invalid = (u32)-1;
    u8 _padding[4];

    explicit IndexMap(usize cap) : capacity(capacity) {
        keys = (u32 *)malloc(sizeof(u32) * cap);
        values = (u32 *)malloc(sizeof(u32) * cap);
        for (usize i = 0; i < cap; i++) { // memset doesn't work here because usize is not a single byte
            keys[i] = invalid;
            values[i] = invalid;
        }
    }

    void add(u32 key, u32 value) {
        for (usize i = 0; i < capacity; i++) {
            if (keys[i] == invalid) {
                keys[i] = key;
                values[i] = value;
                return;
            }
        }
        assert(0); // This should never hit
    }

    void remove(u32 key) {
        for (usize i = 0; i < capacity; i++) {
            if (keys[i] == key) {
                keys[i] = invalid;
                return;
            }
        }
    }

    u32 get(u32 key) {
        for (usize i = 0; i < capacity; i++) {
            if (keys[i] == key) {
                return values[i];
            }
        }
        assert(0); // TODO @INCOMPLETE: We don't handle this case right now
        return invalid;
    }

    ~IndexMap() {
        free(keys);
        free(values);
    }

    IndexMap(const IndexMap &) = delete;
    IndexMap(IndexMap &&) = delete;
    void operator=(const IndexMap &) = delete;
    void operator=(IndexMap &&) = delete;
};

//
// TagMap: A map where we store values with unique names
//

template <typename T>
struct TagMapNode {
    T value; // Owning this here because initially we thought we'd used PODs as values
    String key;
    TagMapNode *next;
};

template <typename T>
struct TagMap {
    usize capacity;
    TagMapNode<T> **buckets;
    const usize invalid_bucket = (usize)-1;

    explicit TagMap(usize cap) : capacity(cap) {
        buckets = (TagMapNode<T> **)malloc(sizeof(TagMapNode<T> *) * capacity);
        memset(buckets, 0, sizeof(TagMapNode<T> *) * capacity);
    }

    usize hash_func(String *key) {
        return 4;
        // u32 sum = 0;
        // for (usize i = 0; i < key->len; i++) {
        //     sum += (u32)key->data[i];
        // }
        // return (sum % 599) % capacity;
    }

    void add_or_update(String *key, T value) {
        usize bucket_index = hash_func(key);
        TagMapNode<T> *bucket = buckets[bucket_index];
        TagMapNode<T> *prev_node = nullptr;
        while (bucket != nullptr) {
            if (bucket->key == key) { // Same key found, overwrite the old value
                bucket->value = value;
                return;
            }
            prev_node = bucket;
            bucket = bucket->next;
        }

        // TODO @ROBUSTNESS: Need calloc to zero-initalize the key string.
        // Otherwise its assignment operator fails
        bucket = (TagMapNode<T> *)calloc(1, sizeof(TagMapNode<T>));
        bucket->key = *key; // Copies key string
        bucket->value = value;
        bucket->next = nullptr;

        if (prev_node == nullptr) {
            buckets[bucket_index] = bucket; // Adding as the first node in this bucket
        } else {
            prev_node->next = bucket;
        }
    }

    T *get(String *key) {
        usize bucket_index = hash_func(key);

        for (TagMapNode<T> *bucket = buckets[bucket_index]; bucket; bucket = bucket->next) {
            if (bucket->key == key) {
                return &bucket->value;
            }
        }

        return nullptr;
    }

    void remove(String *key) {
        usize bucket_index = hash_func(key);
        TagMapNode<T> *node_to_remove = buckets[bucket_index];
        TagMapNode<T> *prev_node = nullptr;

        while (node_to_remove != nullptr) {
            if (node_to_remove->key == key) {
                break;
            }
            prev_node = node_to_remove;
            node_to_remove = node_to_remove->next;
        }
        assert(node_to_remove != nullptr);

        node_to_remove->key.~String();
        node_to_remove->value.~T();
        TagMapNode<T> *next = node_to_remove->next;
        free(node_to_remove);

        if (prev_node == nullptr) {
            buckets[bucket_index] = next; // Removing the first node of the bucket
        } else {
            prev_node->next = next;
        }
    }

    ~TagMap() {
        for (usize i = 0; i < capacity; i++) {
            TagMapNode<T> *bucket = buckets[i];
            while (bucket != nullptr) {
                bucket->key.~String();
                bucket->value.~T();
                TagMapNode<T> *next = bucket->next;
                free(bucket);
                bucket = next;
            }
        }

        free(buckets);
    }

    TagMap(const TagMap &) = delete;
    TagMap(TagMap &&) = delete;
    void operator=(const TagMap &) = delete;
    void operator=(TagMap &&) = delete;
};

