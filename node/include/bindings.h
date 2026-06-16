#include "msgT.h"
#include "types.h"
#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif



typedef enum {
    CARD_NOTI           = 0,
    GIVE_NOTI           = 1,
    ACCEPTED_NOTI       = 2,
    DECLINED_NOTI       = 3,
    PERM_REQ_NOTI       = 4,
} API_CODES;


SZT* start_sz();
void block_stop(SZT* sz);
void batch_poll(SZT* sz, Msg** msg, size_t* len);
void recycle_msgs(SZT* sz, Msg* msg, size_t len);
void submit_msgs(SZT* sz, Msg* msg, size_t len);


// THESE CAN JUST BE HELPER FUNCTIONS...
// IF YOU WANT TO USE THEM YOU CAN BUT YOU DONT NEED TO...
// IF YOU CARE ABOUT PERFORMANCE DONT USE THESE
// Just implement them yourselve in your native language.

// AUTH
void get_challenge(SZT* sz, char** challenge, size_t* len);
SSID login_user(SZT* sz, const char* key, const char* signature);
SSID register_user(SZT* sz, const char* key, const char* signature);
void logout_user(SZT* sz, SSID ssid);


// THESE PASS MSGS TO SZT ACTORS
// POP FROM SZT MSG STORE AND PUSH TO DESTINATION

HashT new_file(SZT* sz, SSID ssid, size_t total_len);
bool file_chunk(SZT* sz, TrxID trx_id, const unsigned char* data, size_t len);
TrxID get_file(SZT* sz, SSID ssid, HashT* name);


HashT put_card(SZT* sz, SSID ssid, const unsigned char* data, size_t len, int cardType);
TrxID remove_card(SZT* sz, SSID ssid, const char* id);


// Broadcast Voucher for Card.
// list of pubkeys and a single card hash.
// I think you just add something like a broadcast id to the connection.
// so when the message finishes sending then you can access man.broadcast
//      to send to the next user or whatever.

// Request card[] from remote user
// AKA Redeem voucher || Temp Request


#ifdef __cplusplus
}
#endif
