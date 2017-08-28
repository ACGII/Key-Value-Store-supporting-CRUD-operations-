#!/bin/bash

#################################################
# FILE NAME: KVStoreGrader.sh
#
# DESCRIPTION: Grader for MP2
#
# RUN PROCEDURE:
# $ chmod +x KVStoreGrader.sh
# $ ./KVStoreGrader.sh
#################################################

function contains () {
    local e
    for e in "${@:2}"
  	do
    	if [ "$e" == "$1" ]; then
      		echo 1
      		return 1;
    	fi
  	done
    echo 0
}

####
# Main function
####

verbose=1

###
# Global variables
###
SUCCESS=0
FAILURE=-1
RF=3
RFPLUSONE=4
CREATE_OPERATION="CREATE OPERATION"
CREATE_SUCCESS="create success"
GRADE=0
DELETE_OPERATION="DELETE OPERATION"
DELETE_SUCCESS="delete success"
DELETE_FAILURE="delete fail"
INVALID_KEY="invalidKey"
READ_OPERATION="READ OPERATION"
READ_SUCCESS="read success"
READ_FAILURE="read fail"
QUORUM=2
QUORUMPLUSONE=3
UPDATE_OPERATION="UPDATE OPERATION"
UPDATE_SUCCESS="update success"
UPDATE_FAILURE="update fail"

echo ""
#echo "############################"
#echo ""

echo ""
echo "############################"
echo " UPDATE TEST"
echo "############################"
echo ""

UPDATE_TEST1_STATUS="${FAILURE}"
UPDATE_TEST1_SCORE=0
UPDATE_TEST2_STATUS="${FAILURE}"
UPDATE_TEST2_SCORE=0
UPDATE_TEST3_PART1_STATUS="${FAILURE}"
UPDATE_TEST3_PART1_SCORE=0
UPDATE_TEST3_PART2_STATUS="${FAILURE}"
UPDATE_TEST3_PART2_SCORE=0
UPDATE_TEST4_STATUS="${FAILURE}"
UPDATE_TEST4_SCORE=0
UPDATE_TEST5_STATUS="${FAILURE}"
UPDATE_TEST5_SCORE=0

if [ "${verbose}" -eq 0 ]
then
    make clean > /dev/null 2>&1
    make > /dev/null 2>&1
    if [ $? -ne "${SUCCESS}" ]
    then
    	echo "COMPILATION ERROR !!!"
    	exit
    fi
    ./Application ./testcases/update.conf > /dev/null 2>&1
else
	make clean
	make
	if [ $? -ne "${SUCCESS}" ]
	then
    	echo "COMPILATION ERROR !!!"
    	exit
    fi
	./Application ./testcases/update.conf
fi

update_operations=`grep -i "${UPDATE_OPERATION}" dbg.log  | cut -d" " -f3 | tr -s ']' ' '  | tr -s '[' ' ' | sort`

cnt=1
for time in ${update_operations}
do
	if [ ${cnt} -eq 1 ]
	then
		echo "TEST 1: Update a key. Check for correct value being updated at least in quorum of replicas"
		update_op_test1_time="${time}"
		update_op_test1_key=`grep -i "${UPDATE_OPERATION}" dbg.log | grep "${update_op_test1_time}" | cut -d" " -f7`
		update_op_test1_value=`grep -i "${UPDATE_OPERATION}" dbg.log | grep "${update_op_test1_time}" | cut -d" " -f9`
		echo "update_op_test1_time=${update_op_test1_time} update_op_test1_key= ${update_op_test1_key} update_op_test1_value= ${update_op_test1_value}"
	elif [ ${cnt} -eq 2 ]
	then
		echo "TEST 2: Update a key after failing a replica. Check for correct value being updated at least in quorum of replicas"
		update_op_test2_time="${time}"
		update_op_test2_key=`grep -i "${UPDATE_OPERATION}" dbg.log | grep "${update_op_test2_time}" | cut -d" " -f7`
		update_op_test2_value=`grep -i "${UPDATE_OPERATION}" dbg.log | grep "${update_op_test2_time}" | cut -d" " -f9`
		echo "update_op_test2_time=${update_op_test2_time} update_op_test2_key= ${update_op_test2_key} update_op_test2_value= ${update_op_test2_value}"
	elif [ ${cnt} -eq 3 ]
	then
		echo "TEST 3 PART 1: Update a key after failing two replicas. Update should fail TIME = ${time}"
		update_op_test3_part1_time="${time}"
		update_op_test3_part1_key=`grep -i "${UPDATE_OPERATION}" dbg.log | grep "${update_op_test3_part1_time}" | cut -d" " -f7`
		update_op_test3_part1_value=`grep -i "${UPDATE_OPERATION}" dbg.log | grep "${update_op_test3_part1_time}" | cut -d" " -f9`
		echo "update_op_test3_part1_time=${update_op_test3_part1_time} update_op_test3_part1_key= ${update_op_test3_part1_key} update_op_test3_part1_value= ${update_op_test3_part1_value}"
	elif [ ${cnt} -eq 4 ]
	then
		echo "TEST 3 PART 2: Update the key after allowing stabilization protocol to kick in. Check for correct value being updated at least in quorum of replicas TIME=${time}"
		update_op_test3_part2_time="${time}"
		update_op_test3_part2_key=`grep -i "${UPDATE_OPERATION}" dbg.log | grep "${update_op_test3_part2_time}" | cut -d" " -f7`
		update_op_test3_part2_value=`grep -i "${UPDATE_OPERATION}" dbg.log | grep "${update_op_test3_part2_time}" | cut -d" " -f9`
		echo "update_op_test3_part2_time=${update_op_test3_part2_time} update_op_test3_part2_key= ${update_op_test3_part2_key} update_op_test3_part2_value= ${update_op_test3_part2_value}"
	elif [ ${cnt} -eq 5 ]
	then
		echo "TEST 4: Update a key after failing a non-replica. Check for correct value being updated at least in quorum of replicas TIME=${time}"
		update_op_test4_time="${time}"
		update_op_test4_key="${update_op_test1_key}"
		update_op_test4_value="${update_op_test1_value}"
		echo "update_op_test4_time=${update_op_test4_time} update_op_test4_key= ${update_op_test4_key} update_op_test4_value= ${update_op_test4_value}"
	elif [ ${cnt} -eq 6 ]
	then
		echo "TEST 5: Attempt update of an invalid key"
		update_op_test5_time="${time}"
	fi
	cnt=$(( ${cnt} + 1 ))
done

update_test1_success_count=0
update_test2_success_count=0
update_test3_part2_success_count=0
update_test4_success_count=0

update_successes=`grep -i "${UPDATE_SUCCESS}" dbg.log | grep ${update_op_test1_key} | grep ${update_op_test1_value} 2>/dev/null`
# echo "update_successes = ${update_successes}"
if [ "${update_successes}" ]
then
	while read success
	do
		time_of_this_success=`echo "${success}" | cut -d" " -f2 | tr -s '[' ' ' | tr -s ']' ' '`
		if [ "${time_of_this_success}" -ge "${update_op_test1_time}" -a "${time_of_this_success}" -lt "${update_op_test2_time}" ]
		then
			update_test1_success_count=`expr ${update_test1_success_count} + 1`
		elif [ "${time_of_this_success}" -ge "${update_op_test2_time}" -a "${time_of_this_success}" -lt "${update_op_test3_part1_time}" ] 
		then
			update_test2_success_count=`expr ${update_test2_success_count} + 1`
		elif [ "${time_of_this_success}" -ge "${update_op_test3_part2_time}" -a "${time_of_this_success}" -lt "${update_op_test4_time}" ]  
		then
			update_test3_part2_success_count=`expr ${update_test3_part2_success_count} + 1`
		elif [ "${time_of_this_success}" -ge "${update_op_test4_time}" ]
		then
			update_test4_success_count=`expr ${update_test4_success_count} + 1`
		fi
	done <<<"${update_successes}"
fi

update_test3_part1_fail_count=0
update_test5_fail_count=0

update_fails=`grep -i "${UPDATE_FAILURE}" dbg.log 2>/dev/null`
# echo "update_fails =${update_fails}"
if [ "${update_fails}" ]
then
	while read fail
	do
		time_of_this_fail=`echo "${fail}" | cut -d" " -f2 | tr -s '[' ' ' | tr -s ']' ' '`
		if [ "${time_of_this_fail}" -ge "${update_op_test3_part1_time}" -a "${time_of_this_fail}" -lt "${update_op_test3_part2_time}" ]
		then
			actual_key=`echo "${fail}" | grep "${update_op_test3_part1_key}" | wc -l`
			#echo "actual_key=${actual_key}"
			if [ "${actual_key}"  -eq 1 ]
			then	
				update_test3_part1_fail_count=`expr ${update_test3_part1_fail_count} + 1`
			fi
		elif [ "${time_of_this_fail}" -ge "${update_op_test5_time}" ]
		then
			actual_key=`echo "${fail}" | grep "${INVALID_KEY}" | wc -l`
			if [ "${actual_key}" -eq 1 ]
			then
				update_test5_fail_count=`expr ${update_test5_fail_count} + 1`
			fi
		fi
	done <<<"${update_fails}"
fi
echo "update_test1_success_count = ${update_test1_success_count} update_test2_success_count = ${update_test2_success_count} update_test3_part2_success_count= ${update_test3_part2_success_count} update_test4_success_count = ${update_test4_success_count}"
echo "update_test3_part1_fail_count= ${update_test3_part1_fail_count} update_test5_fail_count= ${update_test5_fail_count}"
echo "QUORUMPLUSONE= ${QUORUMPLUSONE} RFPLUSONE= ${RFPLUSONE}"


if [ "${update_test1_success_count}" -eq "${QUORUMPLUSONE}" -o "${update_test1_success_count}" -eq "${RFPLUSONE}" ]
then
	UPDATE_TEST1_STATUS="${SUCCESS}"
fi
if [ "${update_test2_success_count}" -eq "${QUORUMPLUSONE}" ]
then
	UPDATE_TEST2_STATUS="${SUCCESS}"
fi
if [ "${update_test3_part1_fail_count}" -eq 1 ]
then
	UPDATE_TEST3_PART1_STATUS="${SUCCESS}"
fi
if [ "${update_test3_part2_success_count}" -eq "${QUORUMPLUSONE}" -o "${update_test3_part2_success_count}" -eq "${RFPLUSONE}" ]
then
	UPDATE_TEST3_PART2_STATUS="${SUCCESS}"
fi
if [ "${update_test4_success_count}" -eq "${QUORUMPLUSONE}" -o "${update_test4_success_count}" -eq "${RFPLUSONE}" ]
then
	UPDATE_TEST4_STATUS="${SUCCESS}"
fi
if [ "${update_test5_fail_count}" -eq "${QUORUMPLUSONE}" -o "${update_test5_fail_count}" -eq "${RFPLUSONE}" ]
then
	UPDATE_TEST5_STATUS="${SUCCESS}"
fi

if [ "${UPDATE_TEST1_STATUS}" -eq "${SUCCESS}" ]
then
	UPDATE_TEST1_SCORE=3
fi
if [ "${UPDATE_TEST2_STATUS}" -eq "${SUCCESS}" ]
then
	UPDATE_TEST2_SCORE=9
fi
if [ "${UPDATE_TEST3_PART1_STATUS}" -eq "${SUCCESS}" ]
then
	UPDATE_TEST3_PART1_SCORE=9
fi
if [ "${UPDATE_TEST3_PART2_STATUS}" -eq "${SUCCESS}" ]
then
	UPDATE_TEST3_PART2_SCORE=10
fi
if [ "${UPDATE_TEST4_STATUS}" -eq "${SUCCESS}" ]
then
	UPDATE_TEST4_SCORE=6
fi
if [ "${UPDATE_TEST5_STATUS}" -eq "${SUCCESS}" ]
then
	UPDATE_TEST5_SCORE=3
fi

# Display score
echo "TEST 1 SCORE..................: ${UPDATE_TEST1_SCORE} / 3"
echo "TEST 2 SCORE..................: ${UPDATE_TEST2_SCORE} / 9"
echo "TEST 3 PART 1 SCORE..................: ${UPDATE_TEST3_PART1_SCORE} / 9"
echo "TEST 3 PART 2 SCORE..................: ${UPDATE_TEST3_PART2_SCORE} / 10"
echo "TEST 4 SCORE..................: ${UPDATE_TEST4_SCORE} / 6"
echo "TEST 5 SCORE..................: ${UPDATE_TEST5_SCORE} / 3"
# Add to grade
GRADE=`echo ${GRADE} ${UPDATE_TEST1_SCORE} | awk '{print $1 + $2}'`
GRADE=`echo ${GRADE} ${UPDATE_TEST2_SCORE} | awk '{print $1 + $2}'`
GRADE=`echo ${GRADE} ${UPDATE_TEST3_PART1_SCORE} | awk '{print $1 + $2}'`
GRADE=`echo ${GRADE} ${UPDATE_TEST3_PART2_SCORE} | awk '{print $1 + $2}'`
GRADE=`echo ${GRADE} ${UPDATE_TEST4_SCORE} | awk '{print $1 + $2}'`
GRADE=`echo ${GRADE} ${UPDATE_TEST5_SCORE} | awk '{print $1 + $2}'`

#echo ""
#echo "############################"
#echo " UPDATE TEST ENDS"
#echo "############################"
#echo ""

echo ""
echo "TOTAL GRADE: ${GRADE} / 90" 
echo ""
