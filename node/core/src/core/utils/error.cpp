#include "utils/error.h"
#include "utils/vec.h"

int marshal_error(
    Error& e, 
    Msg* msg, 
    Actors too,
    std::function<Vec*(size_t)> get_buffer
) {
    msg->code = e.code;
    msg->id = e.id;
    msg->too = too;
    msg->is_wiped = false;

    msg->data = get_buffer(KEY_SIZE + sizeof(e.r) + e.msg.size());
    if (!msg->data) return -1;
    Vec* d = msg->data;

    vec_write(d, e.key);
    vec_write(d, e.r);
    vec_write(d, e.msg.size());
    vec_write(d, reinterpret_cast<unsigned char*>(e.msg.data()), e.msg.size());

    return 0;
}

void unmarshal_error(Error& e, Msg* m) {
    e.code = m->code;
    e.id = m->id;

    if (!m->data) return;
    Vec* d = m->data;

    vec_read(d, e.key);
    vec_read(d, e.r);

    size_t len;
    vec_read(d, len);
    if (len < vec_remaining(d)) {
        e.msg.resize(len);
        vec_read(d, reinterpret_cast<unsigned char*>(e.msg.data()), len);
    }
}
