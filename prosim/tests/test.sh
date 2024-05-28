#!/bin/sh

echo ======================================================
echo ====================== TEST $1 ========================
echo ======================================================
if timeout 10 ./$2/$3 < tests/test.$1.in > tests/test.$1.out; then 
  if diff -b tests/test.$1.out tests/test.$1.expected > /dev/null; then
    echo PASSED
  else
    echo FAILED
    echo ======
    echo ______________Your_output______________ ____________Expected_Output_____________
   #echo +++++++++++++++++++++++++++++++++++++++ ++++++++++++++++++++++++++++++++++++++++
    diff -b -y -W 80 tests/test.$1.out tests/test.$1.expected
    echo =====================
    echo Your Output:
    echo ++++++++++++++++
    cat tests/test.$1.out
    echo =====================
    echo Expected Output:
    echo ++++++++++++++++
    cat tests/test.$1.expected
    exit 1
  fi
elif [ $? -eq 124 ]; then
  echo TIMEOUT
  exit 1
else 
  echo Abnormal program termination: the program crashed
  echo Exit code $?
  exit 1
fi
