#!/bin/sh

sample_1() {
    ./main 3 6 10 0 0
}

sample_2() {
    ./main 3 5 3 0 0 &
    child=$!
    sleep 1.5
    kill -TSTP $child
    sleep 2
    kill -TSTP $child
    sleep 3
    kill -TSTP $child
    wait $child
}

sample_3() {
    ./main 3 0 0 200 -900 &
    child=$!
    sleep 0.5
    kill -TSTP $child
    wait $child
}
sample_4() {
    ./$1 2 2 -7161328 1 8 1 4 1 4 1 3 2 5232666 2 1321464 0 2 2 2051988 2 -3540999 2 -4296146 2 6096739 2 -8182490 2 -2517850 2 -4127329 2 7747040 6095054 &
    child=$!
    sleep 7.5
    kill -TSTP $child
    sleep 6.0
    kill -TSTP $child
    sleep 7.0
    kill -TSTP $child
    sleep 7.0
    kill -TSTP $child
    sleep 8.0
    kill -TSTP $child
    sleep 3.0
    kill -TSTP $child
    sleep 3.0
    kill -TSTP $child
    sleep 7.0
    kill -TSTP $child
    sleep 4.0
    kill -TSTP $child
    wait $child
}


print_help() {
    echo "usage: $0 [subtask]"
}

main() {
    case "$1" in
        1)
            sample_1
            ;;
        2)
            sample_2
            ;;
        3)
            sample_3
            ;;
        4)
            sample_4
            ;;
        *)
            print_help
            ;;
    esac
}

main "$1"
