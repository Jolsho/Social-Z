#ifdef __cplusplus
extern "C" {
#endif

typedef struct SZ SZ;

SZ* start_sz();

void block_stop(SZ* sz);


typedef struct SZ SZ;


// THESE PASS MSGS TO SZ ACTORS
// POP FROM SZ MSG STORE AND PUSH TO DESTINATION

void login_user(SZ* sz, const char* key, const char* password);


#ifdef __cplusplus
}
#endif
