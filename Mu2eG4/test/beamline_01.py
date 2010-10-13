#
# Configuration file for Beamline 01.
# This can be used to debug the muon beamline.
#
# $Id: beamline_01.py,v 1.6 2010/10/13 23:39:40 kutschke Exp $
# $Author: kutschke $
# $Date: 2010/10/13 23:39:40 $
#
# Original author Rob Kutschke
#
# Spacing is not signficant in this file.

# Define the default configuration for the framework.
import FWCore.ParameterSet.python.Config as mu2e

# Give this process a name.  
process = mu2e.Process("Beamline01")

# Maximum number of events to do.
process.maxEvents = mu2e.untracked.PSet(
    input = mu2e.untracked.int32(100)
)

# Load the standard message logger configuration.
# Threshold=Info. Limit of 5 per category; then exponential backoff.
process.load("MessageLogger_cfi")

# Load the service that manages root files for histograms.
process.TFileService = mu2e.Service("TFileService",
                       fileName = mu2e.string("beamline_01.root"),
                       closeFileFast = mu2e.untracked.bool(False)
)

# Define the random number generator service.
process.RandomNumberGeneratorService = mu2e.Service("RandomNumberGeneratorService")

# Define the geometry.
process.GeometryService = mu2e.Service("GeometryService",
       inputfile=mu2e.untracked.string("Mu2eG4/test/beamline_geom.txt")
)

# Access the conditions data.
process.ConditionsService = mu2e.Service("ConditionsService",
       conditionsfile=mu2e.untracked.string("Mu2eG4/test/conditions_01.txt")
)

# Define and configure some modules to do work on each event.
# Modules are just defined for now, the are scheduled later.

# Start each new event with an empty event.
process.source = mu2e.Source("EmptySource")

#  Make some generated tracks and add them to the event.
process.generate = mu2e.EDProducer(
    "EventGenerator",
    inputfile = mu2e.untracked.string("Mu2eG4/test/beamline_genconfig.txt"),
    seed=mu2e.untracked.vint32(7789)
)

#  Use this generator instead of the one above if using G4Beamline input files
#process.generate = mu2e.EDProducer(
#    "G4BeamlineGenerator",
#    inputfile = mu2e.untracked.string("Mu2eG4/test/beamline_genconfig.txt"),
#    seed=mu2e.untracked.vint32(7789)
#)


# Run G4 and add its hits to the event.
process.g4run = mu2e.EDProducer(
    "G4",
    generatorModuleLabel = mu2e.string("generate"),
    visMacro = mu2e.untracked.string("Mu2eG4/test/beamline_xz.mac"),
    seed=mu2e.untracked.vint32(9877)
    )

# Look at the hits from G4.
process.checkhits = mu2e.EDAnalyzer(
    "ReadBack",
    g4ModuleLabel = mu2e.string("g4run"),
    minimumEnergy = mu2e.double(0.00),
    maxFullPrint  = mu2e.untracked.int32(201)
)

# Look at the hits from virtual detectors
process.readvd = mu2e.EDAnalyzer(
    "ReadVirtualDetector",
    vdStepPoints = mu2e.untracked.string("virtualdetector"),
    savePDG = mu2e.untracked.vint32(13,-211)
)

# Save state of random numbers to the event.
process.randomsaver = mu2e.EDAnalyzer("RandomNumberSaver")

# Define the output file.
process.outfile = mu2e.OutputModule(
    "PoolOutputModule",
    fileName = mu2e.untracked.string('file:beamlineData_01.root'),
    outputCommands = cms.untracked.vstring(
     'keep *_*_*_*',
#     'drop mu2eSimParticles_*_*_*'   # Uncomment this line to reduce file size.
    ),

)

# End of the section that defines and configures modules.

# Tell the system to execute all paths.
process.output = mu2e.EndPath(  process.generate*
                                process.g4run*
                                process.readvd*
                                process.randomsaver*
                                process.outfile );
