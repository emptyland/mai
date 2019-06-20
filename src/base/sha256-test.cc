#include "gtest/gtest.h"

extern "C" {
#include "base/sha256.h"
}

TEST(Sha256Test, Speed) {
    SHA256_CTX ctx;
    
    const char data[] = "18781927416";
    BYTE hash[32];
    
    for (int i = 0; i < 1000; ++i) {
        sha256_init(&ctx);
        sha256_update(&ctx, reinterpret_cast<const BYTE *>(data), sizeof(data) - 1);
        sha256_final(&ctx, hash);
    }
}
