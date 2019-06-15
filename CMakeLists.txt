
cmake_minimum_required(VERSION 2.8.12)
project(mai CXX C)
enable_language(CXX)
enable_language(C)
enable_language(ASM)

include_directories(
    include
    src)

set(BASE_SOURCES
    base/arena-utils.cc
    base/arena.cc
    base/arenas.cc
    base/ebr.cc
    base/hash.cc
    base/lock-group.cc
    base/slice.cc
    base/spin-locking.cc
    base/tls-test.cc
    base/varint-encoding.cc
    base/zone.cc
    core/bw-tree-memory-table.cc
    core/bytewise-comparator.cc
    core/decimal-v1.cc
    core/decimal-v2.cc
    core/delta-amend-iterator.cc
    core/env.cc
    core/error.cc
    core/internal-key-comparator.cc
    core/iterator-warpper.cc
    core/iterator.cc
    core/key-boundle.cc
    core/lru-cache-v1.cc
    core/memory-table.cc
    core/merging.cc
    core/ordered-memory-table.cc
    core/skip-list-test.cc
    core/unordered-memory-table.cc
    $<TARGET_OBJECTS:build_version>)

set(MAI_DB_SOURCES
    $<BASE_SOURCES>
    )
