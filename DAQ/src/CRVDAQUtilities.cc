//#include "artdaq-core-mu2e/Overlays/CRVFragment.hh"
#include "artdaq-core-mu2e/Overlays/FragmentType.hh"
#include <artdaq-core/Data/Fragment.hh>

#include "Offline/DAQ/inc/CRVDAQUtilities.hh"
#include <iostream>


// TO BE REMOVED
std::unique_ptr<mu2e::CRVFragment::CRVROCStatusPacket> mu2e::CRVFragment::GetCRVROCStatusPacket(size_t blockIndex) const
{
  auto dataPtr = dataAtBlockIndex(blockIndex);
  if (dataPtr == nullptr) return nullptr;

  std::unique_ptr<CRVROCStatusPacket> output(nullptr);
  output.reset(new CRVROCStatusPacket(*reinterpret_cast<CRVROCStatusPacket const*>(dataPtr->GetData())));
  return output;
}

std::vector<mu2e::CRVFragment::CRVHitReadoutPacket> mu2e::CRVFragment::GetCRVHitReadoutPackets(size_t blockIndex) const
{
  auto dataPtr = dataAtBlockIndex(blockIndex);
  if (dataPtr == nullptr) return std::vector<CRVHitReadoutPacket>();

  auto crvRocHdr = reinterpret_cast<CRVROCStatusPacket const*>(dataPtr->GetData());
        size_t nHits = 0;
        if(2*crvRocHdr->ControllerEventWordCount>sizeof(CRVROCStatusPacket))
         nHits = (2*crvRocHdr->ControllerEventWordCount-sizeof(CRVROCStatusPacket)) / sizeof(CRVHitReadoutPacket);

  std::vector<CRVHitReadoutPacket> output(nHits);

  memcpy(&output[0], reinterpret_cast<CRVHitReadoutPacket const*>(crvRocHdr + 1), nHits * sizeof(CRVHitReadoutPacket));

  return output;
}


namespace mu2e {
  CRVDAQUtilities::CRVDAQUtilities(std::string ModuleName):moduleName_(ModuleName){}

  void CRVDAQUtilities::printCrvFragmentInfo(CRVFragment const& cc){
    std::cout << std::endl;
    std::cout << "ArtFragmentReader: ";
    std::cout << "\tBlock Count: " << std::dec << cc.block_count() << std::endl;
    //std::cout << "\tByte Count: " << artDaqFragment.dataSizeBytes() << std::endl;
    std::cout << std::endl;
    std::cout << "\t"
              << "====== Example Block Sizes ======" << std::endl;
    for (size_t i = 0; i < 10; i++) {
      if (i < cc.block_count()) {
        std::cout << "\t" << i << "\t" << cc.blockSizeBytes(i) << std::endl;
      }
    }
    std::cout << "\t"
              << "=========================" << std::endl;
  }

  //void printCrvROCStatus(mu2e::CRVFragment::CRVROCStatusPacket const& hdr) {
  void printCrvROCStatus(CRVFragment const& cc, size_t curBlockIdx) {
    auto hdr = cc.GetCRVROCStatusPacket(curBlockIdx);
    std::cout << std::hex;
    std::cout << "PacketType      0x" << hdr->PacketType << std::endl;
    std::cout << "ControllerID    0x" << hdr->ControllerID << std::endl;
    std::cout << "ControllerEvCnt 0x" << hdr->ControllerEventWordCount << std::endl;
    std::cout << "Active FEBs     0x" << hdr->ActiveFEBFlags2 << hdr->ActiveFEBFlags1 << hdr->ActiveFEBFlags0 << std::endl;
    std::cout << "TriggerCount    0x" << hdr->TriggerCount << std::endl;
    std::cout << "Status          0x" << hdr->Status2 << hdr->Status << std::endl;
    std::cout << "EvWindowTag     0x" << hdr->MicroBunchNumberHigh << hdr->MicroBunchNumberLow << std::endl;
    std::cout << std::dec;
  }
};
