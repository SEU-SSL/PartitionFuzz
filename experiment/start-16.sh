#!/bin/bash

set -exu


TEST_CONFIGS=(
    # ：[PUT]:[HARNESS]:[PRE_ARG]:[POST_ARG]
    # "libpng:libpng_read_fuzzer::"
    # "libsndfile:sndfile_fuzzer::"
    # "libtiff:tiff_read_rgba_fuzzer::"
    # "libxml2:libxml2_xml_read_memory_fuzzer::"
    # "lua:lua::"
    # "openssl:asn1::"
    # "php:parser::"
    "poppler:pdf_fuzzer::"
    # "sqlite3:sqlite3_fuzz::"
)

# 
export PARTITIONFUZZ="/fuzzer/dcgf/AFLplusplus-4.21c"
export START_CPU=16
export CYCLE=0
export REDIS_DB=15


# （）
for config in "${TEST_CONFIGS[@]}"; do
    # 
    IFS=':' read -r PUT HARNESS PRE_ARG POST_ARG <<< "$config"
    
    # ！
    export OUT="/out/partionfuzz-16/${CYCLE}-partionfuzz-${PUT}"
    [[ ! -d $OUT ]] && mkdir -p $OUT

    # CPU[4]
    export END_CPU=$((START_CPU + 15))
    # echo "${PUT} ${HARNESS} ${PRE_ARG} ${POST_ARG} ${OUT} ${END_CPU}"

    # Step 1: 
    echo "[*] Starting fuzzing for $PUT"

    timeout 24h nohup $PARTITIONFUZZ/afl-fuzz -M master  -H $REDIS_DB -b $((START_CPU + 0))  -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/master.log 2>&1 &
    timeout 24h nohup $PARTITIONFUZZ/afl-fuzz -S clien1  -H $REDIS_DB -b $((START_CPU + 1))  -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien1.log 2>&1 &
    timeout 24h nohup $PARTITIONFUZZ/afl-fuzz -S clien2  -H $REDIS_DB -b $((START_CPU + 2))  -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien2.log 2>&1 &
    timeout 24h nohup $PARTITIONFUZZ/afl-fuzz -S clien3  -H $REDIS_DB -b $((START_CPU + 3))  -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien3.log 2>&1 &
    timeout 24h nohup $PARTITIONFUZZ/afl-fuzz -S clien4  -H $REDIS_DB -b $((START_CPU + 4))  -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien4.log 2>&1 &
    timeout 24h nohup $PARTITIONFUZZ/afl-fuzz -S clien5  -H $REDIS_DB -b $((START_CPU + 5))  -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien5.log 2>&1 &
    timeout 24h nohup $PARTITIONFUZZ/afl-fuzz -S clien6  -H $REDIS_DB -b $((START_CPU + 6))  -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien6.log 2>&1 &
    timeout 24h nohup $PARTITIONFUZZ/afl-fuzz -S clien7  -H $REDIS_DB -b $((START_CPU + 7))  -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien7.log 2>&1 &
    timeout 24h nohup $PARTITIONFUZZ/afl-fuzz -S clien8  -H $REDIS_DB -b $((START_CPU + 8))  -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien8.log 2>&1 &
    timeout 24h nohup $PARTITIONFUZZ/afl-fuzz -S clien9  -H $REDIS_DB -b $((START_CPU + 9))  -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien9.log 2>&1 &
    timeout 24h nohup $PARTITIONFUZZ/afl-fuzz -S clien10 -H $REDIS_DB -b $((START_CPU + 10)) -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien10.log 2>&1 &
    timeout 24h nohup $PARTITIONFUZZ/afl-fuzz -S clien11 -H $REDIS_DB -b $((START_CPU + 11)) -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien11.log 2>&1 &
    timeout 24h nohup $PARTITIONFUZZ/afl-fuzz -S clien12 -H $REDIS_DB -b $((START_CPU + 12)) -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien12.log 2>&1 &
    timeout 24h nohup $PARTITIONFUZZ/afl-fuzz -S clien13 -H $REDIS_DB -b $((START_CPU + 13)) -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien13.log 2>&1 &
    timeout 24h nohup $PARTITIONFUZZ/afl-fuzz -S clien14 -H $REDIS_DB -b $((START_CPU + 14)) -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien14.log 2>&1 &
    timeout 24h nohup $PARTITIONFUZZ/afl-fuzz -S clien15 -H $REDIS_DB -b $((START_CPU + 15)) -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien15.log 2>&1 &

    
    # CPU
    START_CPU=$((END_CPU + 1))
    REDIS_DB=$((REDIS_DB + 1))
done
