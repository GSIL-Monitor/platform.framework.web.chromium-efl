// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/upi_field.h"

#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/browser/autofill_field.h"
#include "components/autofill/core/common/autofill_regex_constants.h"
#include "components/autofill/core/browser/autofill_scanner.h"

namespace autofill {

std::unique_ptr<FormField> UpiField::Parse(AutofillScanner* scanner) {
  AutofillField* field;
  if (ParseFieldSpecifics(scanner, base::UTF8ToUTF16(kUpiRe),
                          MATCH_DEFAULT | MATCH_EMAIL, &field)) {
    return base::WrapUnique(new UpiField(field));
  }
  return NULL;
}

UpiField::UpiField(const AutofillField* field) : field_(field) {}

void UpiField::AddClassifications(FieldCandidatesMap* field_candidates) const {
  AddClassification(field_, UPI_FIELD, kBaseUpiParserScore, field_candidates);
}
}
