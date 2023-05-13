
#include "art/Framework/Principal/Event.h"
#include "art_root_io/TFileService.h"
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "Offline/RecoDataProducts/inc/CrvDigi.hh"
#include "Offline/DataProducts/inc/CrvDAQStatus.hh"
#include <artdaq-core-mu2e/Data/EventHeader.hh>
#include "TTree.h"
#include <iostream>
#include <fstream>
#include <string>

namespace mu2e {

  // crvDigi root struct, use root types
  struct crvDigiOut {
    ULong64_t eventWindowTag;
    Int_t     barindex;
    UChar_t   sipm;
    UChar_t   time;
    Char_t    hits[8];
  };
  const char * const crvDigiOut_str = "eventWindowTag/l:barindex/I:sipm/b:time/b:hits[8]/B";

  struct crvStatusOut {
    ULong64_t eventWindowTag;
    UInt_t    eventWindowTagRoc;
    UInt_t    activeFEBs;
    UShort_t  triggerCount;
    UShort_t  status;
    UShort_t  wordCount;
    UChar_t   controllerID;
  };
  const char * const crvStatusOut_str = "eventWindowTag/l:eventWindowTagRoc/i:activeFEBs/i:triggerCount/s:status/s:wordCount/s:controllerID/b";

  class CRVDigiDumper : public art::EDAnalyzer {

  public:
    explicit CRVDigiDumper(fhicl::ParameterSet const& );

    void beginRun(art::Run const& run) override;
    void endRun(art::Run const& run) override;
    void endJob() override;
    void analyze(art::Event const& event) override;

  private:
    int _diagLevel;
    std::string _crvDigiModuleLabel;
    std::string _crvDAQStatusModuleLabel;
    std::string _eventHeaderModuleLabel;

    TTree *tOutDigi;
    TTree *tOutStatus;
    crvDigiOut _crvDigi;
    crvStatusOut _crvStatus;
    void clearStructs();

  };

  mu2e::CRVDigiDumper::CRVDigiDumper(fhicl::ParameterSet const& pset)
    : art::EDAnalyzer(pset)
    , _diagLevel(pset.get<int>("diagLevel",0))
    , _crvDigiModuleLabel(pset.get<std::string>("crvDigiModuleLabel","crvDigi"))
    , _crvDAQStatusModuleLabel(pset.get<std::string>("crvDigiModuleLabel","crvDigi"))
    , _eventHeaderModuleLabel(pset.get<std::string>("eventHeaderModuleLabel","eventHeader"))
    , tOutDigi(nullptr)
    , tOutStatus(nullptr)
    {
  }

  void mu2e::CRVDigiDumper::beginRun(art::Run const& run) {
    art::ServiceHandle<art::TFileService> tfs;
    tOutDigi = tfs->make<TTree>("crvDigi","crvDigi");
    tOutDigi->Branch("crvDigi",&_crvDigi.eventWindowTag,crvDigiOut_str);

    tOutStatus = tfs->make<TTree>("crvStatus","crvStatus");
    tOutStatus->Branch("crvStatus",&_crvStatus.eventWindowTag, crvStatusOut_str);

    gDirectory->Append(tOutDigi);
    gDirectory->Append(tOutStatus);

    clearStructs();
  }

  void mu2e::CRVDigiDumper::analyze(art::Event const& event) {

    // header, if avaiable
    art::Handle<mu2e::EventHeader> header;
    event.getByLabel(_eventHeaderModuleLabel,"",header);
    art::Handle<CrvDigiCollection> crvDigiCollection;
    event.getByLabel(_crvDigiModuleLabel,"",crvDigiCollection);
    art::Handle<CrvDAQStatusCollection> crvDAQStatusCollection;
    event.getByLabel(_crvDAQStatusModuleLabel,"",crvDAQStatusCollection);

    if (_diagLevel > 1) {
      if(header.isValid()) {
        std::cout << "Event header found: event window tag: " << header->ewt;
        if(header->isOnSpill()) std::cout << " (on spill)";
        else std::cout << " (off spill): ";
        std::cout << "." << std::endl;
      }
      std::cout << "Dumping ";
      if(crvDAQStatusCollection.isValid()) std::cout << (*crvDigiCollection).size() << " crvDigis, ";
      if(crvDAQStatusCollection.isValid()) std::cout << (*crvDAQStatusCollection).size() << " crvDAQStatus, ";
      std::cout << "events.";
    }

    // dump crvDigis
    if(crvDigiCollection.isValid()) {
      for(auto const & digis : *crvDigiCollection) {
        if(header.isValid()) _crvDigi.eventWindowTag = header->ewt;
        else                 _crvDigi.eventWindowTag = event.event();
        _crvDigi.sipm           = digis.GetSiPMNumber();
        _crvDigi.barindex       = digis.GetScintillatorBarIndex().asInt();
        _crvDigi.time           = digis.GetStartTDC();
        std::copy(digis.GetADCs().data(), digis.GetADCs().data() + digis.GetADCs().size(), _crvDigi.hits);
        std::cout << "SiPM: " <<  _crvDigi.sipm << std::endl;
        tOutDigi->Fill();
      }
    }

    // dump crvDAQStatus
    if(crvDAQStatusCollection.isValid()) {
      for(auto const & s : *crvDAQStatusCollection) {
        if(header.isValid()) _crvStatus.eventWindowTag = header->ewt;
        else                _crvStatus.eventWindowTag = event.event();
        _crvStatus.eventWindowTagRoc = s.GetEventWindowTag();
        _crvStatus.activeFEBs        = s.GetActiveFEBs();
        _crvStatus.triggerCount      = s.GetTriggerCount();
        _crvStatus.status            = s.GetStatus();
        _crvStatus.wordCount         = s.GetWordCount();
        _crvStatus.controllerID      = s.GetDTCId();
        tOutStatus->Fill();
      }
    }
  }

  void mu2e::CRVDigiDumper::endJob() { }

  void mu2e::CRVDigiDumper::endRun(art::Run const& run) {  }

  void mu2e::CRVDigiDumper::clearStructs() { }

}; // end mu2e namespace

DEFINE_ART_MODULE(mu2e::CRVDigiDumper)
