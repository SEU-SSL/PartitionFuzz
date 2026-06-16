#!/bin/bash

set -exu

# 
TEST_CONFIGS=(
    # ：[PUT]:[HARNESS]:[PRE_ARG]:[POST_ARG]
    # "libpng:libpng_read_fuzzer::"
    # "libsndfile:sndfile_fuzzer::"
    # "libtiff:tiff_read_rgba_fuzzer::"
    "libxml2:libxml2_xml_read_memory_fuzzer::"
    # "lua:lua::"
    "openssl:asn1::"
    # "php:parser::"
    # "poppler:pdf_fuzzer::"
    # "sqlite3:sqlite3_fuzz::"
)

# 
export PARTITIONFUZZ="/fuzzer/dcgf/AFLplusplus-4.21c"
export START_CPU=0
export CYCLE=0
export REDIS_DB=0


# （）
for config in "${TEST_CONFIGS[@]}"; do
    # 
    IFS=':' read -r PUT HARNESS PRE_ARG POST_ARG <<< "$config"
    
    # ！
    export OUT="/out/new/partionfuzz/${CYCLE}-partionfuzz-${PUT}"
    [[ ! -d $OUT ]] && mkdir -p $OUT

    # CPU[4]
    export END_CPU=$((START_CPU + 3))
    # echo "${PUT} ${HARNESS} ${PRE_ARG} ${POST_ARG} ${OUT} ${END_CPU}"

    # Step 1: 
    echo "[*] Starting fuzzing for $PUT"

    # echo "timeout 24h nohup $PARTITIONFUZZ/afl-fuzz -M master -H $REDIS_DB -b $((START_CPU + 0)) -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/master.log 2>&1 &"

    timeout 24h nohup $PARTITIONFUZZ/afl-fuzz -M master -H $REDIS_DB -b $((START_CPU + 0)) -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/master.log 2>&1 &
    timeout 24h nohup $PARTITIONFUZZ/afl-fuzz -S clien1 -H $REDIS_DB -b $((START_CPU + 1)) -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien1.log 2>&1 &
    timeout 24h nohup $PARTITIONFUZZ/afl-fuzz -S clien2 -H $REDIS_DB -b $((START_CPU + 2)) -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien2.log 2>&1 &
    timeout 24h nohup $PARTITIONFUZZ/afl-fuzz -S clien3 -H $REDIS_DB -b $((START_CPU + 3)) -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien3.log 2>&1 &

    
    # CPU
    START_CPU=$((END_CPU + 1))
    REDIS_DB=$((REDIS_DB + 1))
done


# mkdir /out/partionfuzz/0-partionfuzz-openssl
# timeout 24h nohup /fuzzer/dcgf/AFLplusplus-4.21c/afl-fuzz -M master -H 0 -b 0 -i /fuzzing/afl/magma-openssl/openssl-lto/out/corpus-cmin/ -o /out/partionfuzz/0-partionfuzz-openssl -t 1000+ -m none -z -- /fuzzing/afl/magma-openssl/openssl-lto/out/server @@ > /out/partionfuzz/0-partionfuzz-openssl/master.log 2>&1 &
# timeout 24h nohup /fuzzer/dcgf/AFLplusplus-4.21c/afl-fuzz -S clien1 -H 0 -b 1 -i /fuzzing/afl/magma-openssl/openssl-lto/out/corpus-cmin/ -o /out/partionfuzz/0-partionfuzz-openssl -t 1000+ -m none -z -- /fuzzing/afl/magma-openssl/openssl-lto/out/server @@ > /out/partionfuzz/0-partionfuzz-openssl/master.log 2>&1 &
# timeout 24h nohup /fuzzer/dcgf/AFLplusplus-4.21c/afl-fuzz -S clien2 -H 0 -b 2 -i /fuzzing/afl/magma-openssl/openssl-lto/out/corpus-cmin/ -o /out/partionfuzz/0-partionfuzz-openssl -t 1000+ -m none -z -- /fuzzing/afl/magma-openssl/openssl-lto/out/server @@ > /out/partionfuzz/0-partionfuzz-openssl/master.log 2>&1 &
# timeout 24h nohup /fuzzer/dcgf/AFLplusplus-4.21c/afl-fuzz -S clien3 -H 0 -b 3 -i /fuzzing/afl/magma-openssl/openssl-lto/out/corpus-cmin/ -o /out/partionfuzz/0-partionfuzz-openssl -t 1000+ -m none -z -- /fuzzing/afl/magma-openssl/openssl-lto/out/server @@ > /out/partionfuzz/0-partionfuzz-openssl/master.log 2>&1 &