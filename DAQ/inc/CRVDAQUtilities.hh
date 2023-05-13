
#ifndef DAQ_CRVDAQUtilities_hh
#define  DAQ_CRVDAQUtilities_hh


//#include "artdaq-core-mu2e/Overlays/CRVFragment.hh"
#include "artdaq-core-mu2e/Overlays/FragmentType.hh"
#include <artdaq-core/Data/Fragment.hh>



// BELOW: TO BE REEMOED

#include "artdaq-core-mu2e/Overlays/ArtFragment.hh"
#include <memory>
#include <vector>
#include <array>

namespace mu2e {
class CRVFragment : public ArtFragment
{
public:
  explicit CRVFragment(artdaq::Fragment const& f)
    : ArtFragment(f) {}

  CRVFragment(const void* ptr, size_t sz)
    : ArtFragment(ptr, sz) {}

  explicit CRVFragment(std::pair<const void*, size_t> p)
    : CRVFragment(p.first, p.second) {}

  struct CRVROCStatusPacket
  {
    uint8_t PacketType : 4;  // == 0x6
    uint8_t ControllerID: 4;
    uint8_t unused1;

    uint16_t ControllerEventWordCount;

    uint8_t ActiveFEBFlags2;
    uint8_t unused2;

    uint8_t ActiveFEBFlags0;
    uint8_t ActiveFEBFlags1;

    uint16_t TriggerCount;

    uint8_t Status2;
    uint8_t Status;

    //uint8_t unused4;
    //uint8_t unused5;

    //uint8_t Errors;
    ///uint8_t EventType;
                uint16_t MicroBunchNumberHigh;
                uint16_t MicroBunchNumberLow;

    CRVROCStatusPacket()
       : PacketType(0)
       , ControllerID(0)
       , unused1(0)
       , ControllerEventWordCount(0)
       , ActiveFEBFlags2(0)
       , unused2(0)
       , ActiveFEBFlags0(0)
       , ActiveFEBFlags1(0)
       , TriggerCount(0)
                        , Status2(0)
       , Status(0)
       //, unused3(0)
       //, unused4(0)
       //, unused5(0)
       //, Errors(0)
       //, EventType(0)
       , MicroBunchNumberHigh(0)
       , MicroBunchNumberLow(0)
    {}
  };

  struct CRVHitWaveformSample
  {
    int16_t ADC : 12;
    int16_t unused : 4;
    CRVHitWaveformSample()
      : ADC(0)
                        , unused(0)
    {}
  };

  struct CRVHitReadoutPacket
  {
    uint16_t SiPMID;
    uint16_t HitTime : 12;
    uint16_t NumSamples : 4;

    CRVHitWaveformSample  WaveformSamples[8];

    CRVHitReadoutPacket()
      : SiPMID(0)
      , HitTime(0)
                        , NumSamples(0)
                {}
  };

  std::unique_ptr<CRVROCStatusPacket> GetCRVROCStatusPacket(size_t blockIndex) const;
  std::vector<CRVHitReadoutPacket> GetCRVHitReadoutPackets(size_t blockIndex) const;
};
}  // namespace mu2e


namespace mu2e {

  class CRVDAQUtilities{

  public:
    CRVDAQUtilities(std::string ModuleName);

    void printCrvFragmentInfo(CRVFragment const&);
    void printCrvROCStatus(CRVFragment const& cc, size_t curBlockIdx);
    //void printCrvROCStatus(CRVFragment::CRVROCStatusPacket const&);

  private:
    std::string moduleName_;
  };
}

#endif  // DAQ_CRVDAQUtilities_hh
