/* vec — generic dynamic array + stack over a caller-provided byte buffer.
 *
 * Minimal implementation of the frozen CLIBS_VEC_H contract. No malloc: all
 * state lives in the caller-supplied struct + storage buffer. Every method is
 * NULL-self-safe. Value semantics: each push/pop/get/set copies exactly
 * elem_size bytes via memcpy.
 */
#include "vec.h"

void vec_init(vec *self, void *storage, size_t cap, size_t elem_size) {
    if (self == NULL) {
        return;
    }
    self->buf = (uint8_t *)storage;
    self->elem_size = elem_size;
    self->cap = cap;
    self->len = 0;
}

bool vec_push(vec *self, const void *elem) {
    if (self == NULL || elem == NULL) {
        return false;
    }
    if (self->len >= self->cap) {
        return false;
    }
    memcpy(self->buf + self->len * self->elem_size, elem, self->elem_size);
    self->len++;
    return true;
}

bool vec_pop(vec *self, void *out) {
    if (self == NULL) {
        return false;
    }
    if (self->len == 0) {
        return false;
    }
    self->len--;
    if (out != NULL) {
        memcpy(out, self->buf + self->len * self->elem_size, self->elem_size);
    }
    return true;
}

bool vec_get(const vec *self, size_t i, void *out) {
    if (self == NULL || out == NULL) {
        return false;
    }
    if (i >= self->len) {
        return false;
    }
    memcpy(out, self->buf + i * self->elem_size, self->elem_size);
    return true;
}

bool vec_set(vec *self, size_t i, const void *elem) {
    if (self == NULL || elem == NULL) {
        return false;
    }
    if (i >= self->len) {
        return false;
    }
    memcpy(self->buf + i * self->elem_size, elem, self->elem_size);
    return true;
}

size_t vec_len(const vec *self) {
    if (self == NULL) {
        return 0;
    }
    return self->len;
}

size_t vec_cap(const vec *self) {
    if (self == NULL) {
        return 0;
    }
    return self->cap;
}

bool vec_is_empty(const vec *self) {
    if (self == NULL) {
        return true;
    }
    return self->len == 0;
}

bool vec_is_full(const vec *self) {
    if (self == NULL) {
        return false;
    }
    return self->len == self->cap;
}

void vec_clear(vec *self) {
    if (self == NULL) {
        return;
    }
    self->len = 0;
}
