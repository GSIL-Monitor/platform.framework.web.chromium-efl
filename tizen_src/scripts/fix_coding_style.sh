#!/bin/bash

SCRIPTDIR=$(readlink -e $(dirname $0))
TOPDIR=$(readlink -f "${SCRIPTDIR}/../..")
verbose=0
fix_mode=1
commit=HEAD~
include_ewk=0
issue_not_fixed=0
files_skipped=
files_fix=
issue_detected=0
issue_detected_for_file=0

function usage() {
cat << EOF

  usage: `basename $0` [OPTIONS] [<commit>]

  <commit>:
  If provided, fix/check coding styles for git diff from <commit> to current working copy.
  Otherwise, do the same for git diff from HEAD~ to current working copy (Default).

  OPTIONS:
    -h,--help   Show this message
    -t          Just detect style issues, not fixing them. (Test mode)
                Intended use is by buildbot
    -v          Verbosely log messages
    --include-ewk
                Also run style checker for EWK API files.
  examples:
    `basename $0` -t HEAD => Detect style issues for local changes.
    `basename $0` => Fix any style issue for last commit and local changes (diff since HEAD~).
EOF
  exit
}

function get_diff_lines() {
  filename=$1
  output=
  for line in `git diff --unified=0 --ignore-blank-lines ${commit} -- $filename | grep ^@@ | cut -d@  -f3 | cut -d+ -f2`; do
    if [ `echo $line | grep -c ,0` == 1 ]; then
       # Skip removal changes.
       continue
    elif [ `echo $line | grep -c ,` == 0 ]; then
       # 1 line change. Normalize it for further processing.
       line=${line},1
    fi
    offset=`echo $line | cut -d, -f1`
    length=`echo $line | cut -d, -f2`
    end=$((offset+length-1))
    output+=" -lines=${offset}:${end}"
  done
  echo $output
}

function choose_style() {
  style='Chromium'
  echo $style
}

function fix_filemode_if_need() {
  f=$1
  need_fix=0
  if [[ -x "$f" && $(echo $f | grep -c '\.cpp$\|\.cc$\|\.h$\|\.c$\|\.gyp$\|\.gypi$\|\.gn$') == 1 ]]; then
     [ $fix_mode -eq 1 ] && chmod a-x ${TOPDIR}/$f
     echo 1
  else
     echo 0
  fi
}

function parse() {
  while [[ $# > 0 ]]; do
    case "$1" in
      -h|--help)
        usage
        ;;
      -t)
        fix_mode=0
        ;;
      -v)
        verbose=1
        ;;
      --include-ewk)
        include_ewk=1
        ;;
      *)
        commit=${1}
        ;;
    esac
    shift;
  done
}

function on_start_file() {
  f="$1"
  issue_detected_for_file=0
  if [ $verbose -eq 1 ]; then
    echo
    echo "...Checking $f"
  fi
}

function on_style_issue_detected() {
  f="$1"
  issue_detected=1
  issue_detected_for_file=1
  if [ $verbose -eq 1 ]; then
    [ $fix_mode -eq 1 ] && echo "   : Style issues found and fixed." \
                        || echo "   : Need to fix style issues"
  fi
}

function on_filemode_issue_detected() {
  f="$1"
  issue_detected=1
  issue_detected_for_file=1
  if [ $verbose -eq 1 ]; then
    [ $fix_mode -eq 1 ] && echo "   : Removed file attribute +x." \
                        || echo "   : Need to remove file attribute +x."
  fi
}

function on_finish_file() {
  f="$1"
  if [ $issue_detected_for_file -eq 1 ]; then
    files_fix="$files_fix $f"
    [ $verbose -eq 1 ] && echo "   => Issue found."
  else
    [ $verbose -eq 1 ] && echo "   => No issue found."
  fi
}

######## START #######
parse "$@"

if [ $verbose -eq 1 ]; then
  echo ----Setting------------
  echo . commit=$commit
  echo . fix_mode=$fix_mode
  echo . include_ewk=$include_ewk
  echo -----------------------
fi

pushd $TOPDIR > /dev/null

for f in $(git diff --name-only ${commit}); do
  on_start_file $f

  filemode_fixed=`fix_filemode_if_need $f`
  if [ "$filemode_fixed" -eq 1 ] ; then
    on_filemode_issue_detected $f
  fi

  # only check for c/c++ files
  if [[ $(echo $f | grep -c '\.cpp$\|\.cc$\|\.h$\|\.c$') == 0 ]]; then
    [ "$filemode_fixed" -ne 1 ] && files_skipped="$files_skipped $f"
    [ $verbose -eq 1 ] && echo "   : Skipped Style checking (not a C/C++ file)"
    on_finish_file $f
    continue
  fi

  # Exclude ewk api files which follows different coding style with historical reason.
  if [[ $include_ewk == 0 &&\
        $(echo $f |\
          grep -c -e 'tizen_src/ewk/efl_integration/public/'\
         ) == 1 ]]; then
    [ "$filemode_fixed" -ne 1 ] && files_skipped="$files_skipped $f"
    [ $verbose -eq 1 ] && echo "   : Skipped Style checking (public ewk header)."
    on_finish_file $f
    continue
  fi

  s=`choose_style $f`
  l=`get_diff_lines $f`

  # Detect style issues and fix them if necessary.
  if [ -n "$l" ]; then
    tmpfile=$(mktemp)
    [ $verbose -eq 1 ] && echo "   : Running \$ $TOPDIR/buildtools/linux64/clang-format -style=\"$s\" $l ${TOPDIR}/$f"
    $TOPDIR/buildtools/linux64/clang-format -style="$s" $l ${TOPDIR}/$f > $tmpfile
    diff -q $tmpfile $f > /dev/null 2>&1
    if [ $? -eq 1 ]; then
      on_style_issue_detected $f
      if [ $fix_mode -eq 1 ]; then
        $TOPDIR/buildtools/linux64/clang-format -style="$s" $l -i ${TOPDIR}/$f
      fi
    else
      [ $verbose -eq 1 ] && echo "     --> No style issue found."
    fi
    rm $tmpfile
  fi

  on_finish_file $f
done

echo
if [ "$issue_detected" -eq 1 ]; then
  echo -n "*** Style issues are detected "
  if [[ $fix_mode == 1 ]]; then
    echo "and fixed!"
  else
    echo "and not fixed because of -t option."
    echo "To fix them, run $0 without -t option."
  fi
else
  echo "*** No style issue found :)"
fi

if [ -n "$files_fix" ]; then
  echo
  [ $fix_mode -eq 1 ] && echo "Files Fixed:" || echo "Files Need Fix:"
  for a in $files_fix; do
    echo "  $a"
  done
fi

if [ -n "$files_skipped" ]; then
  echo
  echo "[WARNING] Following files are skipped. Please check them for any style issue manually."
  echo "Skipped Files:"
  for f in $files_skipped; do
    [ -x $f ] && echo -n "(!) $f" || echo "  $f"
    [ -x $f ] && echo " => Execute filemode is set. Please check!"
  done
fi

popd > /dev/null

if [ "$issue_detected" -eq 1 ]; then
  exit 1
fi
exit 0
