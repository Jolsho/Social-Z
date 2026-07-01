#include "sz/client.h"
#include "sz/codec.h"
#include "sz/crypto.h"

int defined_func() {
    Key k;
    HashT h;
    Signature sig;
    sign_hash(&k, &sig, &h);
    return 1;
}

