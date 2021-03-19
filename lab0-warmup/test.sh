#!/usr/local/cs/bin/bash

IN_CONTENT="Test input\nSecond line"
IN_FILE="lab0_check_input.txt" 
OUT_FILE="lab0_check_output.txt"
echo $IN_CONTENT > $IN_FILE

check_exit_status()
{
	if [ "$2" != "$3" ]
	then
		echo "ERROR: '${1}' exit with status '${3}' instead of '${2}'"
		exit 1
	fi
}

check_input_output()
{
	if [ "$1" != "$2" ]
	then
		echo "ERROR: '${3}' content not the same as '${4}' content"
		exit 1
	fi
}

RES=$(./lab0 < $IN_FILE)
RET=$(echo $?)
check_input_output "$RES" "$IN_CONTENT" "stdout" "stdin"
check_exit_status "./lab0" 0 $RET

RES=$(./lab0 --input=$IN_FILE)
RET=$(echo $?)
check_input_output "$RES" "$IN_CONTENT" "stdout" "--input file"
check_exit_status "./lab0 --input=${IN_FILE}" 0 $RET

./lab0 --output=$OUT_FILE <$IN_FILE
RET=$(echo $?)
OUT=$(cat $OUT_FILE)
check_input_output "$OUT" "$IN_CONTENT" "--output file" "stdin"
check_exit_status "./lab0 --output=$OUT_FILE <$IN_FILE" 0 $RET

CMD="./lab0 --input=$IN_FILE --output=$OUT_FILE"
$CMD
RET=$(echo $?)
IN=$(cat $IN_FILE)
OUT=$(cat $OUT_FILE)
check_input_output "$OUT" "$IN" "--output file" "--input file"
check_exit_status "$CMD" 0 $RET

CMD="./lab0 --unrecognized"
RET=$(${CMD} 2> /dev/null || echo $?)
check_exit_status "$CMD" 1 $RET

CMD="./lab0 --input='./lab0_check_nonexistent.txt'"
RET=$(${CMD} 2> /dev/null || echo $?)
check_exit_status "$CMD" 2 $RET

CMD="./lab0 --output='/dev/null' <${IN_FILE}"
RET=$(${CMD} 2> /dev/null || echo $?)
check_exit_status "$CMD" 3 $RET

CMD="./lab0 --segfault --catch"
RET=$(${CMD} 2> /dev/null || echo $?)
check_exit_status "$CMD" 4 $RET

rm $IN_FILE $OUT_FILE

echo "lab0 passes smoke-test"
