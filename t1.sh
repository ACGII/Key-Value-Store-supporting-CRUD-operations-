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

verbose=$(contains "-v" "$@")

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
verbose=1

echo ""
echo "############################"
echo " CREATE TEST"
echo "############################"
echo ""

CREATE_TEST_STATUS="${SUCCESS}"
CREATE_TEST_SCORE=0

if [ "${verbose}" -eq 0 ]
then
    make clean > /dev/null 2>&1
    make > /dev/null 2>&1
    if [ $? -ne "${SUCCESS}" ]
    then
    	echo "COMPILATION ERROR !!!"
    	exit
    fi
    ./Application ./testcases/create.conf > /dev/null 2>&1
else
	make clean
	make
	if [ $? -ne "${SUCCESS}" ]
	then
    	echo "COMPILATION ERROR !!!"
    	exit
    fi
	./Application ./testcases/create.conf
fi

echo "TEST 1: Create 3 replicas of every key"

create_count=`grep -i "${CREATE_OPERATION}" dbg.log | wc -l`
create_success_count=`grep -i "${CREATE_SUCCESS}" dbg.log | wc -l`
expected_count=$(( ${create_count} * ${RFPLUSONE} ))


echo "cc=${create_count}  csc=${create_success_count}  ec=${expected_count}"
if [ ${create_success_count} -ne ${expected_count} ]
then 
	CREATE_TEST_STATUS="${FAILURE}"
else
	keys=`grep -i "${CREATE_OPERATION}" dbg.log | cut -d" " -f7`
	for key in ${keys}
	do 
		key_create_success_count=`grep -i "${CREATE_SUCCESS}" dbg.log | grep "${key}" | wc -l`
		if [ "${key_create_success_count}" -ne "${RFPLUSONE}" ]
		then
			CREATE_TEST_STATUS="${FAILURE}"
			break
		fi
	done
fi

if [ "${CREATE_TEST_STATUS}" -eq "${SUCCESS}" ] 
then
	CREATE_TEST_SCORE=3	
fi

# Display score
echo "TEST 1 SCORE..................: ${CREATE_TEST_SCORE} / 3"
# Add to grade
GRADE=$(( ${GRADE} + ${CREATE_TEST_SCORE} ))

#echo ""
#echo "############################"
#echo " CREATE TEST ENDS"
#echo "############################"
#echo ""

