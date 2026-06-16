#!/bin/bash

# 
TEST_CONFIGS=(
    # ：[PUT]:[HARNESS]:[PRE_ARG]:[POST_ARG]
    #"libpng:libpng_read_fuzzer::"
    #"libsndfile:sndfile_fuzzer::"
    #"libtiff:tiff_read_rgba_fuzzer::"
    #  "libxml2:libxml2_xml_read_memory_fuzzer::"
    # "lua:lua::"
    #  "openssl:asn1::"
    #  "php:parser::"
    "poppler:pdf_fuzzer::"
    "sqlite3:sqlite3_fuzz::"
)


export PARTITIONFUZZ="/fuzzer/dcgf/AFLplusplus-4.21c"
# export START_CPU=0
# export CYCLE=0
# export REDIS_DB=0


for CYCLE in {1..9}; do

  PIDS=()
  START_CPU=0
  REDIS_DB=0

  for config in "${TEST_CONFIGS[@]}"; do
      IFS=':' read -r PUT HARNESS PRE_ARG POST_ARG <<< "$config"
      
      export OUT="/out/partionfuzz-24/${CYCLE}-partionfuzz-${PUT}"
      [[ ! -d $OUT ]] && mkdir -p $OUT

      export END_CPU=$((START_CPU + 23))

      echo "[*] Starting fuzzing for $PUT"

      timeout 4h nohup $PARTITIONFUZZ/afl-fuzz -M master -H $REDIS_DB -b $((START_CPU + 0))  -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/master.log 2>&1 &
      PIDS+=($!)
      timeout 4h nohup $PARTITIONFUZZ/afl-fuzz -S clien1 -H $REDIS_DB -b $((START_CPU + 1))  -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien1.log 2>&1 &
      PIDS+=($!)
      timeout 4h nohup $PARTITIONFUZZ/afl-fuzz -S clien2 -H $REDIS_DB -b $((START_CPU + 2))  -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien2.log 2>&1 &
      PIDS+=($!)
      timeout 4h nohup $PARTITIONFUZZ/afl-fuzz -S clien3 -H $REDIS_DB -b $((START_CPU + 3))  -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien3.log 2>&1 &
      PIDS+=($!)
      timeout 4h nohup $PARTITIONFUZZ/afl-fuzz -S clien4 -H $REDIS_DB -b $((START_CPU + 4))  -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien4.log 2>&1 &
      PIDS+=($!)
      timeout 4h nohup $PARTITIONFUZZ/afl-fuzz -S clien5 -H $REDIS_DB -b $((START_CPU + 5))  -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien5.log 2>&1 &
      PIDS+=($!)
      timeout 4h nohup $PARTITIONFUZZ/afl-fuzz -S clien6 -H $REDIS_DB -b $((START_CPU + 6))  -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien6.log 2>&1 &
      PIDS+=($!)
      timeout 4h nohup $PARTITIONFUZZ/afl-fuzz -S clien7 -H $REDIS_DB -b $((START_CPU + 7))  -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien7.log 2>&1 &
      PIDS+=($!)
      timeout 4h nohup $PARTITIONFUZZ/afl-fuzz -S clien8 -H $REDIS_DB -b $((START_CPU + 8))  -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien8.log 2>&1 &
      PIDS+=($!)
      timeout 4h nohup $PARTITIONFUZZ/afl-fuzz -S clien9 -H $REDIS_DB -b $((START_CPU + 9))  -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien9.log 2>&1 &
      PIDS+=($!)
      timeout 4h nohup $PARTITIONFUZZ/afl-fuzz -S clien10 -H $REDIS_DB -b $((START_CPU + 10)) -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien10.log 2>&1 &
      PIDS+=($!)
      timeout 4h nohup $PARTITIONFUZZ/afl-fuzz -S clien11 -H $REDIS_DB -b $((START_CPU + 11)) -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien11.log 2>&1 &
      PIDS+=($!)
      timeout 4h nohup $PARTITIONFUZZ/afl-fuzz -S clien12 -H $REDIS_DB -b $((START_CPU + 12)) -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien12.log 2>&1 &
      PIDS+=($!)
      timeout 4h nohup $PARTITIONFUZZ/afl-fuzz -S clien13 -H $REDIS_DB -b $((START_CPU + 13)) -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien13.log 2>&1 &
      PIDS+=($!)
      timeout 4h nohup $PARTITIONFUZZ/afl-fuzz -S clien14 -H $REDIS_DB -b $((START_CPU + 14)) -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien14.log 2>&1 &
      PIDS+=($!)
      timeout 4h nohup $PARTITIONFUZZ/afl-fuzz -S clien15 -H $REDIS_DB -b $((START_CPU + 15)) -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien15.log 2>&1 &
      PIDS+=($!)
      timeout 4h nohup $PARTITIONFUZZ/afl-fuzz -S clien16 -H $REDIS_DB -b $((START_CPU + 16)) -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien16.log 2>&1 &
      PIDS+=($!)
      timeout 4h nohup $PARTITIONFUZZ/afl-fuzz -S clien17 -H $REDIS_DB -b $((START_CPU + 17)) -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien17.log 2>&1 &
      PIDS+=($!)
      timeout 4h nohup $PARTITIONFUZZ/afl-fuzz -S clien18 -H $REDIS_DB -b $((START_CPU + 18)) -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien18.log 2>&1 &
      PIDS+=($!)
      timeout 4h nohup $PARTITIONFUZZ/afl-fuzz -S clien19 -H $REDIS_DB -b $((START_CPU + 19)) -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien19.log 2>&1 &
      PIDS+=($!)
      timeout 4h nohup $PARTITIONFUZZ/afl-fuzz -S clien20 -H $REDIS_DB -b $((START_CPU + 20)) -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien20.log 2>&1 &
      PIDS+=($!)
      timeout 4h nohup $PARTITIONFUZZ/afl-fuzz -S clien21 -H $REDIS_DB -b $((START_CPU + 21)) -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien21.log 2>&1 &
      PIDS+=($!)
      timeout 4h nohup $PARTITIONFUZZ/afl-fuzz -S clien22 -H $REDIS_DB -b $((START_CPU + 22)) -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien22.log 2>&1 &
      PIDS+=($!)
      timeout 4h nohup $PARTITIONFUZZ/afl-fuzz -S clien23 -H $REDIS_DB -b $((START_CPU + 23)) -i /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/corpus-cmin/ -o $OUT -t 1000+ -m none -z -- /fuzzing/afl/magma-${PUT}/${PUT}-lto/out/${HARNESS} ${PRE_ARG} @@ ${POST_ARG} > $OUT/clien23.log 2>&1 &
      PIDS+=($!)

      START_CPU=$((END_CPU + 1))
      REDIS_DB=$((REDIS_DB + 1))
  done

  echo "[*] All fuzzers for cycle $CYCLE started with PIDs: ${PIDS[*]}"

  # 
  while true; do
      all_done=true
      for pid in "${PIDS[@]}"; do
          if kill -0 "$pid" 2>/dev/null; then
              all_done=false
              break
          fi
      done

      if $all_done; then
          echo "[*] All fuzzers for cycle $CYCLE exited. Proceeding to next configuration."
          break
      fi

      sleep 120  #  2 
  done


done