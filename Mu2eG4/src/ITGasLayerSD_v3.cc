// Mu2e includes
#include "Mu2eG4/inc/ITGasLayerSD_v3.hh"

#include "TMath.h"
#include "TRandom.h"

using namespace std;

namespace mu2e {

ITGasLayerSD_v3::ITGasLayerSD_v3(G4String name) : ITGasLayerSD(name){
}

G4bool ITGasLayerSD_v3::ProcessHits(G4Step* aStep,G4TouchableHistory*){

	int ring = _ring;
	//	if(ring==0) ring++;/*return false;*/
	G4double edep = aStep->GetTotalEnergyDeposit();

	// Eventually we will want this but not now.
	//if(edep==0.) return false;

	// Origin of the ITracker.  Need to get this from G4.
	static G4ThreeVector detectorOrigin( -3904., -7350., 6200.);

	// Position at start of step point, in world system and in
	// a system in which the center of the tracking detector is the origin.
	G4ThreeVector prePosWorld   = aStep->GetPreStepPoint()->GetPosition();
	G4ThreeVector prePosTracker = prePosWorld - detectorOrigin;

	G4ThreeVector preMomWorld = aStep->GetPreStepPoint()->GetMomentum();

	GeomHandle<ITracker> itracker;

	G4ThreeVector pos(prePosTracker);

	double phihit=TMath::ATan2(pos[1],pos[0]);
	double hit[4]={pos[0],pos[1],pos[2],aStep->GetPreStepPoint()->GetGlobalTime()};

	float xywire[3];
	double phiwire1=0;
	int wire1=-1;
	double cosstereo;
	double invcosstereo;

	try {
//		std::cout<<"S "<<_superlayer<<" R "<<ring<<std::endl;
		itracker->getCellGeometryHandle()->SelectCell(_superlayer,ring,0);

		cosstereo=TMath::Cos(itracker->getCellGeometryHandle()->GetWireEpsilon());
		invcosstereo=1.0/cosstereo;
		itracker->getCellGeometryHandle()->WirePosAtZ(pos[2]/**invcosstereo*/,xywire);

//		std::cout<<"0 wire center "<<xywire[0]<<" "<<xywire[1]<<" "<<xywire[2]<<std::endl;
//		std::cout<<"hit pos "<<pos[0]<<" "<<pos[1]<<" "<<pos[2]<<std::endl;
//		std::cout<<"hit 0 wire dist "<<sqrt(pow(pos[0]-xywire[0],2)+pow(pos[1]-xywire[1],2)+pow(pos[2]-xywire[2],2))<<std::endl;

//		//---------------- test --------------------
//		//TRandom *rn = new TRandom();
//		pos[0]=xywire[0]+gRandom->Uniform(-6,6);
//		pos[1]=xywire[1]+gRandom->Uniform(-6,6);
//		hit[0]=pos[0];
//		hit[1]=pos[1];
//		prePosTracker[0]=pos[0];
//		prePosTracker[1]=pos[1];
//		phihit=TMath::ATan2(pos[1],pos[0]);
//		//------------------------------------------

		phiwire1=TMath::ATan2(xywire[1],xywire[0]);

//		std::cout<<"phihit "<<phihit <<" phiwire1 "<<phiwire1 <<" _Dphi "<<_Dphi<<std::endl;
		phihit-=phiwire1;
		int wire;
		wire = TMath::Nint(phihit/_Dphi);
		//		wire = TMath::Nint(phihit/Dphi);

		if (wire<0) wire+=_nwires;

		double tdist, mdist;
		mdist=1.0e+9;
		int maxW = _nwires-1;

		itracker->getCellGeometryHandle()->SelectCell(_superlayer,ring,wire);
		mdist=itracker->getCellGeometryHandle()->DistFromWireCenter(hit);
		wire1=wire;
		--wire1;
		if (wire1<0) wire1+=_nwires;
		itracker->getCellGeometryHandle()->SelectCell(_superlayer,ring,wire1);
		tdist=itracker->getCellGeometryHandle()->DistFromWireCenter(hit);
		if (mdist>tdist) {
			mdist=tdist;
			wire=wire1;
		}else {
			++wire1;
			++wire1;
			if (wire1>maxW) wire1-=_nwires;
			itracker->getCellGeometryHandle()->SelectCell(_superlayer,ring,wire1);
			tdist=itracker->getCellGeometryHandle()->DistFromWireCenter(hit);
			if (mdist>tdist) {
				mdist=tdist;
				wire=wire1;
			}
		}
		if (mdist>itracker->getCellGeometryHandle()->GetCellRad()) return false;

//		std::cout<<"selected wire "<<ring<<" "<<wire<<std::endl;

		unsigned long det=itracker->getCellGeometryHandle()->computeDet(_superlayer,ring,wire);

		StepPointG4* newHit =
				new StepPointG4( aStep->GetTrack()->GetTrackID(),
						det,
						edep,
						prePosTracker,
						preMomWorld,
                                                aStep->GetPreStepPoint()->GetGlobalTime(),
                                                aStep->GetStepLength()
				);

		// The collection takes ownership of the hit.
		_collection->insert( newHit );

		newHit->Print();
		//newHit->Draw();

		return true;
	}catch (cms::Exception e) {
		cerr<<e;
		return false;
	}

}

} //namespace mu2e

