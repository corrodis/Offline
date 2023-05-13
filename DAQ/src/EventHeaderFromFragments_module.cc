// ======================================================================
//
// EventHeaderFromFragments_plugin:  Add Heeader data products to the event
// This is a temporary implementation
//
// ======================================================================

#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "fhiclcpp/ParameterSet.h"

#include "art/Framework/Principal/Handle.h"
//#include "Offline/DataProducts/inc/EventHeader.hh"
#include <artdaq-core-mu2e/Data/EventHeader.hh>
#include <artdaq-core/Data/Fragment.hh>

#include "Offline/DAQ/inc/CRVDAQUtilities.hh" // To be removed

#include <iostream>

#include <string>

#include <memory>

namespace art {
class EventHeaderFromFragments;
}

using art::EventHeaderFromFragments;

// ======================================================================

class art::EventHeaderFromFragments : public EDProducer {

public:
  struct Config {
    fhicl::Atom<int> diagLevel{fhicl::Name("diagLevel"), fhicl::Comment("diagnostic Level")};
    fhicl::Atom<art::InputTag> DTCEVTFragmentsTag{fhicl::Name("dtcevtTag"),
                                               fhicl::Comment("crv Fragments Tag")};
  };

  // --- C'tor/d'tor:
  explicit EventHeaderFromFragments(const art::EDProducer::Table<Config>& config);
  virtual ~EventHeaderFromFragments() {}

  // --- Production:
  virtual void produce(Event&);

private:

  int diagLevel_;
  uint64_t _EventWindowTag; // in future mu2e::EWT

  art::InputTag DTCEVTFragmentsTag_;

}; // CrvDigisFromFragments

// ======================================================================

EventHeaderFromFragments::EventHeaderFromFragments(const art::EDProducer::Table<Config>& config)
  : art::EDProducer{config}
  , diagLevel_(config().diagLevel())
  , DTCEVTFragmentsTag_(config().DTCEVTFragmentsTag()) {
      produces<EventNumber_t>();
      produces<mu2e::EventHeader>();
}

// ----------------------------------------------------------------------

void EventHeaderFromFragments::produce(Event& event) {

  art::EventNumber_t eventNumber = event.event();

  auto fragments = event.getValidHandle<artdaq::Fragments>(DTCEVTFragmentsTag_);

  if (diagLevel_ > 1) {
    std::cout << std::dec << "Producer: Run " << event.run() << ", subrun " << event.subRun()
              << ", event " << eventNumber << " has " << std::endl;
    std::cout << fragments->size() << " fragments." << std::endl;
  }

  // temp code
  for (const auto& artDaqFragment : (*fragments)) {
    auto dtcEventPtr = new DTCLib::DTC_Event(artDaqFragment.dataBeginBytes());
    dtcEventPtr->SetupEvent();
    _EventWindowTag = dtcEventPtr->GetEventWindowTag().GetEventWindowTag(true);
    //auto header = dtcEventPtr->GetHeader();

    auto hdr = std::unique_ptr<mu2e::EventHeader>(new mu2e::EventHeader(
                          dtcEventPtr->GetEventWindowTag().GetEventWindowTag(true),
                          dtcEventPtr->GetHeader()->event_mode >> 8,    // 40bits, might be wrong mapping);
                          dtcEventPtr->GetHeader()->dtc_mac & 0x8,      // not sure that's the right mapping
                          dtcEventPtr->GetHeader()->event_mode & 0x8)); // not sure that's the right mapping

    event.put(std::move(hdr));
  }

  if (diagLevel_ > 0) {
    std::cout << "mu2e::EventHeaderFromFragments::produce exiting eventNumber=" << (int)(event.event())
              << " / eventWindowTag=" << _EventWindowTag << std::endl;
  }

  event.put(std::unique_ptr<EventNumber_t>(new EventNumber_t(eventNumber)));
} // produce()

// ======================================================================

DEFINE_ART_MODULE(EventHeaderFromFragments)

// ======================================================================
