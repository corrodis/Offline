//
// Free function to create  DS. (Detector Solenoid)
//
// $Id: constructDS.cc,v 1.8 2012/11/16 23:49:19 genser Exp $
// $Author: genser $
// $Date: 2012/11/16 23:49:19 $
//
// Original author KLG based on Mu2eWorld constructDS
//
// Notes:
// Construct the DS. Parent volume is the air inside of the hall.
// This makes 4 volumes:
//  0 - a single volume that represents the coils+cryostats in an average way.
//  1 - DS1, a small piece of DS vacuum that surrounds TS5.
//  2 - DS2, the upstream part of the DS vacuum, that has a graded field.
//  3 - DS3, the downstream part of the DS vacuum, that may have a uniform field.

// Mu2e includes.

#include "BeamlineGeom/inc/Beamline.hh"
#include "G4Helper/inc/VolumeInfo.hh"
#include "GeometryService/inc/GeomHandle.hh"
#include "GeometryService/inc/GeometryService.hh"
#include "Mu2eG4/inc/MaterialFinder.hh"
#include "Mu2eG4/inc/constructDS.hh"
#include "Mu2eG4/inc/nestTubs.hh"
#include "Mu2eG4/inc/finishNesting.hh"

// G4 includes
#include "G4ThreeVector.hh"
#include "G4Material.hh"
#include "G4Color.hh"
#include "G4Tubs.hh"
#include "G4SubtractionSolid.hh"

using namespace std;

namespace mu2e {

  void constructDS( const VolumeInfo& parent,
                    SimpleConfig const & _config
                    ){

    int const verbosityLevel = _config.getInt("toyDS.verbosityLevel",0);

    // Extract information from the config file.
    TubsParams detSolCoilParams( _config.getDouble("toyDS.rIn"),
                                 _config.getDouble("toyDS.rOut"),
                                 _config.getDouble("toyDS.halfLength"));

    GeomHandle<Beamline> beamg;
    double solenoidOffset = beamg->solenoidOffset();
    double rTorus         = beamg->getTS().torusRadius();
    double rCryo          = beamg->getTS().outerRadius();
    double ts5HalfLength  = beamg->getTS().getTS5().getHalfLength();

    double dsCoilZ0          = _config.getDouble("toyDS.z0");
    double ds1HalfLength     = _config.getDouble("toyDS1.halfLength");
    double ds2HalfLength     = _config.getDouble("toyDS2.halfLength");
    double ds3HalfLength     = _config.getDouble("toyDS3.halfLength");
    double dsFrontHalfLength = _config.getDouble("toyDS.frontHalfLength");

    // All Vacuum volumes fit inside the DS coil+cryostat package.
    // DS1 surrounds ts5.
    // DS2 and DS3 extend to r=0
    TubsParams dsFrontParams( rCryo,
                              detSolCoilParams.innerRadius(),
                              dsFrontHalfLength);

    TubsParams ds1VacParams( rCryo,
                             detSolCoilParams.innerRadius(),
                             ds1HalfLength);

    TubsParams ds2VacParams( 0.,
                             detSolCoilParams.innerRadius(),
                             ds2HalfLength);

    // Compute positions of objects in Mu2e coordinates.
    double dsFrontZ0 = rTorus + 2.*ts5HalfLength - 2.*ds1HalfLength - dsFrontHalfLength;
    double ds1Z0     = rTorus + 2.*ts5HalfLength - ds1HalfLength;
    double ds2Z0     = rTorus + 2.*ts5HalfLength + ds2HalfLength;

    // MBS

    // if MBS is there the vacuum is longer; we also add a minimal
    // length even when MBS is not there to have a legal subtraction
    // volume since other components (e.g. neutron absorber) depend on
    // it now

    double ds3OriginalHalfLength = ds3HalfLength;
    double ds3OriginalZ0         = ds2Z0  + ds2HalfLength  + ds3HalfLength;

    double static const addedLength = 1.0;

    double BSTSHLength = ds3HalfLength + addedLength;
    double BSTSZ       = ds3OriginalZ0 + addedLength;
    if ( _config.getBool("hasMBS",false) ) {
      BSTSHLength = _config.getDouble("mbs.BSTSHLength");
      BSTSZ       = _config.getDouble("mbs.BSTSZ");
    }

    ds3HalfLength = ( -ds2Z0 - ds2HalfLength + BSTSZ + BSTSHLength)*0.5;

    double ds3Z0     = ds2Z0 + ds2HalfLength + ds3HalfLength;

    // ds3 is a subtraction solid accommodating the MBS

    double ds3SubtrZ0         = ( ds3OriginalZ0  + ds3OriginalHalfLength + ds3Z0 + ds3HalfLength)*0.5;
    double ds3SubtrHalfLength = (-ds3OriginalZ0  - ds3OriginalHalfLength + ds3Z0 + ds3HalfLength)*0.5;

    TubsParams ds3VacParams( 0.,
                             detSolCoilParams.innerRadius(),
                             ds3HalfLength);

    double SPBSOuterRadius   = _config.getBool("hasMBS",false) ?
      _config.getDouble("mbs.SPBSOuterRadius") : 0.;
    TubsParams ds3VacSubtrParams( SPBSOuterRadius,
                                  detSolCoilParams.innerRadius(),
                                  ds3SubtrHalfLength);

    G4ThreeVector detSolCoilPosition(-solenoidOffset, 0., dsCoilZ0);
    G4ThreeVector    dsFrontPosition(-solenoidOffset, 0., dsFrontZ0);
    G4ThreeVector        ds1Position(-solenoidOffset, 0., ds1Z0);
    G4ThreeVector        ds2Position(-solenoidOffset, 0., ds2Z0);
    G4ThreeVector        ds3Position(-solenoidOffset, 0., ds3Z0);
    G4ThreeVector   ds3SubtrPosition(-solenoidOffset, 0., ds3SubtrZ0);


    if ( verbosityLevel > 0) {
      double zhl  =  ds3HalfLength;
      cout << __func__ << " ds3Vac Original Z Offset in Mu2e  : " << ds3OriginalZ0 << endl;
      cout << __func__ << " ds3OriginalHalfLength             : " << ds3OriginalHalfLength << endl;
      cout << __func__ << " ds3Vac Z Offset in Mu2e           : " << ds3Z0 << endl;
      cout << __func__ << " ds3HalfLength                     : " << zhl << endl;
      cout << __func__ << " ds3Vac Z extent in Mu2e           : " <<
        ds3Z0 - zhl << ", " << ds3Z0 + zhl << endl;
    }

    if ( verbosityLevel > 0) {
      double zhl  =  ds3SubtrHalfLength;
      cout << __func__ << " ds3Subtr Z Offset in Mu2e         : " << ds3SubtrZ0 << endl;
      cout << __func__ << " ds3SubtrHalfLength                : " << zhl << endl;
      cout << __func__ << " ds3Subtr Z extent in Mu2e         : " <<
        ds3SubtrZ0 - zhl << ", " << ds3SubtrZ0 + zhl << endl;
    }

    MaterialFinder materialFinder(_config);
    G4Material* detSolCoilMaterial = materialFinder.get("toyDS.materialName");
    G4Material* vacuumMaterial     = materialFinder.get("toyDS.insideMaterialName");

    // Single volume representing the DS coils + cryostat in an average way.

    bool toyDSVisible        = _config.getBool("toyDS.visible",true);
    bool toyDSSolid          = _config.getBool("toyDS.solid",true);
    bool forceAuxEdgeVisible = _config.getBool("g4.forceAuxEdgeVisible",false);
    bool doSurfaceCheck      = _config.getBool("g4.doSurfaceCheck",false);
    bool const placePV       = true;

    G4ThreeVector _hallOriginInMu2e = parent.centerInMu2e();

    VolumeInfo detSolCoilInfo = nestTubs( "ToyDSCoil",
                                          detSolCoilParams,
                                          detSolCoilMaterial,
                                          0,
                                          detSolCoilPosition-_hallOriginInMu2e,
                                          parent,
                                          0,
                                          toyDSVisible,
                                          G4Color::Magenta(),
                                          toyDSSolid,
                                          forceAuxEdgeVisible,
                                          placePV,
                                          doSurfaceCheck
                                          );

    // Upstream face of the DS coils+cryo.
    VolumeInfo dsFrontInfo    = nestTubs( "ToyDSFront",
                                          dsFrontParams,
                                          detSolCoilMaterial,
                                          0,
                                          dsFrontPosition-_hallOriginInMu2e,
                                          parent,
                                          0,
                                          toyDSVisible,
                                          G4Color::Blue(),
                                          toyDSSolid,
                                          forceAuxEdgeVisible,
                                          placePV,
                                          doSurfaceCheck
                                          );


    VolumeInfo ds1VacInfo     = nestTubs( "ToyDS1Vacuum",
                                          ds1VacParams,
                                          vacuumMaterial,
                                          0,
                                          ds1Position-_hallOriginInMu2e,
                                          parent,
                                          0,
                                          toyDSVisible,
                                          G4Colour::Green(),
                                          toyDSSolid,
                                          forceAuxEdgeVisible,
                                          placePV,
                                          doSurfaceCheck
                                          );

    VolumeInfo ds2VacInfo     = nestTubs( "ToyDS2Vacuum",
                                          ds2VacParams,
                                          vacuumMaterial,
                                          0,
                                          ds2Position-_hallOriginInMu2e,
                                          parent,
                                          0,
                                          toyDSVisible,
                                          G4Colour::Yellow(),
                                          toyDSSolid,
                                          forceAuxEdgeVisible,
                                          placePV,
                                          doSurfaceCheck
                                          );

    if ( verbosityLevel > 0) {
      double pzhl   = static_cast<G4Tubs*>(ds2VacInfo.solid)->GetZHalfLength();
      double ds2VacZ = ds2VacInfo.centerInMu2e()[CLHEP::Hep3Vector::Z];
      cout << __func__ << 
	" ds2Vac Z offset in Mu2e           : " << ds2VacZ << endl;
      cout << __func__ << 
	" ds2Vac Z extent in Mu2e           : " << ds2VacZ - pzhl  << ", " <<  ds2VacZ + pzhl  << endl;
    }

    // downstream vacuum part also enclosing the Muon Beam Stop
    // it is a subtraction solid with the "subtraction tub" placed downstream

    VolumeInfo ds3VacInfo;
    ds3VacInfo.name = "ToyDS3Vacuum";

    G4Tubs* ds3VacSolid = new G4Tubs(ds3VacInfo.name+"Full",
                                     ds3VacParams.innerRadius(),
                                     ds3VacParams.outerRadius(),
                                     ds3VacParams.zHalfLength(),
                                     ds3VacParams.phi0(),
                                     ds3VacParams.phiMax());

    G4Tubs* ds3VacSubtrSolid = new G4Tubs(ds3VacInfo.name+"Subtr",
                                          ds3VacSubtrParams.innerRadius(),
                                          ds3VacSubtrParams.outerRadius(),
                                          ds3VacSubtrParams.zHalfLength(),
                                          ds3VacSubtrParams.phi0(),
                                          ds3VacSubtrParams.phiMax());

    G4ThreeVector subtractedVolumeOffset = ds3SubtrPosition-ds3Position;

    if (subtractedVolumeOffset[CLHEP::Hep3Vector::Z]<0.0) {
      throw cet::exception("GEOM")
        << "Incorrect ds3 subtraction volume paramaters \n";
    }

    ds3VacInfo.solid = new G4SubtractionSolid(ds3VacInfo.name,
                                              ds3VacSolid, ds3VacSubtrSolid,0,subtractedVolumeOffset);

    if ( verbosityLevel > 0) {

      cout << __func__ << 
	" ds3VacParams.zHalfLength()        : " << ds3VacParams.zHalfLength() << endl;
      cout << __func__ << 
	" ds3VacSubtrParams.zHalfLength()   : " << ds3VacSubtrParams.zHalfLength() << endl;
      cout << __func__ << 
	" ds3VacSolid->GetZHalfLength()Full : " << ds3VacSolid->GetZHalfLength() << endl;
      cout << __func__ << 
	" ds3VacSubtrSolid->GetZHalfLength(): " << ds3VacSubtrSolid->GetZHalfLength() << endl;

      G4Tubs* solid0 = static_cast<G4Tubs*>(ds3VacInfo.solid->GetConstituentSolid(0));
      // note the intricacy of this call in a case of a shifted solid
      G4Tubs* solid1 = 
	static_cast<G4Tubs*>(static_cast<G4DisplacedSolid*>(ds3VacInfo.solid->GetConstituentSolid(1))
			     ->GetConstituentMovedSolid());
      cout << __func__ <<
	" ConstituentSolid(0) name          : " <<
	solid0->GetName() << endl;
      cout << __func__ <<
	" ConstituentSolid(1) name          : " <<
	solid1->GetName() << endl;

      cout << __func__ <<
	" ConstituentSolid(0)) zHalfLength  : " <<
	solid0->GetZHalfLength() << endl;
      cout << __func__ <<
	" ConstituentSolid(1)) zHalfLength  : " <<
	solid1->GetZHalfLength() << endl;

    }

    // we are assuming that the ds3VacSolid is the main volume defining most ds3VacInfo position params

    ds3VacInfo.centerInParent = ds3Position-_hallOriginInMu2e;
    ds3VacInfo.centerInWorld  = ds3VacInfo.centerInParent + parent.centerInWorld;

    // the above should probably be done by finishNesting, well may be not for the boolean volumes

    finishNesting(ds3VacInfo,
                  vacuumMaterial,
                  0,
                  ds3VacInfo.centerInParent,
                  parent.logical,
                  0,
                  toyDSVisible,
                  G4Color::Blue(),
                  toyDSSolid,
                  forceAuxEdgeVisible,
                  placePV,
                  doSurfaceCheck
                  );

  } // end of Mu2eWorld::constructDS;

}
