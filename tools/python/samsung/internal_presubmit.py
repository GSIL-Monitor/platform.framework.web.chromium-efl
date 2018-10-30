# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Presubmit helper functions."""

import collections
import copy
import os
import re
import subprocess
import sys
import xml.dom.minidom

class ScopedSysPathHijacker(object):
  def __init__(self, dir_path):
    self.dir = dir_path

  def __enter__(self):
    self._saved_sys_path = sys.path
    sys.path = sys.path + [self.dir]

  def __exit__(self, type, value, traceback):
    sys.path = self._saved_sys_path


def _PanProjectChecks(input_api, output_api, excluded_paths=None,
                     text_files=None, license_header=None,
                     project_name=None, maxlen=80):
  """Our version of PanProjectChecks from
  depot_tools/presubmit_canned_checks.py. We need to fork this function
  because we want to run a different set of default checks and we do not
  want to patch depot_tools."""

  excluded_paths = tuple(excluded_paths or [])
  text_files = tuple(text_files or (
     r'.+\.txt$',
     r'.+\.json$',
  ))
  black_list = input_api.DEFAULT_BLACK_LIST + excluded_paths
  white_list = input_api.DEFAULT_WHITE_LIST + text_files
  sources = lambda x: input_api.FilterSourceFile(x, black_list=black_list)
  text_files = lambda x: input_api.FilterSourceFile(
      x, black_list=black_list, white_list=white_list)

  results = []
  results.extend(input_api.canned_checks.CheckLongLines(
      input_api, output_api, maxlen, source_file_filter=sources))
  results.extend(input_api.canned_checks.CheckChangeHasNoTabs(
      input_api, output_api, source_file_filter=sources))
  results.extend(input_api.canned_checks.CheckChangeHasNoStrayWhitespace(
      input_api, output_api, source_file_filter=sources))
  results.extend(input_api.canned_checks.CheckDoNotSubmitInDescription(
      input_api, output_api))
  results.extend(input_api.canned_checks.CheckDoNotSubmitInFiles(
      input_api, output_api))
  results.extend(input_api.canned_checks.CheckGNFormatted(
      input_api, output_api))
  return results


def _CheckDescription(input_api, output_api):
  """Checks internal commit message requirements."""
  description_text = input_api.change.FullDescriptionText()
  lines = description_text.splitlines()
  results = []

  ISSUE = '[Issue#]'
  PROBLEM = '[Problem]'
  SOLUTION = '[Solution]'
  TOGETHER_WITH = 'Together with'
  PID = 'PID'
  CHANGE_ID = 'Change-Id'
  SIGNED_OFF = 'Signed-off-by'

  # Listed in the preferred order of occurance.
  tags = [ ISSUE, PROBLEM, SOLUTION, TOGETHER_WITH, PID, CHANGE_ID, SIGNED_OFF ]

  def should_preceed(tag1, tag2):
    return tags.index(tag1) < tags.index(tag2)

  class State(object):
    def __init__(self, num_lines):
      self.num_lines = num_lines
      self.pos = 0
      self.last_tag = None

      self.tags = {}
      # Value is a tuple: (has already seen, line number of first occurance).
      for t in tags:
        self.tags[t] = (False, -1)

  s = State(len(lines))

  def seen_tag(tag):
    return s.tags[tag][0]

  def set_seen_tag(tag):
    s.last_tag = tag
    if s.tags[tag][0]:
      return
    s.tags[tag] = (True, s.pos)

  def line(offset=0):
    index = s.pos + offset
    if index < 0 or index >= s.num_lines:
      return ""
    return lines[index]

  def empty(offset=0):
    index = s.pos + offset
    if index < 0 or index >= s.num_lines:
      return False
    return len(lines[s.pos + offset]) == 0

  def step(num=1):
    s.pos += num

  def end_reached_after(num_lines):
    return s.pos + num_lines >= s.num_lines

  def end_reached():
    return end_reached_after(0)

  def format_message(msg):
    if end_reached():
      return "ChangeLog: %s" % msg
    return "ChangeLog(%d): %s" % (s.pos + 1, msg)

  def notify(msg):
    results.extend([output_api.PresubmitNotifyResult(format_message(msg))])

  def error(msg):
    results.extend([output_api.PresubmitError(format_message(msg))])

  def warning(msg):
    results.extend([output_api.PresubmitPromptWarning(format_message(msg))])

  LOOSE_FIXUP_RE = re.compile(r'(?i)^\W*\bfix[\s-]*up!?.*$')
  FIXUP_RE = re.compile(r'^fixup! (.*)$')

  def proc_title():
    if s.num_lines < 2 or empty() or not empty(1):
      error('Commit message should start with title followed by an ' \
            'empty line and detailed description.')

    if re.match(LOOSE_FIXUP_RE, line()):
      m = FIXUP_RE.match(line())
      if not m:
        warning('Fix-up commits should start with "fixup! " followed by ' \
                'the original commit title')
      else:
        original_title = m.group(1)
        args = ['git', 'rev-parse', '--verify', '--quiet',
                'HEAD^^{/^' + re.escape(original_title) + '}']
        try:
          input_api.subprocess.check_output(args)
        except:
          error('No matching commit found for fix-up directive.')

    step()

  LOOSE_TAG_RE = re.compile(
      r'(?i)^\W*' \
      r'(\[\Issue\#\]|\[Problem\]|\[Solution\]|PID|Together([\s-]+)?with|' \
      r'Change-Id|Signed-off-by)' \
      r'\W*:?.*$')
  TAG_RE = re.compile(
      r'(?i)^(?P<trail0>\W+^[\[])?(?P<key>\[?[\w\- ]+\w\#?\]?)' \
      r'(?P<trail1>[^[\#\]]\W]]+)?' \
      r':?(?P<separator>\W+)?(?P<value>.+)?$')

  def proc_tag_common(tag):
    if seen_tag(tag) and not s.last_tag == tag:
      warning('Multiple occurance of "%s" found.\n' \
              'Please group them together at subsequent lines ' \
              '(no empty lines between).' % tag)

    # 1. "Signed-off-by" should follow "Change-Id" immediately.
    # 2. "[Solution]" should follow "[Issue#]" immediately.
    if s.last_tag != tag and not empty(-1) \
      and not tag == ISSUE \
      and not tag == SOLUTION \
      and not tag == SIGNED_OFF \
      and not (s.last_tag == ISSUE and tag == PROBLEM):
      warning('No empty line before "%s".' % tag)

    # Look for a tag that has seen before but should be preceeded
    # by the current one.
    for other_tag, (seen, _) in s.tags.iteritems():
      if tag == other_tag or not seen:
        continue
      if should_preceed(other_tag, tag):
        continue
      warning(('Wrong order of tags: %s should preceed %s.\n' %
          (tag, other_tag)) + ('Order of tags should be: %s.' % str(tags)))

    # LOOSE_TAG_RE should have been checked already so we should have a match
    # and key should not be empty.
    m = TAG_RE.match(line())
    assert m
    assert m.group('key')

    should_be = 'Should be "%s: <value>" with a single space between ' \
                'the semicolon and the value.' % tag
    trailing = \
        ('Trailing whitespaces in line containing tag "%s".\n' % tag) + \
        should_be
    no_space = ('No space after "%s:".\n' % tag) + should_be

    if m.group('trail0') or m.group('trail1') \
      or (m.group('separator') and m.group('separator') != ' '):
      warning(trailing)
    if not m.group('separator'):
      warning(no_space)
    if m.group('key') != tag:
      warning('Mistyped tag? Found "%s" instead of "%s".' %
          (m.group('key'), tag))
    if not m.group('value'):
      warning('Missing value for "%s".' % tag)

    if m.group('key') == tag:
      set_seen_tag(tag)

    return m.group('value')

  def proc_tag_exist(tag):
    if seen_tag(tag) and not s.last_tag == tag:
      warning('Multiple occurance of "%s" found.\n' \
              'Please group them together at subsequent lines ' \
              '(no empty lines between).' % tag)

    m = TAG_RE.match(line())
    assert m
    assert m.group('key')
    if m.group('key') == tag:
      set_seen_tag(tag)

    return m.group('value')

  ISSUE_VALUE_RE = re.compile(r'^None|P\d{6}[-]\d{5}(,|\s|$)')

  def proc_issue():
    value = proc_tag_common(ISSUE)
    if not value or not ISSUE_VALUE_RE.match(value):
      error('Invalid format for "[Issue#]", ' \
            'add PLM issue number <P000000-00000> or None.')

  def proc_problem():
    proc_tag_exist(PROBLEM)

  def proc_solution():
    proc_tag_exist(SOLUTION)

  def proc_together_with():
    proc_tag_common(TOGETHER_WITH)

  def proc_change_id():
    proc_tag_common(CHANGE_ID)

  def proc_signed_off():
    proc_tag_common(SIGNED_OFF)
    (seen, pos) = s.tags[CHANGE_ID]
    if not seen or pos != (s.pos - 1):
      warning('"Signed-off-by" is misplaced. It should follow "Change-Id"' \
              ' immediately in the next line.')

  PID_VALUE_RE = re.compile(r'\w+-\d+', re.I)

  def proc_pid():
    value = proc_tag_common(PID)
    if not value or not PID_VALUE_RE.search(value):
      error('Invalid format for "PID", PID format should be PID:INT-<pid_id>.')

  def consume_tags():
    while not end_reached():
      m = LOOSE_TAG_RE.match(line())
      if not m:
        step()
        continue
      tag_lower = m.group(1).lower()
      if tag_lower == 'pid':
        proc_pid()
      elif tag_lower.startswith('[issue#]'):
        proc_issue()
      elif tag_lower.startswith('[problem]'):
        proc_problem()
      elif tag_lower.startswith('[solution]'):
        proc_solution()
      elif tag_lower.startswith('together'):
        proc_together_with()
      elif tag_lower == 'change-id':
        proc_change_id()
      elif tag_lower == 'signed-off-by':
        proc_signed_off()
      else:
        assert False
      step()

  def finish():
    if not seen_tag(ISSUE):
      error('No [Issue#] field. Add [Issue#] and plm number.')
    if not seen_tag(PROBLEM):
      error('No [Problem] field. Add [Problem] and describe problems.')
    if not seen_tag(SOLUTION):
      error('No [Solution] field. Add [Solution] and describe solutions.')
    if not seen_tag(PID):
      error('No PID number. \n "PID:INT-<pid_number>" is mandatory.')
    if not seen_tag(CHANGE_ID):
      error('Missing "Change-Id". Forgot to add commit-hook?')
    if not seen_tag(SIGNED_OFF):
      error('Missing "Signed-off-by". Use git commit -s.')

  # Off we go parsing the commit message.
  proc_title()
  consume_tags()
  finish()

  return results


def _CheckFilePermissions(input_api, output_api):
  results = []
  chromium_root = input_api.os_path.abspath(
      input_api.os_path.join(__file__, '..', '..', '..', '..'))
  import tempfile
  with tempfile.NamedTemporaryFile() as jsonfile:
    checkperms = input_api.os_path.join(
        chromium_root, 'tools/checkperms/checkperms.py')
    args = [input_api.python_executable, checkperms,
        '--root', input_api.change.RepositoryRoot()]
    for f in input_api.AffectedFiles():
      args += ['--file', f.LocalPath()]
    args += ['--json', jsonfile.name]
  try:
    input_api.subprocess.check_output(args)
    return []
  except input_api.subprocess.CalledProcessError as error:
    import json
    with open(jsonfile.name) as json_file:
      errors = json.load(json_file)
    errors = '\n'.join('%s: %s' % (e['rel_path'], e['error']) for e in errors)
    results.extend([output_api.PresubmitError('File permission error',
        errors.splitlines())])
  return results


def CommonChecks(input_api, output_api, excluded_paths=None,
                 license_header=None, maxlen=80):
  results = []
  results.extend(_PanProjectChecks(input_api, output_api,
      excluded_paths=excluded_paths, license_header=license_header,
      maxlen=maxlen))
  results.extend(_CheckDescription(input_api, output_api))
  results.extend(_CheckFilePermissions(input_api, output_api))
  return results


def CommonChromiumToplevelChecks(input_api, output_api):
  """Common checks from toplevel PRESUBMIT.py."""
  """Mostly C++ and Java style checks."""
  """Use it only for subrepositories that follow chromium guidelines exactly."""

  results = []

  path_api = input_api.os_path
  chromium_root = path_api.abspath(
      input_api.os_path.join(__file__, '..', '..', '..', '..'))

  class ChromiumPresubmitShim(object):
    """Bridge for toplevel PRESUBMIT.py."""
    def __init__(self):
      self._input_api = input_api
      self._output_api = output_api
      self._Init()

    def _Init(self):
      path_api = self._input_api.os_path
      script_path = path_api.join(chromium_root, 'PRESUBMIT.py')
      with open(script_path, mode='r') as script:
        script_text = script.read()

      # Execute script to get it's function definitions into self._context.
      self._context = {}
      exec script_text in self._context

      # Common api for chech functions.
      self._context['__args'] = (self._input_api, self._output_api)

    def PrepareSysPath(self):
      # This is normally done by src/PRESUBMIT.py but it fails because it
      # assumes that we are doing the presubmit in src/ so we help a little.
      self._saved_sys_path = sys.path
      extra_import_paths = [
        path_api.join(chromium_root, 'buildtools', 'checkdeps'),
        path_api.join(chromium_root, 'tools', 'json_comment_eater')
      ]
      sys.path = sys.path + extra_import_paths

    def RestoreSysPath(self):
      sys.path = self._saved_sys_path

    def DoCheck(self, function_name):
      """Executes function from toplevel PRESUBMIT.py on this change."""
      result = eval(function_name + '(*__args)', self._context)
      results.extend(result)


  shim = ChromiumPresubmitShim()

  def CheckFilePermissions():
    import tempfile
    with tempfile.NamedTemporaryFile() as jsonfile:
      checkperms = path_api.join(chromium_root, 'tools/checkperms/checkperms.py')
      args = [input_api.python_executable, checkperms,
          '--root', input_api.change.RepositoryRoot()]
      for f in input_api.AffectedFiles():
        args += ['--file', f.LocalPath()]
      args += ['--json', jsonfile.name]
      process = input_api.subprocess.Popen(
          args, stdout=input_api.subprocess.PIPE)
      error = process.communicate()[0].strip()
      if error:
        import json
        errors = json.load(jsonfile)
        errors = '\n'.join('%s: %s' % (e['rel_path'], e['error']) for e in errors)
        results.extend([output_api.PresubmitError('File permission error',
            errors.splitlines())])


  def CheckJavaStyle():
    import fnmatch
    checkstyle_jar_dir = path_api.join(chromium_root,
                                       'third_party', 'checkstyle')
    checkstyle_jar_name = fnmatch.filter(os.listdir(checkstyle_jar_dir),
                                         'checkstyle-*.jar')[0]
    checkstyle_jar = path_api.join(checkstyle_jar_dir, checkstyle_jar_name)

    checkstyle_xml_dir = path_api.join(chromium_root,
                                       'tools', 'python', 'samsung')
    checkstyle_xml_name = fnmatch.filter(os.listdir(checkstyle_xml_dir),
                                         'chromium-style-*.xml')[0]
    style_file = path_api.join(checkstyle_xml_dir, checkstyle_xml_name)

    java_files = [x.LocalPath() for x in input_api.AffectedFiles(False)
                  if os.path.splitext(x.LocalPath())[1] == '.java']
    if not java_files:
      return []

    # Run checkstyle
    checkstyle_env = os.environ.copy()
    checkstyle_env['JAVA_CMD'] = 'java'
    try:
      args = ['java', '-cp', checkstyle_jar,
              'com.puppycrawl.tools.checkstyle.Main', '-c', style_file,
              '-f', 'xml'] + java_files
      check = subprocess.Popen(args, stdout=subprocess.PIPE, env=checkstyle_env)
      stdout, _ = check.communicate()
    except OSError as e:
      import errno
      if e.errno == errno.ENOENT:
        install_error = ('  checkstyle is not installed. Please run '
                         'build/install-build-deps-android.sh')
        return [output_api.PresubmitPromptWarning(install_error)]

    # Process output.
    # Opposite to upstream we only report errors in regard to the actual
    # changed lines instead of all the errors in the changed files.
    # As of now we have so many style errors in our java codebase that doing
    # otherwise would not be very productive for the time being.
    output_dom_root = xml.dom.minidom.parseString(stdout)
    result_errors = []
    result_warnings = []
    changed_sources = dict([
        (affected.LocalPath(), affected)
        for affected in input_api.AffectedSourceFiles(
          lambda x: os.path.splitext(x.LocalPath())[1] == '.java')])

    for file_element in output_dom_root.getElementsByTagName('file'):
      file_name = file_element.attributes['name'].value
      file_name = os.path.relpath(file_name, input_api.PresubmitLocalPath())

      affected_file = changed_sources[file_name]
      changed_lines = [line for (line, _) in affected_file.ChangedContents()]
      errors = file_element.getElementsByTagName('error')
      lines_to_errors = dict([
        (int(error.attributes['line'].value), error)
        for error in errors])

      for line in changed_lines:
        error = lines_to_errors.get(line)
        if error is None:
          continue
        column = ''
        if error.hasAttribute('column'):
          column = '%s:' % error.attributes['column'].value
        message = error.attributes['message'].value
        result = '  %s:%s:%s %s' % (file_name, line, column, message)
        severity = error.attributes['severity'].value
        if severity == 'error':
          result_errors.append(result)
        elif severity == 'warning':
          result_warnings.append(result)

    if result_warnings:
      results.append(output_api.PresubmitPromptWarning(
          '\n'.join(result_warnings)))
    if result_errors:
      results.append(output_api.PresubmitError('\n'.join(result_errors)))


  shim.PrepareSysPath()
  shim.DoCheck('_CheckNoUNIT_TESTInSourceFiles')
  shim.DoCheck('_CheckNoNewWStrings')
  shim.DoCheck('_CheckNoDEPSGIT')
  shim.DoCheck('_CheckNoBannedFunctions')
  shim.DoCheck('_CheckNoPragmaOnce')
  shim.DoCheck('_CheckNoTrinaryTrueFalse')
  shim.DoCheck('_CheckIncludeOrder')
  shim.DoCheck('_CheckForVersionControlConflicts')
  shim.DoCheck('_CheckPatchFiles')
  shim.DoCheck('_CheckForInvalidOSMacros')
  shim.DoCheck('_CheckForInvalidIfDefinedMacros')
  shim.DoCheck('_CheckSpamLogging')
  shim.DoCheck('_CheckForAnonymousVariables')
  shim.DoCheck('_CheckParseErrors')
  shim.DoCheck('_CheckForIPCRules')
  shim.DoCheck('_CheckForWindowsLineEndings')
  shim.DoCheck('_CheckSingletonInHeaders')
  shim.DoCheck('_CheckUnwantedDependencies')
  shim.RestoreSysPath()

  CheckJavaStyle()

  return results

def CheckAllowedEncodingType(input_api, output_api):
  class CheckFileEncodingType:
    import codecs
    bom_list = (codecs.BOM, codecs.BOM_UTF8, codecs.BOM_BE, codecs.BOM_LE,
      codecs.BOM_UTF16, codecs.BOM_UTF16_BE, codecs.BOM_UTF16_LE);
    text_pattern = re.compile(r'(?<=\:).*text')

    @staticmethod
    # Byte Of Mark and Little Endian is not allowed
    def isAllowed(affectedFile):
      # Check if file is removed or not
      if not affectedFile.NewContents():
        return True

      # Check file type using linux file command
      # if printed file type has "text", it shall be considered as text file
      block = os.popen("file " + affectedFile.LocalPath()).read()
      if not CheckFileEncodingType.text_pattern.findall(block):
        return True

      bytes = min(32, os.path.getsize(affectedFile.LocalPath()))
      raw = open(affectedFile.LocalPath(), 'rb').read(bytes)
      if any(raw.startswith(bom) for bom in CheckFileEncodingType.bom_list):
        return False
      return True

  results = []
  for affectedFile in input_api.AffectedFiles():
    if not CheckFileEncodingType.isAllowed(affectedFile):
      results.extend([output_api.PresubmitError(
          affectedFile.LocalPath()  + ': File encoding type error. '
          'Little Endian(Windows) and Byte Of Mark can not be used.')])

  return results

def CheckHardcodedPackageName(input_api, output_api, project_name):
  class CommentPatterns:
    comment = re.compile(
        r'(.*\*\/)|((^|[\t\s\;])+\/[\/\*]+.*$)|(\<\!\-\-.*$)|(.*\-\-\>)')
    inline_comment = re.compile(r'(\/\*.*\*\/)|(\<\!\-\-.*\-\-\>)')

    @staticmethod
    def remove(line):
        return CommentPatterns.comment.sub('',
            CommentPatterns.inline_comment.sub('', line))

  CheckingPattern = collections.namedtuple('CheckingRule',
      ['file_suffix', 'pattern', 'errorType', 'description'])

  DESCRIPTION = 'com.sec.android.app.sbrowser string cannot be '\
      'used in this, since package name is changed when beta/edge version '\
      'is built. '

  CHECKING_PATTERNS = {
      'sbrowser': (
          CheckingPattern(
              '.java',
              re.compile(r'(?<!Class\.forName\()\"[\w\s\'\:\.\/]*'
                  'com.sec.android.app.sbrowser'),
              'Error',
              DESCRIPTION + 'Use Constants.EXTRA_APPLICATION_ID instead of '
              'this. This Constants.EXTRA_APPLICATION_ID is changed by '
              'build type.'),
          CheckingPattern(
              'AndroidManifest.xml',
              re.compile(r'(?<!package\=\")com.sec.android.app.sbrowser'),
              'Error',
              DESCRIPTION + 'Remove base package name if full package name is '
              'not required. or Use ${applicationId} instead of this, '
              'if package name is required. ')
          ),
      'terrace': (
          CheckingPattern(
              '.java',
              re.compile(r'\"[\w\s\'\:\.\/]*com.sec.android.app.sbrowser'),
              'Error',
              DESCRIPTION),
          CheckingPattern(
              'AndroidManifest.xml',
              re.compile(r'com.sec.android.app.sbrowser'),
              'Warning',
              DESCRIPTION),
          CheckingPattern(
              '.gradle',
              re.compile(r'com.sec.android.app.sbrowser'),
              'Warning',
              DESCRIPTION)
          )
  }

  results = []
  for affectedFile in input_api.AffectedFiles():
    for checkingPattern in CHECKING_PATTERNS[project_name]:
      if not affectedFile.LocalPath().endswith(checkingPattern.file_suffix):
        continue
      for line_num, line in affectedFile.ChangedContents():
        # Remove comments
        line = CommentPatterns.remove(line)
        if not checkingPattern.pattern.search(line):
          continue
        if (checkingPattern.errorType == 'Error'):
          results.extend([output_api.PresubmitError(
              '  ' + affectedFile.LocalPath() + ':' + str(line_num) + ':'
              + checkingPattern.description)])
        else:
          results.extend([output_api.PresubmitPromptWarning(
              '  ' + affectedFile.LocalPath() + ':' + str(line_num) + ':'
              + checkingPattern.description)])
  return results

def CheckImportBreachLayerClass(input_api, output_api):
  results = []
  result_errors = []
  pattern_import = re.compile('import\s+org.chromium')

  for affectedFile in input_api.AffectedFiles():
    if not affectedFile.LocalPath().endswith("java"):
      continue
    for line_num, line in affectedFile.ChangedContents():
      if pattern_import.match(line):
        result_errors.append(
            "  cowboy can not use chromium java files directly.\n"
            "  a breach of import policy at %s:%d\n" %
            (affectedFile.LocalPath(), line_num))

  if result_errors:
    results.append(output_api.PresubmitError('\n'.join(result_errors)))
  return results
