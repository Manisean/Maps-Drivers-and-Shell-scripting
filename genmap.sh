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
    temp_map=""
    for ((i=0; i<$height; i++)); do
        for ((j=0; j<$width; j++)); do
            if [ $((RANDOM%2)) -eq 0 ]; then
                temp_map+="#"
            else
                temp_map+="."
            fi
        done
        temp_map+="\n"
    done

    blnk_cnt=$(echo -e "${temp_map}" | tr -d "#\n" | wc -c)

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
