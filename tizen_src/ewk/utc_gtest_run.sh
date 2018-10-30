#!/bin/bash

##  @file   utc_gtest_run.sh
##  @author Jongsoo Yoon <join.yoon@samsung.com>, Mikolaj Czyzewski <m.czyzewski@samsung.com>
##  @date   2014-02-20
##  @mod    2015-01-16
##  @brief  This shell-script file is made to test all unit test cases automatically for Chromium EFL API

# Tests can be run in a four different ways:
# -single test
#   ./utc_gtest_run.sh -s utc_blink_ewk_view_visibility_set.POS_TEST1
# -set of tests
#   ./utc_gtest_run.sh -s utc_blink_ewk_view_user_agent*
# -tests listed in the file (each test_name/API_function in separate line)
#   ./utc_gtest_run.sh -l file
# -all tests
#   ./utc_gtest_run.sh -a
#
# To generate list of all tests:
#   ./utc_gtest_run.sh -g     (to stdout)
#   ./utc_gtest_run.sh -g > all_tests.txt
#   ./utc_gtest_run.sh -g | grep ewk_view > ewk_view_tests.txt
#
# The test results are generated in /opt/usr/utc_results/unittest-result-{DATE}-{TIME}
# To parse the results:
#   ./utc_gtest_run.sh -p /opt/usr/utc_results/unittest-result-{DATE}-{TIME}
# This creates file unittest-result-{DATE}-{TIME}.txt in current directory.

function usage {
  cat << EOF
Usage: utc_gtest_run.sh [OPTION]
  -a       run all tests
  -g       generate list of all tests
  -h       show this help
  -l file  run all tests from a text file (every test pattern in separate line)
  -p dir   search directory for xml files, parse them and print results
  -s name  run single test
EOF
}

function getHostArch() {
  echo $(uname -m | sed -e \
        's/i.86/ia32/;s/x86_64/x64/;s/amd64/x64/;s/arm.*/arm/;s/i86pc/ia32/;s/aarch64/arm64/')
}

#  Set target environment depending on the host architecture
function init {
  case "$(getHostArch)" in
  x64)
    UTC_EXEC="ewk_unittests"
    tmp_dir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
    CHROMIUM_DIR=$(dirname $tmp_dir)
    echo $CHROMIUM_DIR
    binaries=($(find "$CHROMIUM_DIR/out.x64" -type f -name "$UTC_EXEC"))
    num_of_bin=${#binaries[@]}

    case $num_of_bin in
    0)
      echo "=== No $UTC_EXEC binary found. Exiting ==="
      exit 2
      ;;
    1)
      echo "=== Only one utc binary found: ==="
      echo "=== ${binaries[@]} ==="
      EXEC=$binaries
      ;;
    *)
      echo "=== Please choose utc binary to run: ==="
      i=1
      for bin in ${binaries[@]}; do
        echo "$i) $bin"
        ((i++))
      done
      while true; do
        read number
        if [[ $number =~ ^-?[0-9]+$ ]] && [ $number -ge 1 ] && [ $number -le $num_of_bin ]; then
          break
        else
          echo "=== Please enter an integer from 1 to $num_of_bin ==="
        fi
      done
      EXEC="${binaries[$number-1]}"
      echo "=== Running $EXEC ==="
      ;;
    esac

    UTC_RESULTS_ROOT_DIR=/tmp/utc_results

    CHROMIUM_EFL_LIBDIR="$(dirname $EXEC)/lib"
    CHROMIUM_EFL_DEPENDENCIES_LIBDIR="$CHROMIUM_DIR/out.x64/Dependencies/Root/lib64"
    export LD_LIBRARY_PATH="$CHROMIUM_EFL_DEPENDENCIES_LIBDIR:$CHROMIUM_EFL_LIBDIR:${LD_LIBRARY_PATH}"
    export UTC_RESOURCE_PATH="$CHROMIUM_DIR/ewk/unittest/resources"
    EXEC="$EXEC --disable-gpu-driver-bug-workarounds"
    ;;

  arm | arm64)
    UTC_EXEC=ewk_unittests
    EXEC="/opt/usr/utc_exec/$UTC_EXEC"
    chmod +x $EXEC
    UTC_RESULTS_ROOT_DIR=/opt/usr/utc_results
    killall -s QUIT lockscreen
    ;;

  *)
    echo "=== Unsupported architecture ==="
    ;;
  esac # getHostArch

  FILE_DATE=$(date +%y%m%d_%H%M%S)
  UTC_RESULT_DIR="$UTC_RESULTS_ROOT_DIR/unittest-result-$FILE_DATE"

  OPTIND=1
  shift $((OPTIND-1))
  [ "$1" = "--" ] && shift

  echo "=== EXEC = $EXEC ==="
  echo "=== UTC_RESULT_DIR = $UTC_RESULT_DIR ==="

}

function deinit {
  echo -e "=== Test results were saved in \033[0;34m$UTC_RESULT_DIR\033[0m ==="
}

# Parse single xml file and print result
function parse_xml () {
  xml_file=$1
  if [ ! -e "$xml_file" ]; then
    echo "=== File $xml_file doesn't exist ==="
    return
  fi

  testsuite_count=$(xmllint --xpath "count(//testsuites/testsuite)" $xml_file)
  for (( i=1; i<=$testsuite_count; i++ )); do
    testcase_count=$(xmllint --xpath "count(//testsuites/testsuite[$i]/testcase)" $xml_file)
    for (( j=1; j<=$testcase_count; j++ )); do
      classname=$(xmllint --xpath "string(//testsuites/testsuite[$i]/testcase[$j]/@classname)" $xml_file)
      name=$(xmllint --xpath "string(//testsuites/testsuite[$i]/testcase[$j]/@name)" $xml_file)
      message=$(xmllint --xpath "string(//testsuites/testsuite[$i]/testcase[$j]/failure/@message)" $xml_file)
      message=$(echo $message | tr -d "\n\r")
      if [ -n "$message" ]; then
        echo "FAILURE $classname.$name $message"
      else
        echo "SUCCESS $classname.$name"
      fi
    done
  done
}

# Search directory for xml files, parse them and print results
function parse_dir () {
  xml_dir=$1
 pushd $xml_dir &>/dev/null
  echo "=== Parsing files in: `pwd` ==="

  for file in *.xml; do
    echo "$(parse_xml $file)"
  done

  popd &>/dev/null
}

# Generate list of all tests and print it to stdout
function generate_tests_list () {
  if [ -n "$1" ]; then
    filter="*$1*"
  else
    filter="*"
  fi
  list=$($EXEC --gtest_list_tests --gtest_filter="$filter" | egrep "^[a-zA-Z ]")
  utc_line=""
  result=""

  while read line; do
    if [[ $line == utc_* ]]; then
      utc_line=$line
    else
      echo "$utc_line$line"
    fi
  done <<< "$list"
}

# Run single test
function run_test () {
  test_name=$1
  if [ -n "$test_name" ]; then
    echo "=== Running test $test_name ==="
    file="$UTC_RESULT_DIR/$test_name.xml"

    # Generate fake xml file that tells about possible segfault
    if [ ! -f $file ]; then
      IFS='.' read -ra name <<< "$test_name"
      echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" >> $file
      echo "<testsuites tests=\"1\" failures=\"0\" disabled=\"0\" errors=\"1\" time=\"0.1\" name=\"AllTests\">" >> $file
      echo "  <testsuite name=\"${name[0]}\" tests=\"1\" failures=\"0\" disabled=\"0\" errors=\"1\" time=\"0.1\">" >> $file
      echo "    <testcase name=\"${name[1]}\" status=\"run\" time=\"0.1\" classname=\"${name[0]}\">" >> $file
      echo "      <failure message=\"SIGSEGV\" type=\"\"></failure>" >> $file
      echo "    </testcase>" >> $file
      echo "  </testsuite>" >> $file
      echo "</testsuites>" >> $file
    fi

    $EXEC --gtest_output="xml:$UTC_RESULT_DIR/$test_name.xml" --gtest_filter="$test_name"
    echo "=== Finished test $test_name ==="
    return 0
  else
    echo "=== No test name specified ==="
    return 1
  fi
}

# Run tests listed in file
function run_list () {
  test_file=$1
  if [ -f "$test_file" ]; then
    echo "=== Running tests from file: $test_file ==="
  else
    echo "=== Test file not specified ==="
    return 1
  fi

  ## Assume that in this file are only patterns (i.e. ewk_view_application_name) instead of
  ## full test names (i.e. utc_blink_ewk_view_application_name_for_user_agent_set.POS_TEST).
  ## So for each pattern we find all matching tests and run them one by one.
  while read test_pattern; do
    find_tests=$(generate_tests_list "$test_pattern")
    while read single_test; do
      run_test "$single_test"
    done <<< "$find_tests"
  done < "$test_file"

  echo "=== Finished running tests from file: $test_file ==="
}

function run_all {
  echo "=== Running all test ==="

  while read test; do
    run_test "$test"
  done <<< "$(generate_tests_list)"

  echo "=== Finished running all test ==="
}

while getopts "aghHl:p:s:" opt; do
  case "$opt" in
  # all tests
  a)
    init
    mkdir -p $UTC_RESULT_DIR
    run_all
    deinit
    exit 0
    ;;
  # generate list of all tests
  g)
    init
    generate_tests_list
    exit 0
    ;;
  # help
  h|H)
    usage
    exit 1
    ;;
  # run from list
  l)
    init
    mkdir -p $UTC_RESULT_DIR
    run_list $OPTARG
    deinit
    exit 0
    ;;
  # parse dir
  p)
    init
    xml_dir="$OPTARG"
    if [ ! -e "$xml_dir" ]; then
      echo "=== Directory '$xml_dir' doesn't exist ==="
      exit 4
    fi

    result=$(parse_dir $xml_dir)
    file_name="$(basename "$xml_dir").txt"
    file="$UTC_RESULTS_ROOT_DIR/$file_name"

    echo "$result" > $file
    success=$(echo "$result" | grep "^SUCCESS" | wc -l)
    failure=$(echo "$result" | grep "^FAILURE" | wc -l)
    sigsegv=$(echo "$result" | grep "SIGSEGV"  | wc -l)
    echo "" | tee -a $file
    echo "SUCCESS  : $success" | tee -a $file
    echo "FAILURE  : $failure ($sigsegv)" | tee -a $file
    echo "TOTAL    : $(expr $success + $failure)" | tee -a $file
    echo "" | tee -a $file
    echo "=== Results saved to $UTC_RESULTS_ROOT_DIR/$file_name ==="
    exit 0
    ;;
  # single test
  s)
    init
    mkdir -p $UTC_RESULT_DIR
    run_test $OPTARG
    deinit
    exit 0
    ;;
  esac
done

usage
exit 1
