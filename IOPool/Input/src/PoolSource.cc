/*----------------------------------------------------------------------
$Id: PoolSource.cc,v 1.21 2006/02/08 00:44:28 wmtan Exp $
----------------------------------------------------------------------*/

#include "IOPool/Input/src/PoolSource.h"
#include "IOPool/Input/src/RootFile.h"
#include "IOPool/Common/interface/ClassFiller.h"

#include "DataFormats/Common/interface/BranchDescription.h"
#include "FWCore/Framework/interface/EventPrincipal.h"
#include "DataFormats/Common/interface/ProductRegistry.h"
#include "DataFormats/Common/interface/ProductID.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

namespace edm {
  PoolRASource::PoolRASource(ParameterSet const& pset, InputSourceDescription const& desc) :
    VectorInputSource(desc),
    catalog_(PoolCatalog::READ, pset.getUntrackedParameter("catalog", std::string())),
    files_(pset.getUntrackedParameter("fileNames", std::vector<std::string>())),
    fileIter_(files_.begin()),
    rootFile_(),
    rootFiles_(),
    maxEvents_(pset.getUntrackedParameter<int>("maxEvents", -1)),
    remainingEvents_(maxEvents_),
    mainInput_(pset.getParameter<std::string>("@module_label") == std::string("@main_input")),
    backwardJump_(false),
    forwardJump_(false) {
    ClassFiller();
    init(*fileIter_);
    if (mainInput_) {
      updateRegistry();
    }
  }

  void PoolRASource::init(std::string const& file) {

    // For the moment, we keep all old files open.
    // FIX: We will need to limit the number of open files.
    // rootFiles[file]_.reset();

    RootFileMap::const_iterator it = rootFiles_.find(file);
    if (it == rootFiles_.end()) {
      std::string pfn;
      catalog_.findFile(pfn, file);
      rootFile_ = RootFileSharedPtr(new RootFile(pfn));
      rootFiles_.insert(std::make_pair(file, rootFile_));
    } else {
      rootFile_ = it->second;
      rootFile_->setEntryNumber(-1);
    }
  }

  void PoolRASource::updateRegistry() const {
    if (rootFile_->productRegistry().nextID() > productRegistry().nextID()) {
      productRegistry().setNextID(rootFile_->productRegistry().nextID());
    }
    ProductRegistry::ProductList const& prodList = rootFile_->productRegistry().productList();
    for (ProductRegistry::ProductList::const_iterator it = prodList.begin();
	it != prodList.end(); ++it) {
      productRegistry().copyProduct(it->second);
    }
  }

  bool PoolRASource::next() {
    if(rootFile_->next()) return true;
    ++fileIter_;
    if(fileIter_ == files_.end()) {
      if (mainInput_) {
	return false;
      } else {
	fileIter_ = files_.begin();
      }
    }

    // save the product registry from the current file, temporarily
    boost::shared_ptr<ProductRegistry const> pReg(rootFile_->productRegistrySharedPtr());

    init(*fileIter_);

    // make sure the new product registry is identical to the old one
    if (*pReg != rootFile_->productRegistry()) {
      throw cms::Exception("MismatchedInput","PoolSource::next()")
	<< "File " << *fileIter_ << "\nhas different product registry than previous files\n";
    }
    return next();
  }

  bool PoolRASource::previous() {
    if(rootFile_->previous()) return true;
    if(fileIter_ == files_.begin()) {
      if (mainInput_) {
	return false;
      } else {
	fileIter_ = files_.end();
      }
    }
    --fileIter_;

    // save the product registry from the current file, temporarily
    boost::shared_ptr<ProductRegistry const> pReg(rootFile_->productRegistrySharedPtr());

    init(*fileIter_);

    // make sure the new product registry is identical to the old one
    if (*pReg != rootFile_->productRegistry()) {
      throw cms::Exception("MismatchedInput","PoolSource::previous()")
	<< "File " << *fileIter_ << "\nhas different product registry than previous files\n";
    }
    rootFile_->setEntryNumber(rootFile_->entries());
    return previous();
  }

  PoolRASource::~PoolRASource() {
  }

  // read() is responsible for creating, and setting up, the
  // EventPrincipal.
  //
  //   1. create an EventPrincipal with a unique EventID
  //   2. For each entry in the provenance, put in one Group,
  //      holding the Provenance for the corresponding EDProduct.
  //   3. set up the caches in the EventPrincipal to know about this
  //      Group.
  //
  // We do *not* create the EDProduct instance (the equivalent of reading
  // the branch containing this EDProduct. That will be done by the Delayed Reader,
  //  when it is asked to do so.
  //
  std::auto_ptr<EventPrincipal>
  PoolRASource::read() {
    // If we're done, or out of range, return a null auto_ptr
    if (remainingEvents_ == 0) {
      if (!mainInput_) {
	remainingEvents_ = maxEvents_;
      }
      return std::auto_ptr<EventPrincipal>(0);
    }
    if (!next()) {
      return std::auto_ptr<EventPrincipal>(0);
    }
    --remainingEvents_;
    return rootFile_->read(mainInput_ ? productRegistry() : rootFile_->productRegistry()); 
  }

  std::auto_ptr<EventPrincipal>
  PoolRASource::read(EventID const& id) {
    RootFile::EntryNumber entry = rootFile_->getEntryNumber(id);
    if (entry >= 0) {
      rootFile_->setEntryNumber(entry - 1);
      forwardJump_ = backwardJump_ = false;
      return read();
    } else {
      EventID current = rootFile_->eventID();
      if (current > id) {
	if (backwardJump_) {
	  forwardJump_ = backwardJump_ = false;
	  return std::auto_ptr<EventPrincipal>(0);
	}
	// Set the entry to the last entry in this file
	rootFile_->setEntryNumber(rootFile_->entries() -1);

	// Advance to the first entry of the next file, if there is a next file.
	if(!next()) {
	  forwardJump_ = backwardJump_ = false;
	  return std::auto_ptr<EventPrincipal>(0);
	}
	forwardJump_ = true;
      } else {
	if (forwardJump_) {
	  forwardJump_ = backwardJump_ = false;
	  return std::auto_ptr<EventPrincipal>(0);
	}
	// Set the entry to the first entry in this file
	rootFile_->setEntryNumber(0);

	// Back up to the last entry of the previous file, if there is a previous file.
	if(!previous()) {
	  forwardJump_ = backwardJump_ = false;
	  return std::auto_ptr<EventPrincipal>(0);
	}
	backwardJump_ = true;
      }	
      return read(id);
    }
  }

  // Advance "offset" events. Entry numbers begin at 0.
  // The current entry number is the last one read, not the next one read.
  // The current entry number may be -1, if none have been read yet.
  void
  PoolRASource::skip(int offset) {
    EntryNumber newEntry = rootFile_->entryNumber() + offset;
    if (newEntry >= rootFile_->entries()) {

      // We must go to the next file
      // Calculate how much we will advance in this file,
      // including one for the next() call below
      int increment = rootFile_->entries() - rootFile_->entryNumber();    

      // Set the entry to the last entry in this file
      rootFile_->setEntryNumber(rootFile_->entries() -1);

      // Advance to the first entry of the next file, if there is a next file.
      if(!next()) return;

      // Now skip the remaining offset.
      skip(offset - increment);

    } else if (newEntry < -1) {

      // We must go to the previous file
      // Calculate how much we will back up in this file,
      // including one for the previous() call below
      int decrement = rootFile_->entryNumber() + 1;    

      // Set the entry to the first entry in this file
      rootFile_->setEntryNumber(0);

      // Back up to the last entry of the previous file, if there is a previous file.
      if(!previous()) return;

      // Now skip the remaining offset.
      skip(offset + decrement);
    } else {
      // The same file.
      rootFile_->setEntryNumber(newEntry);
    }
  }

  void
  PoolRASource::readMany_(int number, EventPrincipalVector& result) {
    for (int i = 0; i < number; ++i) {
      std::auto_ptr<EventPrincipal> ev = read();
      if (ev.get() == 0) {
	return;
      }
      EventPrincipalVectorElement e(ev.release());
      result.push_back(e);
    }
  }
}
