
cmake_minimum_required(VERSION 2.8.12)
project(mai CXX C)
enable_language(CXX)
enable_language(C)
enable_language(ASM)

set(CMAKE_CXX_FLAGS "-std=c++17 -O0 -DDEBUG -D_DEBUG")

include_directories(
    include
    src
    third-party/include
    third-party/gflags
    third-party/glog/src)

set(BASE_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src/base)
set(CORE_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src/core)
set(ASM_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src/asm)
set(DB_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src/db)
set(PORT_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src/port)
set(TXN_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src/txn)
set(TABLE_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src/table)
set(LANG_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src/lang)
set(GLOG_SOURCE_DIR ${PROJECT_SOURCE_DIR}/third-party/glog)
set(GFLAGS_SOURCE_DIR ${PROJECT_SOURCE_DIR}/third-party/gflags)

set(BASE_SOURCES
    ${GLOG_SOURCE_DIR}/raw_logging.cc
    ${GLOG_SOURCE_DIR}/utilities.cc
    ${GLOG_SOURCE_DIR}/demangle.cc
    ${GLOG_SOURCE_DIR}/logging.cc
    ${GLOG_SOURCE_DIR}/signalhandler.cc
    ${GLOG_SOURCE_DIR}/symbolize.cc
    ${GLOG_SOURCE_DIR}/vlog_is_on.cc
    ${GFLAGS_SOURCE_DIR}/gflags.cc
    ${GFLAGS_SOURCE_DIR}/gflags_completions.cc
    ${GFLAGS_SOURCE_DIR}/gflags_reporting.cc
    ${BASE_SOURCE_DIR}/crc32.c
    ${BASE_SOURCE_DIR}/sha256.c
    ${BASE_SOURCE_DIR}/arena-utils.cc
    ${BASE_SOURCE_DIR}/arena.cc
    ${BASE_SOURCE_DIR}/arenas.cc
    ${BASE_SOURCE_DIR}/big-number.cc
    ${BASE_SOURCE_DIR}/ebr.cc
    ${BASE_SOURCE_DIR}/hash.cc
    ${BASE_SOURCE_DIR}/lock-group.cc
    ${BASE_SOURCE_DIR}/slice.cc
    ${BASE_SOURCE_DIR}/spin-locking.cc
    ${BASE_SOURCE_DIR}/varint-encoding.cc
    ${BASE_SOURCE_DIR}/zone.cc
    ${CORE_SOURCE_DIR}/bw-tree-memory-table.cc
    ${CORE_SOURCE_DIR}/bytewise-comparator.cc
    ${CORE_SOURCE_DIR}/decimal-test-v2.cc
    ${CORE_SOURCE_DIR}/decimal-v2.cc
    ${CORE_SOURCE_DIR}/delta-amend-iterator.cc
    ${CORE_SOURCE_DIR}/env.cc
    ${CORE_SOURCE_DIR}/error.cc
    ${CORE_SOURCE_DIR}/internal-key-comparator.cc
    ${CORE_SOURCE_DIR}/iterator-warpper.cc
    ${CORE_SOURCE_DIR}/iterator.cc
    ${CORE_SOURCE_DIR}/key-boundle.cc
    ${CORE_SOURCE_DIR}/lru-cache-v1.cc
    ${CORE_SOURCE_DIR}/memory-table.cc
    ${CORE_SOURCE_DIR}/merging.cc
    ${CORE_SOURCE_DIR}/ordered-memory-table.cc
    ${CORE_SOURCE_DIR}/unordered-memory-table.cc
    ${DB_SOURCE_DIR}/column-family.cc
    ${DB_SOURCE_DIR}/compaction-impl.cc
    ${DB_SOURCE_DIR}/config.cc
    ${DB_SOURCE_DIR}/db-impl.cc
    ${DB_SOURCE_DIR}/db-iterator.cc
    ${DB_SOURCE_DIR}/factory.cc
    ${DB_SOURCE_DIR}/files.cc
    ${DB_SOURCE_DIR}/table-cache.cc
    ${DB_SOURCE_DIR}/version.cc
    ${DB_SOURCE_DIR}/write-ahead-log.cc
    ${DB_SOURCE_DIR}/write-batch.cc
    ${PORT_SOURCE_DIR}/env-posix.cc
    ${PORT_SOURCE_DIR}/file-posix.cc
    ${TABLE_SOURCE_DIR}/block-cache.cc
    ${TABLE_SOURCE_DIR}/block-iterator.cc
    ${TABLE_SOURCE_DIR}/data-block-builder.cc
    ${TABLE_SOURCE_DIR}/key-bloom-filter.cc
    ${TABLE_SOURCE_DIR}/plain-block-builder.cc
    ${TABLE_SOURCE_DIR}/s1-table-builder.cc
    ${TABLE_SOURCE_DIR}/s1-table-reader.cc
    ${TABLE_SOURCE_DIR}/sst-table-builder.cc
    ${TABLE_SOURCE_DIR}/sst-table-reader.cc
    ${TABLE_SOURCE_DIR}/table.cc
    ${TXN_SOURCE_DIR}/optimism-transaction-db.cc
    ${TXN_SOURCE_DIR}/optimism-transaction.cc
    ${TXN_SOURCE_DIR}/pessimistic-transaction-db.cc
    ${TXN_SOURCE_DIR}/pessimistic-transaction.cc
    ${TXN_SOURCE_DIR}/transaction-base.cc
    ${TXN_SOURCE_DIR}/transaction-db.cc
    ${TXN_SOURCE_DIR}/transaction-lock-mgr.cc
    ${TXN_SOURCE_DIR}/write-batch-with-index.cc
    ${LANG_SOURCE_DIR}/ast.cc
    ${LANG_SOURCE_DIR}/bytecode-array-builder.cc
    ${LANG_SOURCE_DIR}/bytecode-generator.cc
    ${LANG_SOURCE_DIR}/bytecode.cc
    ${LANG_SOURCE_DIR}/channel.cc
    ${LANG_SOURCE_DIR}/compiler.cc
    ${LANG_SOURCE_DIR}/coroutine.cc
    ${LANG_SOURCE_DIR}/factory.cc
    ${LANG_SOURCE_DIR}/function-template-x64.cc
    ${LANG_SOURCE_DIR}/handle.cc
    ${LANG_SOURCE_DIR}/heap.cc
    ${LANG_SOURCE_DIR}/isolate.cc
    ${LANG_SOURCE_DIR}/lexer.cc
    ${LANG_SOURCE_DIR}/machine.cc
    ${LANG_SOURCE_DIR}/macro-assembler-x64.cc
    ${LANG_SOURCE_DIR}/metadata-space.cc
    ${LANG_SOURCE_DIR}/metadata.cc
    ${LANG_SOURCE_DIR}/page.cc
    ${LANG_SOURCE_DIR}/parser.cc
    ${LANG_SOURCE_DIR}/runtime.cc
    ${LANG_SOURCE_DIR}/scheduler.cc
    ${LANG_SOURCE_DIR}/space.cc
    ${LANG_SOURCE_DIR}/stable-space-builder.cc
    ${LANG_SOURCE_DIR}/syntax.cc
    ${LANG_SOURCE_DIR}/token.cc
    ${LANG_SOURCE_DIR}/type-checker.cc
    ${LANG_SOURCE_DIR}/value.cc
    ${ASM_SOURCE_DIR}/x64/asm-x64.cc
    )

set(UNIT_TEST_SOURCES
    #${PROJECT_SOURCE_DIR}/src/asm/x64/asm-x64-test.cc
    ${PROJECT_SOURCE_DIR}/src/core/merging-test.cc
    ${PROJECT_SOURCE_DIR}/src/core/hash-map-v2-test.cc
    ${PROJECT_SOURCE_DIR}/src/core/lru-cache-test.cc
    ${PROJECT_SOURCE_DIR}/src/core/hash-map-v1-test.cc
    ${PROJECT_SOURCE_DIR}/src/core/error-test.cc
    ${PROJECT_SOURCE_DIR}/src/core/skip-list-test.cc
    ${PROJECT_SOURCE_DIR}/src/core/key-boundle-test.cc
    ${PROJECT_SOURCE_DIR}/src/core/ordered-memory-table-test.cc
    ${PROJECT_SOURCE_DIR}/src/core/bw-tree-memory-table-test.cc
    ${PROJECT_SOURCE_DIR}/src/core/bw-tree-test.cc
    ${PROJECT_SOURCE_DIR}/src/core/pipeline-queue-test.cc
    ${PROJECT_SOURCE_DIR}/src/core/unordered-memory-table-test.cc
    ${PROJECT_SOURCE_DIR}/src/txn/pessimistic-transaction-db-test.cc
    ${PROJECT_SOURCE_DIR}/src/txn/transaction-db-test.cc
    ${PROJECT_SOURCE_DIR}/src/txn/write-batch-with-index-test.cc
    ${PROJECT_SOURCE_DIR}/src/test/table-test.cc
    ${PROJECT_SOURCE_DIR}/src/lang/stable-space-builder-test.cc
    ${PROJECT_SOURCE_DIR}/src/lang/coroutine-test.cc
    ${PROJECT_SOURCE_DIR}/src/lang/token-test.cc
    ${PROJECT_SOURCE_DIR}/src/lang/bytecode-test.cc
    ${PROJECT_SOURCE_DIR}/src/lang/page-test.cc
    ${PROJECT_SOURCE_DIR}/src/lang/space-test.cc
    ${PROJECT_SOURCE_DIR}/src/lang/metadata-space-test.cc
    ${PROJECT_SOURCE_DIR}/src/lang/bytecode-generator-test.cc
    ${PROJECT_SOURCE_DIR}/src/lang/compiler-test.cc
    ${PROJECT_SOURCE_DIR}/src/lang/scheduler-test.cc
    ${PROJECT_SOURCE_DIR}/src/lang/channel-test.cc
    ${PROJECT_SOURCE_DIR}/src/lang/lexer-test.cc
    ${PROJECT_SOURCE_DIR}/src/lang/parser-test.cc
    ${PROJECT_SOURCE_DIR}/src/lang/value-test.cc
    ${PROJECT_SOURCE_DIR}/src/lang/handle-test.cc
    ${PROJECT_SOURCE_DIR}/src/lang/isolate-test.cc
    ${PROJECT_SOURCE_DIR}/src/lang/type-checker-test.cc
    ${PROJECT_SOURCE_DIR}/src/table/sst-table-reader-test.cc
    ${PROJECT_SOURCE_DIR}/src/table/s1-table-builder-test.cc
    ${PROJECT_SOURCE_DIR}/src/table/s1-table-reader-test.cc
    ${PROJECT_SOURCE_DIR}/src/table/data-block-builder-test.cc
    ${PROJECT_SOURCE_DIR}/src/table/filter-block-builder-test.cc
    ${PROJECT_SOURCE_DIR}/src/table/sst-table-builder-test.cc
    ${PROJECT_SOURCE_DIR}/src/db/files-test.cc
    ${PROJECT_SOURCE_DIR}/src/db/version-test.cc
    ${PROJECT_SOURCE_DIR}/src/db/config-test.cc
    ${PROJECT_SOURCE_DIR}/src/db/db-impl-test.cc
    ${PROJECT_SOURCE_DIR}/src/db/compaction-test.cc
    ${PROJECT_SOURCE_DIR}/src/port/file-test.cc
    ${PROJECT_SOURCE_DIR}/src/base/ebr-test.cc
    ${PROJECT_SOURCE_DIR}/src/base/sha256-test.cc
    ${PROJECT_SOURCE_DIR}/src/base/reference-count-test.cc
    ${PROJECT_SOURCE_DIR}/src/base/varint-encoding-test.cc
    ${PROJECT_SOURCE_DIR}/src/base/tls-test.cc
    ${PROJECT_SOURCE_DIR}/src/base/slice-test.cc
    ${PROJECT_SOURCE_DIR}/src/base/thread-pool-test.cc
    ${PROJECT_SOURCE_DIR}/src/base/io-utils-test.cc
    ${PROJECT_SOURCE_DIR}/src/base/spin-locking-test.cc
    ${PROJECT_SOURCE_DIR}/src/base/arenas-test.cc
    ${PROJECT_SOURCE_DIR}/src/base/arena-utils-test.cc
    ${PROJECT_SOURCE_DIR}/src/test/test-main.cc
    ${PROJECT_SOURCE_DIR}/third-party/gtest/gtest-all.cc)

set(DB_BENCHMARK_SOURCES
    ${PROJECT_SOURCE_DIR}/benchmark/benchmark-main.cc)

# libraries:
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build)
set(BASE_LIB_NAME mai_base)

add_library(${BASE_LIB_NAME} STATIC ${BASE_SOURCES})
target_link_libraries(${BASE_LIB_NAME} pthread dl)

# unit-tests:
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build)
add_executable(all-tests ${UNIT_TEST_SOURCES})
target_link_libraries(all-tests pthread dl ${BASE_LIB_NAME})

# db-benchmark
add_executable(db-benchmark ${DB_BENCHMARK_SOURCES})
target_link_libraries(db-benchmark pthread dl ${BASE_LIB_NAME})