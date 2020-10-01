#!/bin/bash
#
# About: Script to run turbostat only store only the interesting data
# within a file. The file will be converted into .cvs format at the end
#

# Turbostat params
CPU_MASK="2" # Core 1
CPU="0"      # Core to measure stats from
NUM_IT="3"
IT_DUR="5"
SAVE_FOLDER="turbostat_stats"
FILE_PATH=$(pwd)
FILE_NAME="out"

# Get @NUM_IT and @IT_DUR, if available
while :; do
    case $1 in
    -n)
        if [ "$2" ]; then
            NUM_IT="$2"
            shift
        fi
        ;;
    -t)
        if [ "$2" ]; then
            IT_DUR="$2"
            shift
        fi
        ;;
    -f)
        if [ "$2" ]; then
            FILE_NAME="$2"
            shift
        fi
        ;;
    -s)
        if [ "$2" ]; then
            SAVE_FOLDER="$2"
            shift
        fi
        ;;
    *)
        break
        ;;
    esac
    shift
done

mkdir $SAVE_FOLDER 2>/dev/null
FILE_NAME="${FILE_NAME}_${NUM_IT}-it_${IT_DUR}-s.ts"
FILE_PATH="${FILE_PATH}/${SAVE_FOLDER}/${FILE_NAME}"
echo $FILE_PATH

# Start turbostat
# taskset $CPU_MASK sudo turbostat --quiet --cpu $CPU --out $FILE_PATH --num_iterations $NUM_IT --show Core,Avg_MHz,PkgTmp,PkgWatt,CorWatt --interval $IT_DUR
# Reduced stats
taskset $CPU_MASK sudo turbostat --quiet --cpu $CPU --out $FILE_PATH --num_iterations $NUM_IT --show Avg_MHz,CorWatt --interval $IT_DUR
# Print, don't save
# taskset $CPU_MASK sudo turbostat --quiet --cpu $CPU --num_iterations $NUM_IT --show Core,Avg_MHz,PkgTmp,PkgWatt,CorWatt --interval $IT_DUR

# Convert output file to csv format
sed -i -e 's/\t/,/g' $FILE_PATH
# sed -i -e 's/Core,Avg_MHz,PkgTmp,PkgWatt,CorWatt\n//g'
