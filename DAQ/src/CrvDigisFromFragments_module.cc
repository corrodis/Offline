// ======================================================================
//
// CrvDigisFromFragments_plugin:  Add CRV data products to the event
//
// ======================================================================

#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "fhiclcpp/ParameterSet.h"

#include "art/Framework/Principal/Handle.h"
//#include "artdaq-core-mu2e/Overlays/CRVFragment.hh"
#include "Offline/RecoDataProducts/inc/CrvDigi.hh"
#include "Offline/DataProducts/inc/CrvDAQStatus.hh"
#include <artdaq-core/Data/Fragment.hh>

#include "Offline/DAQ/inc/CRVDAQUtilities.hh"

#include <iostream>

#include <string>

#include <memory>

#define LONG_FROM_CRV 0 // Whether to print long-form data or single-line summary

namespace art {
class CrvDigisFromFragments;
}

using art::CrvDigisFromFragments;

// ======================================================================

class art::CrvDigisFromFragments : public EDProducer {

public:
  struct Config {
    fhicl::Atom<int> diagLevel{fhicl::Name("diagLevel"), fhicl::Comment("diagnostic Level")};
    fhicl::Atom<art::InputTag> crvFragmentsTag{fhicl::Name("crvTag"),
                                               fhicl::Comment("crv Fragments Tag")};
  };

  // --- C'tor/d'tor:
  explicit CrvDigisFromFragments(const art::EDProducer::Table<Config>& config);
  virtual ~CrvDigisFromFragments() {}

  // --- Production:
  virtual void produce(Event&);

private:
//  int decompressCrvDigi(uint8_t adc);
  int16_t decompressCrvDigi(int16_t adc);

  int diagLevel_;
  uint64_t _EventWindowTag; // in future mu2e::EWT

  art::InputTag crvFragmentsTag_;

  mu2e::CRVDAQUtilities crvDAQUtil_;

}; // CrvDigisFromFragments

// ======================================================================

CrvDigisFromFragments::CrvDigisFromFragments(const art::EDProducer::Table<Config>& config) :
    art::EDProducer{config}, diagLevel_(config().diagLevel()),
    crvFragmentsTag_(config().crvFragmentsTag()),
     crvDAQUtil_("CRVHitsFromFragments") {
  produces<EventNumber_t>();
  produces<mu2e::CrvDigiCollection>();
  produces<mu2e::CrvDAQStatusCollection>();
}

// ----------------------------------------------------------------------

int16_t CrvDigisFromFragments::decompressCrvDigi(int16_t adc)
{
  return adc;
}

void CrvDigisFromFragments::produce(Event& event) {

  art::EventNumber_t eventNumber = event.event();

  auto crvFragments = event.getValidHandle<artdaq::Fragments>(crvFragmentsTag_);
  //size_t numCrvFrags = crvFragments->size();

  if (diagLevel_ > 1) {
    std::cout << std::dec << "Producer: Run " << event.run() << ", subrun " << event.subRun()
              << ", event " << eventNumber << " has " << std::endl;
    std::cout << crvFragments->size() << " fragments." << std::endl;
    //std::cout << crvFragments->size() << " CRV fragments." << std::endl;
  }

  std::unique_ptr<mu2e::CrvDigiCollection> crv_digis(new mu2e::CrvDigiCollection);
  std::unique_ptr<mu2e::CrvDAQStatusCollection> crv_status(new mu2e::CrvDAQStatusCollection);

  // temp code
  for (const auto& artDaqFragment : (*crvFragments)) {
    auto ptr = new DTCLib::DTC_Event(artDaqFragment.dataBeginBytes());
    ptr->SetupEvent();
    _EventWindowTag = ptr->GetEventWindowTag().GetEventWindowTag(true);
    std::vector<DTCLib::DTC_DataBlock> crvBlocks; // what we will get from GetSubsystemDataBlocks
    for(size_t idx = 0; idx < ptr->GetSubEventCount(); ++idx) {
      auto subevt = ptr->GetSubEvent(idx);
      //std::cout << "subevt->GetSubsystem()=" << subevt->GetSubsystem() << ", DTCLib::DTC_Subsystem_CRV=" << DTCLib::DTC_Subsystem_CRV << std::endl;
      //if(subevt->GetSubsystem() == DTCLib::DTC_Subsystem_CRV) { // preliminary data waw not taken with dtc id 32
        auto subevtblocks = subevt->GetDataBlocks();
        crvBlocks.insert(crvBlocks.end(), subevtblocks.begin(), subevtblocks.end());
      //}
    }

    // the code above will be replaced with something like
    // auto evt = bb.get_data();
    // auto crvBlocks = evt.get_subsystem_data(DTCLib::DTC_Subsystem_CRV);

    // this code below will stay
    std::cout << "Processing a DTCEVT with " << crvBlocks.size() << " CRV blocks." << std::endl;
    for(auto& block : crvBlocks) {
      mu2e::CRVFragment cc(block.blockPointer, block.byteSize);

      if (diagLevel_ > 1) {
        crvDAQUtil_.printCrvFragmentInfo(cc);
      }

      for (size_t curBlockIdx = 0; curBlockIdx < cc.block_count(); curBlockIdx++) {
        auto block = cc.dataAtBlockIndex(curBlockIdx);
        if (block == nullptr) {
          std::cerr << "Unable to retrieve block " << curBlockIdx << "!" << std::endl;
          continue;
        }

        auto hdr = block->GetHeader();
        //if (diagLevel_ > 1) {
        //  crvDAQUtil_.printCrvFragmentHeader(hdr);
        //}

        if (diagLevel_ > 1) {
          for (int ii = 0; ii < hdr->GetPacketCount()+1; ++ii) {
            std::cout << std::hex;
            std::cout << DTCLib::DTC_DataPacket(((uint8_t*)block->blockPointer) + ((ii + 0) * 16)).toJSON() << std::endl;
            std::cout << std::dec;
          }
        }


        if (hdr->GetSubsystemID() != 2) {
          throw cet::exception("DATA") << " CRV packet does not have system ID 2";
        }

        // Parse phyiscs information from the CRV packets
        if (hdr->GetPacketCount() > 0) {
          auto crvRocHdr = cc.GetCRVROCStatusPacket(curBlockIdx);
          if (crvRocHdr == nullptr) {
            std::cerr << "Error retrieving CRV ROC Status Packet from DataBlock " << curBlockIdx
                      << "!" << std::endl;
            continue;
          }
          if (diagLevel_ > 1) {
            //crvDAQUtil_.printCrvROCStatus(*crvRocHdr);
            //crvDAQUtil_.printCrvROCStatus(cc, curBlockIdx);

            std::cout << std::hex << std::setfill('0') << std::setw(2);
            std::cout << "CRV ROC Status" << std::endl;
            std::cout << "unused          0x" << +crvRocHdr->unused1 << std::endl;
            std::cout << "PacketType      0x" << +crvRocHdr->PacketType << std::endl;
            std::cout << "ControllerID    0x" << +crvRocHdr->ControllerID << std::endl;
            std::cout << "ControllerEvCnt 0x" << crvRocHdr->ControllerEventWordCount << std::endl;
            std::cout << "unused          0x" << +crvRocHdr->unused2 << std::endl;
            std::cout << "Active FEBs     0x" << +crvRocHdr->ActiveFEBFlags2 << " " << +crvRocHdr->ActiveFEBFlags1 << " " << +crvRocHdr->ActiveFEBFlags0 << std::endl;
            std::cout << "TriggerCount    0x" << crvRocHdr->TriggerCount << std::endl;
            std::cout << "Status          0x" << +crvRocHdr->Status2 << +crvRocHdr->Status << std::endl;
            std::cout << "EvWindowTag     0x" << crvRocHdr->MicroBunchNumberHigh << " " << crvRocHdr->MicroBunchNumberLow << std::endl;
            std::cout << std::dec;
          }

          crv_status->emplace_back(+crvRocHdr->ControllerID,
                                   crvRocHdr->ControllerEventWordCount,
                                   ((uint32_t)crvRocHdr->ActiveFEBFlags2 << 16) + ((uint32_t)crvRocHdr->ActiveFEBFlags1 << 8) + crvRocHdr->ActiveFEBFlags0,
                                   crvRocHdr->TriggerCount,
                                   ((uint16_t)crvRocHdr->Status2 << 8) + crvRocHdr->Status,
                                   ((uint32_t)crvRocHdr->MicroBunchNumberHigh << 16) + crvRocHdr->MicroBunchNumberLow);

          auto crvHits = cc.GetCRVHitReadoutPackets(curBlockIdx);
          for (auto const& crvHit : crvHits) {

            // Fill the CrvDigiCollection
            // CrvDigi(const std::array<unsigned int, NSamples> &ADCs, unsigned int startTDC,
            //         mu2e::CRSScintillatorBarIndex scintillatorBarIndex, int SiPMNumber) :
            // TODO: This is a temporary implementation.
            // There will be a major change on the barIndex+SiPMNumber system,
            // which will be replaced by a channel ID system
            // Only a toy model is used here. The real implementation will follow.
            int channel = crvHit.SiPMID & 0x7F; // right 7 bits
            int FEB = crvHit.SiPMID >> 7;
            int crvBarIndex = (FEB * 64 + channel) / 4;
            int SiPMNumber = (FEB * 64 + channel) % 4;

            // TODO: This is a temporary implementation.
            if(crvHit.NumSamples!=8) {
              std::cerr<<"Number of samples is not 8!"<<std::endl;
              continue;
            }

            // TODO: This is a temporary implementation.
            std::array<int16_t, 8> adc;
            for (int j = 0; j < 8; j++) {
              adc[j] = decompressCrvDigi(crvHit.WaveformSamples[j].ADC);
            }
            crv_digis->emplace_back(adc, crvHit.HitTime, mu2e::CRSScintillatorBarIndex(crvBarIndex),
                                 SiPMNumber);
          }

          if (diagLevel_ > 1) {

            for (auto const& crvHit : crvHits) {

              // TODO: This is a temporary implementation.
              if(crvHit.NumSamples!=8) {
                std::cerr<<"Number of samples is not 8!"<<std::endl;
                continue;
              }
              #if LONG_FORM_CRV
              // TODO: This is a temporary implementation.
              // There will be a major change on the barIndex+SiPMNumber system,
              // which will be replaced by a channel ID system
              // Only a toy model is used here. The real implementation will follow.
              int channel = crvHit.SiPMID & 0x7F; // right 7 bits
              int FEB = crvHit.SiPMID >> 7;
              int crvBarIndex = (FEB * 64 + channel) / 4;
              int SiPMNumber = (FEB * 64 + channel) % 4;

              std::cout << "MAKEDIGI: " << SiPMNumber << " " << crvBarIndex << " " << crvHit.HitTime
                        << " " << crvHits.size() << " ";

              std::cout << "timestamp: " << hdr->GetEventWindowTag().GetEventWindowTag(true) << std::endl;
              std::cout << "hdr->SubsystemID: " << hdr->GetSubsystemID() << std::endl;
              std::cout << "hdr->DTCID: " << hdr->GetID() << std::endl;
              std::cout << "rocID: " << hdr->GetLinkID() << std::endl;
              std::cout << "packetCount: " << hdr->GetPacketCount() << std::endl;
              std::cout << "EVB mode: " << hdr->GetEVBMode() << std::endl;

              std::cout << "SiPMNumber: " << crvHit.SiPMID % 4 << std::endl;
              std::cout << "scintillatorBarIndex: " << crvHit.SiPMID / 4 << std::endl;
              std::cout << "TDC: " << crvHit.HitTime << std::endl;
              std::cout << "Waveform: {";
              // TODO: This is a temporary implementation.
              for (size_t j = 0; j < 8; j++) {
                std::cout << decompressCrvDigi(crvHit.WaveformSamples[j].ADC);
                if (j+1 < 8) std::cout << " ";
              }
              std::cout << "}" << std::endl;
              #else
              // Text format: timestamp sipmID tdc nsamples sample_list
              std::cout << "GREPMECRV: " << hdr->GetEventWindowTag().GetEventWindowTag(true) << " ";
              std::cout << crvHit.SiPMID << " ";
              std::cout << crvHit.HitTime << " ";
              // TODO: This is a temporary implementation.
              for (size_t j = 0; j < 8; j++) {
                std::cout << decompressCrvDigi(crvHit.WaveformSamples[j].ADC);
                if (j+1 < 8) std::cout << " ";
              }
              std::cout << std::endl;
              #endif
            }

            std::cout << "LOOP: " << eventNumber << " " << curBlockIdx << " "
                      << "(" << hdr->GetEventWindowTag().GetEventWindowTag(true) << ")" << std::endl;

          } // End debug output
        }   // End parsing CRV packets
      }
    }
  }

  if (diagLevel_ > 0) {
    std::cout << "mu2e::CrvDigisFromFragments::produce exiting eventNumber=" << (int)(event.event())
              << " / eventWindowTag=" << _EventWindowTag << std::endl;
  }

  event.put(std::unique_ptr<EventNumber_t>(new EventNumber_t(eventNumber)));

  // Store the crv digis in the event
  event.put(std::move(crv_digis));

  // Store the crv status in the event
  event.put(std::move(crv_status));

} // produce()

// ======================================================================

DEFINE_ART_MODULE(CrvDigisFromFragments)

// ======================================================================
