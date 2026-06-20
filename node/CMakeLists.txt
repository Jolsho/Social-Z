cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(sz LANGUAGES C CXX)

option(LEDGER   "Add Ledger Actor"  OFF)
option(RPC      "Add RPC Actor"     OFF)
option(DB       "Add DB Actor"      ON)
option(FS       "Add FS Actor"      ON)
option(LOG      "Add LOG Actor"     ON)
option(P2P      "Add P2P Actor"     ON)

option(CRYPTO   "Add Crypto Utils"  OFF)
option(LMDB     "Add LMDB Utils"    OFF)
option(HASH     "Add Hash Utils"    OFF)

if (LEDGER OR FS)
    set(LMDB      ON CACHE BOOL "" FORCE)
    set(CRYPTO    ON CACHE BOOL "" FORCE)
    set(HASH      ON CACHE BOOL "" FORCE)
elseif (DB)
    set(LMDB      ON CACHE BOOL "" FORCE)
    set(CRYPTO    ON CACHE BOOL "" FORCE)
elseif (P2P)
    set(CRYPTO    ON CACHE BOOL "" FORCE)
endif()

add_subdirectory(core)

if(LEDGER)      
    add_subdirectory(ledger) 
endif()

if(RPC)         
    add_subdirectory(http)  
endif()

if(DB)          
    add_subdirectory(db)    
endif()

if(FS)          
    add_subdirectory(fs)    
endif()

if(LOG)         
    add_subdirectory(log)   
endif()

if(P2P)         
    add_subdirectory(p2p)   
endif()

