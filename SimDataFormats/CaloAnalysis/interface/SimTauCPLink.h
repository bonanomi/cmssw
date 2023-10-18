#ifndef SimDataFormats_SimTauCPLink_h
#define SimDataFormats_SimTauCPLink_h

#include "DataFormats/HGCalReco/interface/TICLCandidate.h"
#include "SimDataFormats/CaloAnalysis/interface/CaloParticleFwd.h"
#include "SimDataFormats/CaloAnalysis/interface/CaloParticle.h"
#include "SimDataFormats/CaloAnalysis/interface/SimCluster.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

class SimTauCPLink {
public:
  SimTauCPLink(){};
  ~SimTauCPLink(){};
  struct DecayNav {
    int pdgId_;
    int resonance_idx_;
    int calo_particle_idx_;
    int gen_particle_idx_;
    int pdgId() const { return pdgId_; }
    int resonance_idx() const { return resonance_idx_; }
    int calo_particle_idx() const { return calo_particle_idx_; }
    int gen_particle_idx() const { return gen_particle_idx_; }
  };

  std::vector<std::pair<int, int>> resonances;
  std::vector<DecayNav> leaves;
  CaloParticleRefVector calo_particle_leaves;
  int decayMode;

  enum decayModes {
    kNull = -1,
    kOneProng0PiZero,
    kOneProng1PiZero,
    kOneProng2PiZero,
    kOneProng3PiZero,
    kOneProngNPiZero,
    kTwoProng0PiZero,
    kTwoProng1PiZero,
    kTwoProng2PiZero,
    kTwoProng3PiZero,
    kTwoProngNPiZero,
    kThreeProng0PiZero,
    kThreeProng1PiZero,
    kThreeProng2PiZero,
    kThreeProng3PiZero,
    kThreeProngNPiZero,
    kRareDecayMode,
    kElectron,
    kMuon
  };

  void dump(void) const {
    for (auto const &l : leaves) {
      LogDebug("SimTauProducer") << "L " + std::to_string(l.pdgId()) + " " + std::to_string(l.resonance_idx()) +
                                        " CP: " + std::to_string(l.calo_particle_idx()) +
                                        " GenP idx: " + std::to_string(l.gen_particle_idx());
    }
    for (auto const &r : resonances) {
      LogDebug("SimTauProducer") << "R " + std::to_string(r.first) + " " + std::to_string(r.second);
    }
  }

  void dumpDecay(const std::pair<int, int> &entry) const {
    if (entry.second == -1) {  // No intermediate mother.
      LogDebug("SimTauProducer") << std::to_string(entry.first) + " " + std::to_string(entry.second);
    } else {
      LogDebug("SimTauProducer") << std::to_string(entry.first) + " " + std::to_string(entry.second) + " coming from: ";
      auto const &mother = resonances[entry.second];
      dumpDecay(mother);
    }
  }

  void dumpFullDecay(void) const {
    for (auto const &leaf : leaves) {
      dumpDecay({leaf.pdgId(), leaf.resonance_idx()});
    }
  }

private:
};

#endif  //SimTauCPLink
