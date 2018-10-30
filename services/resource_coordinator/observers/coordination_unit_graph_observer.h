// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_GRAPH_OBSERVER_H_
#define SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_GRAPH_OBSERVER_H_

#include "base/macros.h"
#include "services/resource_coordinator/public/interfaces/coordination_unit.mojom.h"

namespace resource_coordinator {

class CoordinationUnitBase;
class CoordinationUnitManager;
class FrameCoordinationUnitImpl;
class PageCoordinationUnitImpl;

// An observer API for the coordination unit graph maintained by GRC.
//
// Observers are instantiated when the resource_coordinator service
// is created and are destroyed when the resource_coordinator service
// is destroyed. Therefore observers are guaranteed to be alive before
// any coordination unit is created and will be alive after any
// coordination unit is destroyed. Additionally, any
// Coordination Unit reachable within a callback will always be
// initialized and valid.
//
// To create and install a new observer:
//   (1) Derive from this class.
//   (2) Register by calling on |coordination_unit_manager().ResgiterObserver|
//       inside of the ResourceCoordinatorService::Create.
class CoordinationUnitGraphObserver {
 public:
  CoordinationUnitGraphObserver();
  virtual ~CoordinationUnitGraphObserver();

  // Determines whether or not the observer should be registered with, and
  // invoked for, the |coordination_unit|.
  virtual bool ShouldObserve(const CoordinationUnitBase* coordination_unit) = 0;

  // Called whenever a CoordinationUnit is created.
  virtual void OnCoordinationUnitCreated(
      const CoordinationUnitBase* coordination_unit) {}

  // Called when the |coordination_unit| is about to be destroyed.
  virtual void OnBeforeCoordinationUnitDestroyed(
      const CoordinationUnitBase* coordination_unit) {}

  // Called whenever a new parent-child relationship occurs where the
  // |coordination_unit| is the parent of |child_coordination_unit|
  virtual void OnChildAdded(
      const CoordinationUnitBase* coordination_unit,
      const CoordinationUnitBase* child_coordination_unit) {}

  // Called whenever parent-child relationship ends where the
  // |coordination_unit| was the parent and the |child_coordination_unit|.
  virtual void OnChildRemoved(
      const CoordinationUnitBase* coordination_unit,
      const CoordinationUnitBase* child_coordination_unit) {}

  // Called whenever a new parent-child relationship occurs where the
  // |coordination_unit| is the child of |parent_coordination_unit|.
  virtual void OnParentAdded(
      const CoordinationUnitBase* coordination_unit,
      const CoordinationUnitBase* parent_coordination_unit) {}

  // Called whenever parent-child relationship ends where the
  // |coordination_unit| was the child and the |child_coordination_unit|.
  virtual void OnParentRemoved(
      const CoordinationUnitBase* coordination_unit,
      const CoordinationUnitBase* parent_coordination_unit) {}

  // Called whenever a property of the |coordination_unit| is changed if the
  // |coordination_unit| doesn't implement its own PropertyChanged handler.
  virtual void OnPropertyChanged(const CoordinationUnitBase* coordination_unit,
                                 const mojom::PropertyType property_type,
                                 int64_t value) {}

  // Called whenever a property of the FrameCoordinationUnit is changed.
  virtual void OnFramePropertyChanged(const FrameCoordinationUnitImpl* frame_cu,
                                      const mojom::PropertyType property_type,
                                      int64_t value) {}

  // Called whenever a property of the PageCoordinationUnit is changed.
  virtual void OnPagePropertyChanged(const PageCoordinationUnitImpl* page_cu,
                                     const mojom::PropertyType property_type,
                                     int64_t value) {}

  // Called whenever an event is received in |coordination_unit| if the
  // |coordination_unit| doesn't implement its own EventReceived handler.
  virtual void OnEventReceived(const CoordinationUnitBase* coordination_unit,
                               const mojom::Event event) {}
  virtual void OnFrameEventReceived(const FrameCoordinationUnitImpl* frame_cu,
                                    const mojom::Event event) {}
  virtual void OnPageEventReceived(const PageCoordinationUnitImpl* page_cu,
                                   const mojom::Event event) {}

  void set_coordination_unit_manager(
      CoordinationUnitManager* coordination_unit_manager) {
    coordination_unit_manager_ = coordination_unit_manager;
  }

  const CoordinationUnitManager& coordination_unit_manager() const {
    return *coordination_unit_manager_;
  }

 private:
  CoordinationUnitManager* coordination_unit_manager_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(CoordinationUnitGraphObserver);
};

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_GRAPH_OBSERVER_H_
