
#include "art/Framework/Principal/Event.h"
#include "art_root_io/TFileService.h"
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "Offline/RecoDataProducts/inc/CrvDigi.hh"
#include "Offline/DataProducts/inc/CrvDAQStatus.hh"
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
    std::string _crvDigiModuleLabel;
    std::string _crvDAQStatusModuleLabel;

    TTree *tOutDigi;
    TTree *tOutStatus;
    crvDigiOut _crvDigi;
    crvStatusOut _crvStatus;
    void clearStructs();

  };

  mu2e::CRVDigiDumper::CRVDigiDumper(fhicl::ParameterSet const& pset)
    : art::EDAnalyzer(pset)
    , _crvDigiModuleLabel(pset.get<std::string>("crvDigiModuleLabel","crvDigi"))
    , _crvDAQStatusModuleLabel(pset.get<std::string>("crvDigiModuleLabel","crvDigi"))
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

    // dump crvDigis
    art::Handle<CrvDigiCollection> crvDigiCollection;
    event.getByLabel(_crvDigiModuleLabel,"",crvDigiCollection);
    if(crvDigiCollection.isValid()) {
      for(auto const & digis : *crvDigiCollection) {
        _crvDigi.eventWindowTag = 0; // TODO
        _crvDigi.sipm           = digis.GetSiPMNumber();
        _crvDigi.barindex       = digis.GetScintillatorBarIndex().asInt();
        _crvDigi.time           = digis.GetStartTDC();
        std::copy(digis.GetADCs().data(), digis.GetADCs().data() + digis.GetADCs().size(), _crvDigi.hits);
        std::cout << "SiPM: " <<  _crvDigi.sipm << std::endl;
        tOutDigi->Fill();
      }
    }

    // dump crvDAQStatus
    art::Handle<CrvDAQStatusCollection> crvDAQStatusCollection;
    event.getByLabel(_crvDAQStatusModuleLabel,"CrvDigi",crvDAQStatusCollection);
    if(crvDAQStatusCollection.isValid()) {
      for(auto const & s : *crvDAQStatusCollection) {
        _crvStatus.eventWindowTag    = 0; // TODO
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
