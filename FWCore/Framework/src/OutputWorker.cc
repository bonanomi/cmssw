
/*----------------------------------------------------------------------
$Id: OutputWorker.cc,v 1.36 2008/01/13 01:12:33 wmtan Exp $
----------------------------------------------------------------------*/

#include "FWCore/Framework/interface/OutputModule.h"
#include "FWCore/Framework/src/WorkerParams.h"
#include "FWCore/Framework/src/OutputWorker.h"

namespace edm {
  OutputWorker::OutputWorker(std::auto_ptr<OutputModule> mod,
			     ModuleDescription const& md,
			     WorkerParams const& wp):
      WorkerT<OutputModule>(mod, md, wp)
  {
  }

  OutputWorker::~OutputWorker() {
  }

  void
  OutputWorker::maybeEndFile() {
    module().maybeEndFile();
  }

  void
  OutputWorker::closeFile() {
    module().doCloseFile();
  }

  void
  OutputWorker::openNewFileIfNeeded() {
    module().maybeOpenFile();
  }

  void
  OutputWorker::openFile(FileBlock const& fb) {
    module().doOpenFile(fb);
  }

  void
  OutputWorker::writeRun(RunPrincipal const& rp) {
    module().doWriteRun(rp);
  }

  void
  OutputWorker::writeLumi(LuminosityBlockPrincipal const& lbp) {
    module().doWriteLuminosityBlock(lbp);
  }

  bool OutputWorker::wantAllEvents() const {return module().wantAllEvents();}

  bool OutputWorker::limitReached() const {return module().limitReached();}

  void OutputWorker::configure(OutputModuleDescription const& desc) {module().configure(desc);}
}
