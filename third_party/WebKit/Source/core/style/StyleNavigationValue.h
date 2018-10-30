/*
 * Copyright (C) 2011 Kyounga Ra (kyounga.ra@gmail.com)
 * Copyright (C) 2014 Opera Software ASA. All rights reserved.
 * Copyright (C) 2018 Samsung Electronics. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef StyleNavigationValue_h
#define StyleNavigationValue_h

#include "core/style/ComputedStyleConstants.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

class StyleNavigationValue {
  DISALLOW_NEW();

 public:
  StyleNavigationValue() : is_auto_(true), navigation_target_(Current) {}

  explicit StyleNavigationValue(const String& id,
                                ENavigationTarget target = Current)
      : is_auto_(false), navigation_target_(target), id_(AtomicString(id)) {}

  StyleNavigationValue(const String& id, const String& target)
      : is_auto_(false),
        navigation_target_(TargetName),
        id_(AtomicString(id)),
        target_name_(AtomicString(target)) {}

  bool operator==(const StyleNavigationValue& o) const {
    if (is_auto_)
      return o.is_auto_;
    if (id_ != o.id_)
      return false;
    if (navigation_target_ == TargetName && target_name_ != o.target_name_)
      return false;
    return navigation_target_ == o.navigation_target_;
  }

  bool operator!=(const StyleNavigationValue& o) const { return !(*this == o); }

  bool IsAuto() const { return is_auto_; }
  const AtomicString& Id() const { return id_; }
  ENavigationTarget NavigationTarget() {
    return static_cast<ENavigationTarget>(navigation_target_);
  }
  const AtomicString& GetTargetName() const { return target_name_; }

 private:
  unsigned is_auto_ : 1;
  unsigned navigation_target_ : 2;  // ENavigationTarget
  AtomicString id_;
  AtomicString target_name_;
};

}  // namespace blink

#endif  // StyleNavigationValue_h
