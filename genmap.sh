#!/bin/bash

default_width=50
default_height=50

if [ $# -eq 1 ]; then
    width=$1
    height=$default_height
elif [ $# -eq 2 ]; then
    width=$1
    height=$2
else
    width=$default_width
    height=$default_height
fi

function genmap() {
    temp_map=$(shuf -r -n ${width} -e . "#")
    for ((i=1; i<$height; i++)); do
        temp_map="${temp_map}\n$(shuf -r -n ${width} -e . "#")"
    done

    blnk_cnt=$(echo -e "${temp_map}" | tr -d "#" | wc -c)

    nblank_percent=$((100 - (blnk_cnt * 100) / (width * height)))

    if [ $nblank_percent -ge 50 ]; then
        echo -e "${temp_map}"
        return 0
    else
        return 1
    fi
}

for ((i=1; i<=10; i++)); do
    if genmap; then
        exit 0
    fi
done

echo "Error: Could not generate map" >&2
exit 1

