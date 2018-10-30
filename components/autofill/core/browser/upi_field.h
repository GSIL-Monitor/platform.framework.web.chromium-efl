// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_UPI_FIELD_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_UPI_FIELD_H_

#include <memory>

#include "base/compiler_specific.h"
#include "components/autofill/core/browser/form_field.h"

namespace autofill {

// A form field that can parse for upi fields
class UpiField : public FormField {
 public:
  static std::unique_ptr<FormField> Parse(AutofillScanner* scanner);

 protected:
  void AddClassifications(FieldCandidatesMap* field_candidates) const override;

 private:
  explicit UpiField(const AutofillField* field);
  const AutofillField* field_;

  DISALLOW_COPY_AND_ASSIGN(UpiField);
};
}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_NAME_FIELD_H_
