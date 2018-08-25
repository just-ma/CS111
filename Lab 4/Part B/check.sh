rm -f foo
./lab4b --invalid 1> /dev/null
if [ $? -ne 1 ]
then
	echo "Test 1/5 Failed: Invalid argument undetected"
else
	echo "Test 1/5 Passed: Invalid argument detected"
fi

./lab4b --period=2 --scale=C --log=foo <<EOF
SCALE=F
PERIOD=1
START
STOP
LOG bar
OFF
EOF
ret=$?
if [ $ret -ne 0 ]
then
	echo "Test 2/5 Failed: Valid arguments failed"
else
	echo "Test 2/5 Passed: Valid arguments passed"
fi

if [ ! -s foo ]
then
	echo "Test 3/5 Failed: foo file not created"
	echo "Test 4/5 Failed: foo file not created"
else
	echo "Test 3/5 Passed: foo file created"
	errors=0
	for element in SCALE=F PERIOD=1 START STOP OFF SHUTDOWN "LOG bar"
	do
		grep "$element" foo &> /dev/null
		if [ $? -ne 0 ]
		then
			echo "Element $element not recorded in foo file"
			let errors+=1
		else
			echo "Element $element recorded in foo file"
		fi
	done
	if [ $errors -gt 0 ]
	then
		echo "Test 4/5 Failed: Not all elements recorded in foo file"
	else
		echo "Test 4/5 Passed: All elements recorded in foo file"
	fi
fi

egrep '[0-9][0-9]:[0-9][0-9]:[0-9][0-9] [0-9]?[0-9].[0-9]' foo 1> /dev/null
if [ $? -ne 0 ]
then
	echo "Test 5/5 Failed: Invalid time and temperature format"
else
	echo "Test 5/5 Passed: Valid time and temperature format"
fi


