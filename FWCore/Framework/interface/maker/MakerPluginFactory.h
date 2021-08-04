#ifndef FWCore_Framework_MakerPluginFactory_h
#define FWCore_Framework_MakerPluginFactory_h

#include "FWCore/PluginManager/interface/PluginFactory.h"

namespace edm {
  struct Maker;

  using MakerPluginFactory = edmplugin::PluginFactory<Maker*()>;
}  // namespace edm

#endif
