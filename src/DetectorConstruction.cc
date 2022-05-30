// Definition of world geometry and detectors.
// Created by Codey Olson on May 10, 2021.

/// \file DetectorConstruction.cc
/// \brief Definition of world geometry and detectors.

#include "DetectorConstruction.hh"

#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4SDManager.hh"

#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4Cons.hh"
#include "G4Orb.hh"
#include "G4Sphere.hh"
#include "G4VSolid.hh"
#include "G4VisAttributes.hh"
#include "G4RotationMatrix.hh"
#include "G4Transform3D.hh"
#include "G4Trd.hh"
#include "G4SubtractionSolid.hh"
#include "G4IntersectionSolid.hh"
#include "G4UnionSolid.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4VPrimitiveScorer.hh"
#include "G4MultiFunctionalDetector.hh"
#include "G4SDParticleFilter.hh"
#include "G4PSCellFlux.hh"
#include "G4PSEnergyDeposit.hh"

#include "G4SystemOfUnits.hh"
#include "G4VisAttributes.hh"
#define _USE_MATH_DEFINES 
#include <math.h>
#include <iomanip>
#include <iostream>
#include <string>


DetectorConstruction::DetectorConstruction()
: G4VUserDetectorConstruction()
{
  fmats = {};
  ConstructMaterials();
}

//
//

DetectorConstruction::~DetectorConstruction()
{}

//
//

void DetectorConstruction::ConstructMaterials()
{
  // Get instance of nist material manager:
  G4NistManager* nist = G4NistManager::Instance();

  // Create materials and input into dictionary.
  G4Material* air = nist->FindOrBuildMaterial("G4_AIR");
  fmats["air"] = air;
  G4Material* poly = nist->FindOrBuildMaterial("G4_POLYETHYLENE");
  fmats["poly"] = poly;

  G4Material* he3 = new G4Material("Helium 3", 5.39e-4*g/cm3, 1, kStateGas, 293.*kelvin, 4.*atmosphere); // From Walker Dissertai
  G4Element* helium = new G4Element("Helium", "He", 1);
  G4Isotope* helium3 = new G4Isotope("Helium3", 2, 3, 3.01602932197*g/mole); // from IAEA
  helium->AddIsotope(helium3, 100.*perCent);
  he3->AddElement(helium, 1);
  fmats["he3"] = he3;

  G4Material* steel = nist->FindOrBuildMaterial("G4_STAINLESS-STEEL");
  fmats["steel"] = steel;

  G4Material* aluminum = nist->FindOrBuildMaterial("G4_Al");
  fmats["aluminum"] = aluminum;

  G4Material* galactic = nist->FindOrBuildMaterial("G4_Galactic");
  fmats["galactic"] = galactic;
}

G4VPhysicalVolume* DetectorConstruction::Construct()
{
  G4bool checkOverlaps = true;

  //
  // World:
  // Params:
  G4double worldX, worldY, worldZ;
  worldX = 11.54*cm; 
  worldY = 9.54*cm; 
  worldZ = 11.*cm;
  // Construction:
  G4Box* solidWorld = new G4Box("World", 0.5*worldX, 0.5*worldY,0.5*worldZ);
  G4LogicalVolume* logicWorld = new G4LogicalVolume(solidWorld, fmats["galactic"], "World");
  G4VPhysicalVolume* physWorld = new G4PVPlacement(0, G4ThreeVector(), logicWorld, "World", 0, false, 0, checkOverlaps);

  G4double tubeDiam;
  G4double tubeHeight;
  G4double modx, mody, modz;

  // Tube and moderator dimensions:
  tubeDiam = 2.74*cm;
  tubeHeight = 10*cm;
  modx = 6.54*cm; mody = 4.54*cm; modz = tubeHeight;

  // Tube Construction
  G4Tubs* ssShellSolid = new G4Tubs("SS Shell", 0, 0.5*tubeDiam, 0.5*tubeHeight, 0, 360.*deg);
  G4LogicalVolume* ssShellLogic = new G4LogicalVolume(ssShellSolid, fmats["aluminum"], "SS Shell");
  new G4PVPlacement(0, G4ThreeVector(), ssShellLogic, "SS Shell", logicWorld, false, 0, checkOverlaps); 
  // helium3 fill gas:
  G4Tubs* he3GasSolid = new G4Tubs("He3 Gas", 0, 0.5*(tubeDiam - 2*mm), 0.5*(tubeHeight - 2*mm), 0, 360.*deg);
  G4LogicalVolume* he3GasLogic = new G4LogicalVolume(he3GasSolid, fmats["he3"], "He3 Gas");
  new G4PVPlacement(0, G4ThreeVector(), he3GasLogic, "He3 Tube", ssShellLogic, false, 0, checkOverlaps); 
  //Moderator:
  // Dummies for subtraction solid:
  G4Box* moderatorDummy = new G4Box("He3 Moderator Dummy", 0.5*modx, 0.5*mody, 0.5*modz);
  G4Tubs* moderatorVoidDummy = new G4Tubs("He3 Void Dummy", 0, 0.5*tubeDiam, 0.5*(tubeHeight + 1*cm), 0, 360.*deg);
  // Final solid:
  G4VSolid* he3ModeratorSolid = new G4SubtractionSolid("He3 Moderator", moderatorDummy, moderatorVoidDummy, 0, G4ThreeVector());
  G4LogicalVolume* he3ModeratorLogic = new G4LogicalVolume(he3ModeratorSolid, fmats["poly"], "He3 Moderator");
  new G4PVPlacement(0, G4ThreeVector(), he3ModeratorLogic, "He3 Moderator", logicWorld, false, 0, checkOverlaps);

  G4Box* airSourceDummy = new G4Box("AirSourceDummy", 5.27*cm, 4.27*cm, 5.*cm);
  G4VSolid* airSource = new G4SubtractionSolid("AirSource", airSourceDummy, moderatorDummy, 0, G4ThreeVector(0, 0, 0));
  G4LogicalVolume* airLogic = new G4LogicalVolume(airSource, fmats["air"], "AirSource");
  new G4PVPlacement(0, G4ThreeVector(0, 0, 0), airLogic, "AirSource", logicWorld, false, 0, checkOverlaps);
 
  return physWorld;
}

void DetectorConstruction::ConstructSDandField()
{
  G4SDParticleFilter* nFilter = new G4SDParticleFilter("NeutronFilter");
  nFilter->add("proton");
  nFilter->add("triton");
  nFilter->add("He3");
  nFilter->add("deuteron");
  nFilter->add("GenericIon");

  G4MultiFunctionalDetector* he3Detector = new G4MultiFunctionalDetector("Helium-3");
  G4SDManager::GetSDMpointer()->AddNewDetector(he3Detector);
  G4VPrimitiveScorer* energyDep = new G4PSEnergyDeposit("EnergyDep");
  energyDep->SetFilter(nFilter);
  he3Detector->RegisterPrimitive(energyDep);
  SetSensitiveDetector("He3 Gas", he3Detector);

}