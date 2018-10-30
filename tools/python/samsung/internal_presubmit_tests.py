#!/usr/bin/env python
#
# Copyright 2015 Samsung Electronics. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import internal_presubmit as presubmit
import sys


exit_status = 0

def CommitMessageTests():
  # Mock of both InputApi and OutputApi.
  class Env(object):
    def __init__(self, description):
      self.description = description
      self.issues = []
      self.change = self

    def FullDescriptionText(self):
      return self.description

    def PresubmitError(self, msg):
      self.issues.append(msg)
    def PresubmitPromptWarning(self, msg):
      self.issues.append(msg)
    def PresubmitWarning(self, msg):
      self.issues.append(msg)
    def PresubmitNotifyResult(self, msg):
      self.issues.append(msg)

  def test(commit_message, expected_issues):
    env = Env(commit_message)
    presubmit._CheckDescription(env, env)
    if env.issues != expected_issues:
      global exit_status
      exit_status += 1

      print "TEST FAILED"
      print "-- Commit message --"
      for (i, l) in enumerate(commit_message.splitlines()):
        print "%d:%s" % (i + 1, l)
      print "-- Expected issues (%d) --" % len(expected_issues)
      for i in expected_issues:
        print i
      print "-- Actual issues (%d) --" % len(env.issues)
      for i in env.issues:
        print i
      print ""

  def format_expected(*args):
    if len(args) == 2:
      return 'ChangeLog(%d): %s' % (args[0],  args[1])
    return 'ChangeLog: %s' % args[0]

  def make_expected(lineno, issue):
    return [format_expected(lineno, issue)]

  template = '%s%s'

  title_good = 'Some commit\n'

  tags_good = """
[Issue#] P160720-11111
[Problem] Main branch policy
[Solution] Change it!
And more...

PID: INT-00

Change-Id: I32143214febffdd124123
Signed-off-by: Test Elek <testelek@goodguy.org>
"""

  simple_good = template % (title_good, tags_good)
  test(simple_good, [])

  multiple_issue_comma = \
      simple_good.replace('P160720-11111', 'P160720-11111, P160720-22222')
  test(multiple_issue_comma, [])

  multiple_issue_space = \
      simple_good.replace('P160720-11111', 'P160720-11111 P160720-22222')
  test(multiple_issue_space, [])

  no_title = template % ('\n', tags_good)
  test(no_title, make_expected(1, \
      'Commit message should start with title followed by an ' \
      'empty line and detailed description.'))

  no_blank_after_title = template % ('Title for No blank', tags_good)
  test(no_blank_after_title, make_expected(1, \
      'Commit message should start with title followed by an ' \
      'empty line and detailed description.'))

  bad_fixup_keyword = template % ('fix-up foooo\n\n%s', tags_good)
  test(bad_fixup_keyword, make_expected(1, \
      'Fix-up commits should start with "fixup! " followed by ' \
      'the original commit title'))

  bad_fixup_reference = \
      template % ("fixup! no-such-commit\n\n%s", tags_good)
  test(bad_fixup_reference, make_expected(1, \
      'No matching commit found for fix-up directive.'))

  bad_issue_not_available = simple_good.replace('P160720-11111', 'n/a')
  test(bad_issue_not_available, make_expected(3, \
      'Invalid format for "[Issue#]", ' \
      'add PLM issue number <P000000-00000> or None.'))

  bad_issue = simple_good.replace('P160720-11111', 'P160720-4444')
  test(bad_issue, make_expected(3, \
      'Invalid format for "[Issue#]", ' \
      'add PLM issue number <P000000-00000> or None.'))

  bad_issue_other = simple_good.replace('P160720-11111', 'issue P160720-11111')
  test(bad_issue_other, make_expected(3, \
      'Invalid format for "[Issue#]", ' \
      'add PLM issue number <P000000-00000> or None.'))

  more_tags = """
[Issue#] P160720-22222
[Problem] Main branch policy
[Solution] Together with patches

Together with: I1234

PID: INT-00

Change-Id: I32143214febffdd124123
Signed-off-by: Test Elek <testelek@goodguy.org>
"""
  complex_good = template % (title_good, more_tags)
  test(complex_good, [])

  more_tags_bad_order = """
[Problem] Main branch policy
[Issue#] P160720-22222
[Solution] Together with patches

PID: INT-00

Together with: I1234

Change-Id: I32143214febffdd124123
Signed-off-by: Test Elek <testelek@goodguy.org>
"""
  complex_bad_order = template % (title_good, more_tags_bad_order)
  test(complex_bad_order, [ \
      format_expected(4, \
          "Wrong order of tags: [Issue#] should preceed [Problem].\n" \
          "Order of tags should be: ['[Issue#]', '[Problem]', '[Solution]', " \
          "'Together with', 'PID', 'Change-Id', 'Signed-off-by']."),
      format_expected(9, \
          "Wrong order of tags: Together with should preceed PID.\n" \
          "Order of tags should be: ['[Issue#]', '[Problem]', '[Solution]', " \
          "'Together with', 'PID', 'Change-Id', 'Signed-off-by'].")
      ])

  more_tags_bad = """
Together-with: I1234
PID:INT-00
Change-id: I32143214febffdd124123

Signed-off-by:
"""

  complex_bad = template % (title_good, more_tags_bad)
  test(complex_bad, [ \
      format_expected(3, \
          'Mistyped tag? Found "Together-with" instead of "Together with".'), \
      format_expected(4, \
          'No empty line before "PID".'), \
      format_expected(4, \
          'No space after "PID:".\n'
          'Should be "PID: <value>" with a single space ' \
          'between the semicolon and the value.'), \
      format_expected(5, \
          'No empty line before "Change-Id".'), \
      format_expected(5, \
          'Mistyped tag? Found "Change-id" instead of "Change-Id".'), \
      format_expected(7, \
          'No space after "Signed-off-by:".\n'
          'Should be "Signed-off-by: <value>" with a single space between'
          ' the semicolon and the value.'), \
      format_expected(7, \
          'Missing value for "Signed-off-by".'), \
      format_expected(7, \
          '"Signed-off-by" is misplaced. It should follow "Change-Id"' \
          ' immediately in the next line.'), \
      format_expected('No [Issue#] field. '
          'Add [Issue#] and plm number.'), \
      format_expected('No [Problem] field. '\
          'Add [Problem] and describe problems.'), \
      format_expected('No [Solution] field. '\
          'Add [Solution] and describe solutions.'), \
      format_expected('Missing "Change-Id". Forgot to add commit-hook?')
      ])


if __name__ == '__main__':
  CommitMessageTests()
  sys.exit(exit_status)
