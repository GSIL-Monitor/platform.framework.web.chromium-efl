# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import itertools
import json

from pylib.base import base_test_result

def GenerateResultsDict(test_run_results, global_tags=None):
  """Create a results dict from |test_run_results| suitable for writing to JSON.
  Args:
    test_run_results: a list of base_test_result.TestRunResults objects.
  Returns:
    A results dict that mirrors the one generated by
      base/test/launcher/test_results_tracker.cc:SaveSummaryAsJSON.
  """
  # Example json output.
  # {
  #   "global_tags": [],
  #   "all_tests": [
  #     "test1",
  #     "test2",
  #    ],
  #   "disabled_tests": [],
  #   "per_iteration_data": [
  #     {
  #       "test1": [
  #         {
  #           "status": "SUCCESS",
  #           "elapsed_time_ms": 1,
  #           "output_snippet": "",
  #           "output_snippet_base64": "",
  #           "losless_snippet": "",
  #         },
  #         ...
  #       ],
  #       "test2": [
  #         {
  #           "status": "FAILURE",
  #           "elapsed_time_ms": 12,
  #           "output_snippet": "",
  #           "output_snippet_base64": "",
  #           "losless_snippet": "",
  #         },
  #         ...
  #       ],
  #     },
  #     {
  #       "test1": [
  #         {
  #           "status": "SUCCESS",
  #           "elapsed_time_ms": 1,
  #           "output_snippet": "",
  #           "output_snippet_base64": "",
  #           "losless_snippet": "",
  #         },
  #       ],
  #       "test2": [
  #         {
  #           "status": "FAILURE",
  #           "elapsed_time_ms": 12,
  #           "output_snippet": "",
  #           "output_snippet_base64": "",
  #           "losless_snippet": "",
  #         },
  #       ],
  #     },
  #     ...
  #   ],
  # }

  def status_as_string(s):
    if s == base_test_result.ResultType.PASS:
      return 'SUCCESS'
    elif s == base_test_result.ResultType.SKIP:
      return 'SKIPPED'
    elif s == base_test_result.ResultType.FAIL:
      return 'FAILURE'
    elif s == base_test_result.ResultType.CRASH:
      return 'CRASH'
    elif s == base_test_result.ResultType.TIMEOUT:
      return 'TIMEOUT'
    elif s == base_test_result.ResultType.UNKNOWN:
      return 'UNKNOWN'

  all_tests = set()
  per_iteration_data = []
  test_run_links = {}

  for test_run_result in test_run_results:
    iteration_data = collections.defaultdict(list)
    if isinstance(test_run_result, list):
      results_iterable = itertools.chain(*(t.GetAll() for t in test_run_result))
      for tr in test_run_result:
        test_run_links.update(tr.GetLinks())

    else:
      results_iterable = test_run_result.GetAll()
      test_run_links.update(test_run_result.GetLinks())

    for r in results_iterable:
      result_dict = {
          'status': status_as_string(r.GetType()),
          'elapsed_time_ms': r.GetDuration(),
          'output_snippet': r.GetLog(),
          'losless_snippet': '',
          'output_snippet_base64': '',
          'annotations': r.GetAnnotations(),
          'links': r.GetLinks(),
      }
      iteration_data[r.GetName()].append(result_dict)

    all_tests = all_tests.union(set(iteration_data.iterkeys()))
    per_iteration_data.append(iteration_data)

  return {
    'global_tags': global_tags or [],
    'all_tests': sorted(list(all_tests)),
    # TODO(jbudorick): Add support for disabled tests within base_test_result.
    'disabled_tests': [],
    'per_iteration_data': per_iteration_data,
    'links': test_run_links,
  }


def GenerateJsonResultsFile(test_run_result, file_path, global_tags=None):
  """Write |test_run_result| to JSON.

  This emulates the format of the JSON emitted by
  base/test/launcher/test_results_tracker.cc:SaveSummaryAsJSON.

  Args:
    test_run_result: a base_test_result.TestRunResults object.
    file_path: The path to the JSON file to write.
  """
  with open(file_path, 'w') as json_result_file:
    json_result_file.write(json.dumps(
        GenerateResultsDict(test_run_result, global_tags=global_tags)))


def ParseResultsFromJson(json_results):
  """Creates a list of BaseTestResult objects from JSON.

  Args:
    json_results: A JSON dict in the format created by
                  GenerateJsonResultsFile.
  """

  def string_as_status(s):
    if s == 'SUCCESS':
      return base_test_result.ResultType.PASS
    elif s == 'SKIPPED':
      return base_test_result.ResultType.SKIP
    elif s == 'FAILURE':
      return base_test_result.ResultType.FAIL
    elif s == 'CRASH':
      return base_test_result.ResultType.CRASH
    elif s == 'TIMEOUT':
      return base_test_result.ResultType.TIMEOUT
    else:
      return base_test_result.ResultType.UNKNOWN

  results_list = []
  testsuite_runs = json_results['per_iteration_data']
  for testsuite_run in testsuite_runs:
    for test, test_runs in testsuite_run.iteritems():
      results_list.extend(
          [base_test_result.BaseTestResult(test,
                                           string_as_status(tr['status']),
                                           duration=tr['elapsed_time_ms'])
          for tr in test_runs])
  return results_list

