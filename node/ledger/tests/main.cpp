/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */


#include <string>
#include "tests.h"

int main(int argc, char* argv[]) {
    printf("\noptions == {key_sig, kzg, state}\n\n");
    printf("=====================================\n");

    if (argc > 1) {
        std::string a = argv[1];
        if (a == "key_sig") {
            main_key_sig();

        } else if (a == "kzg") {
            main_kzg();

        } else if (a == "state") {
            main_state_trie();
        }
        return 0;
    }


    // KEY_N_SIG
    main_key_sig();

    // KZG
    main_kzg();

    // STATE_TRIE
    main_state_trie();

    return 0;

}
