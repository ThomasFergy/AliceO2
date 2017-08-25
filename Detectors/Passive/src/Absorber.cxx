// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See https://alice-o2.web.cern.ch/ for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

// -------------------------------------------------------------------------
// ----- main responsible: Sandro Wenzel (sandro.wenzel@cern.ch)       -----
// -------------------------------------------------------------------------

#include <DetectorsBase/Detector.h>
#include <DetectorsPassive/Absorber.h>
#include <TGeoArb8.h> // for TGeoTrap
#include <TGeoCompositeShape.h>
#include <TGeoCone.h>
#include <TGeoManager.h>
#include <TGeoMatrix.h>
#include <TGeoPcon.h>
#include <TGeoPgon.h>
#include <TGeoTube.h>
#include <TGeoVolume.h>
#include <TVirtualMC.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

using namespace o2::passive;

Absorber::~Absorber() = default;

Absorber::Absorber() : FairModule("Absorber", "") {}
Absorber::Absorber(const char* name, const char* Title) : FairModule(name, Title) {}
Absorber::Absorber(const Absorber& rhs) = default;

Absorber& Absorber::operator=(const Absorber& rhs)
{
  // self assignment
  if (this == &rhs)
    return *this;

  // base class assignment
  FairModule::operator=(rhs);

  return *this;
}

namespace
{
// only here temporarily, I would like to harmonize Material treatment (outside of base detector)
int Material(Int_t imat, const char* name, Float_t a, Float_t z, Float_t dens, Float_t radl, Float_t absl,
             Float_t* buf = nullptr, Int_t nwbuf = 0)
{
  int kmat = -1;
  TVirtualMC::GetMC()->Material(kmat, name, a, z, dens, radl, absl, buf, nwbuf);
  return kmat;
}

int Mixture(Int_t imat, const char* name, Float_t* a, Float_t* z, Float_t dens, Int_t nlmat, Float_t* wmat = nullptr)
{
  // Check this!!!
  int kmat = -1;
  TVirtualMC::GetMC()->Mixture(kmat, name, a, z, dens, nlmat, wmat);
  return kmat;
}

int Medium(Int_t numed, const char* name, Int_t nmat, Int_t isvol, Int_t ifield, Float_t fieldm, Float_t tmaxfd,
           Float_t stemax, Float_t deemax, Float_t epsil, Float_t stmin, Float_t* ubuf = nullptr, Int_t nbuf = 0)
{
  // Check this!!!
  int kmed = -1;
  TVirtualMC::GetMC()->Medium(kmed, name, nmat, isvol, ifield, fieldm, tmaxfd, stemax, deemax, epsil, stmin, ubuf,
                              nbuf);
  return kmed;
}
}

void Absorber::createMaterials()
{
  // Define materials for muon absorber
  //
  Int_t isxfld = 2.;
  Float_t sxmgmx = 10.;
  o2::Base::Detector::initFieldTrackingParams(isxfld, sxmgmx);

  //
  // Air
  //
  Float_t aAir[4] = { 12.0107, 14.0067, 15.9994, 39.948 };
  Float_t zAir[4] = { 6., 7., 8., 18. };
  Float_t wAir[4] = { 0.000124, 0.755267, 0.231781, 0.012827 };
  Float_t dAir = 1.20479E-3;
  Float_t dAir1 = 1.20479E-10;
  //
  // Polyethylene
  //
  Float_t apoly[2] = { 12.01, 1. };
  Float_t zpoly[2] = { 6., 1. };
  Float_t wpoly[2] = { .33, .67 };
  //
  // Concrete
  //
  Float_t aconc[10] = { 1., 12.01, 15.994, 22.99, 24.305, 26.98, 28.086, 39.1, 40.08, 55.85 };
  Float_t zconc[10] = { 1., 6., 8., 11., 12., 13., 14., 19., 20., 26. };
  Float_t wconc[10] = { .01, .001, .529107, .016, .002, .033872, .337021, .013, .044, .014 };
  //
  // Steel
  //
  Float_t asteel[4] = { 55.847, 51.9961, 58.6934, 28.0855 };
  Float_t zsteel[4] = { 26., 24., 28., 14. };
  Float_t wsteel[4] = { .715, .18, .1, .005 };
  //
  // Ni-Cu-W alloy
  //
  Float_t aniwcu[3] = { 58.6934, 183.84, 63.546 };
  Float_t zniwcu[3] = { 28., 74., 29 };
  Float_t wniwcu[3] = { 0.015, 0.95, 0.035 };
  //
  // Poly Concrete
  //                      H     Li     F       C      Al     Si      Ca      Pb     O
  Float_t aPolyCc[9] = { 1., 6.941, 18.998, 12.01, 26.98, 28.086, 40.078, 207.2, 15.999 };
  Float_t zPolyCc[9] = { 1., 3., 9., 6., 13., 14., 20., 82., 8. };
  Float_t wPolyCc[9] = { 4.9, 1.2, 1.3, 1.1, 0.15, 0.02, 0.06, 0.7, 1.1 };
  Float_t wtot = 0;
  Int_t i = 0;

  for (i = 0; i < 9; i++) {
    wtot += wPolyCc[i];
  }
  for (i = 0; i < 9; i++) {
    wPolyCc[i] /= wtot;
  }

  //
  // Insulation powder
  //                    Si         O       Ti     Al
  Float_t ains[4] = { 28.0855, 15.9994, 47.867, 26.982 };
  Float_t zins[4] = { 14., 8., 22., 13. };
  Float_t wins[4] = { 0.3019, 0.4887, 0.1914, 0.018 };

  // ****************
  //     Defines tracking media parameters.
  //
  Float_t epsil, stmin, tmaxfd, deemax, stemax;
  epsil = .001;   // Tracking precision,
  stemax = -0.01; // Maximum displacement for multiple scat
  tmaxfd = -20.;  // Maximum angle due to field deflection
  deemax = -.3;   // Maximum fractional energy loss, DLS
  stmin = -.8;
  // ***************
  //

  //    Carbon Material and Medium
  //
  auto kC0 = Material(6, "CARBON0$", 12.01, 6., 1.75, 24.4, 49.9);
  auto kC1 = Material(26, "CARBON1$", 12.01, 6., 1.75, 24.4, 49.9);
  auto kC2 = Material(46, "CARBON2$", 12.01, 6., 1.75, 24.4, 49.9);
  Medium(6, "ABSO_C_C0", kC0, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);
  Medium(26, "ABSO_C_C1", kC1, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);
  Medium(46, "ABSO_C_C2", kC2, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);

  //
  //    Aluminum Material and Medium
  auto kAl0 = Material(9, "ALUMINIUM0$", 26.98, 13., 2.7, 8.9, 37.2);
  auto kAl1 = Material(29, "ALUMINIUM1$", 26.98, 13., 2.7, 8.9, 37.2);
  auto kAl2 = Material(49, "ALUMINIUM2$", 26.98, 13., 2.7, 8.9, 37.2);
  Medium(9, "ABSO_ALU_C0", kAl0, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);
  Medium(29, "ABSO_ALU_C1", kAl1, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);
  Medium(49, "ABSO_ALU_C2", kAl2, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);

  //
  //    Magnesium
  auto kMAG = Material(7, "MAGNESIUM$", 24.31, 12., 1.74, 25.3, 46.0);
  Medium(7, "ABSO_MG_C0", kMAG, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);

  //
  //    Iron
  auto kFe0 = Material(10, "IRON0$", 55.85, 26., 7.87, 1.76, 17.1);
  auto kFe1 = Material(30, "IRON1$", 55.85, 26., 7.87, 1.76, 17.1);
  auto kFe2 = Material(50, "IRON2$", 55.85, 26., 7.87, 1.76, 17.1);
  Medium(10, "ABSO_FE_C0", kFe0, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);
  Medium(30, "ABSO_FE_C1", kFe1, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);
  Medium(50, "ABSO_FE_C2", kFe2, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);

  //
  //    Copper
  auto kCu0 = Material(11, "COPPER0$", 63.55, 29., 8.96, 1.43, 15.1);
  auto kCu1 = Material(31, "COPPER1$", 63.55, 29., 8.96, 1.43, 15.1);
  auto kCu2 = Material(51, "COPPER2$", 63.55, 29., 8.96, 1.43, 15.1);
  Medium(11, "ABSO_Cu_C0", kCu0, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);
  Medium(31, "ABSO_Cu_C1", kCu1, 0, isxfld, sxmgmx, tmaxfd, -stemax, deemax, epsil, stmin);
  Medium(51, "ABSO_Cu_C2", kCu2, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);

  //
  //    Tungsten
  auto kW0 = Material(12, "TUNGSTEN0$ ", 183.85, 74., 19.3, .35, 10.3);
  auto kW1 = Material(32, "TUNGSTEN1$ ", 183.85, 74., 19.3, .35, 10.3);
  auto kW2 = Material(52, "TUNGSTEN2$ ", 183.85, 74., 19.3, .35, 10.3);
  Medium(12, "ABSO_W_C0", kW0, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);
  Medium(32, "ABSO_W_C1", kW1, 0, isxfld, sxmgmx, tmaxfd, -stemax, deemax, epsil, stmin);
  Medium(52, "ABSO_W_C2", kW2, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);

  //
  //     Ni-W-Cu
  auto k1 = Mixture(21, "Ni-W-Cu0$", aniwcu, zniwcu, 18.78, 3, wniwcu);
  auto k2 = Mixture(41, "Ni-W-Cu1$", aniwcu, zniwcu, 18.78, 3, wniwcu);
  auto k3 = Mixture(61, "Ni-W-Cu2$", aniwcu, zniwcu, 18.78, 3, wniwcu);
  Medium(21, "ABSO_Ni/W0", k1, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);
  Medium(41, "ABSO_Ni/W1", k2, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);
  Medium(61, "ABSO_Ni/W3", k3, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);

  //
  //    Lead
  auto kPb0 = Material(13, "LEAD0$", 207.19, 82., 11.35, .56, 18.5);
  auto kPb1 = Material(33, "LEAD1$", 207.19, 82., 11.35, .56, 18.5);
  auto kPb2 = Material(53, "LEAD2$", 207.19, 82., 11.35, .56, 18.5);
  Medium(13, "ABSO_PB_C0", kPb0, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);
  Medium(33, "ABSO_PB_C1", kPb1, 0, isxfld, sxmgmx, tmaxfd, -stemax, deemax, epsil, stmin);
  Medium(53, "ABSO_PB_C2", kPb2, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);

  //
  //    Insulation Powder
  auto kINS0 = Mixture(14, "INSULATION0$", ains, zins, 0.41, 4, wins);
  auto kINS1 = Mixture(34, "INSULATION1$", ains, zins, 0.41, 4, wins);
  auto kINS2 = Mixture(54, "INSULATION2$", ains, zins, 0.41, 4, wins);
  Medium(14, "ABSO_INS_C0", kINS0, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);
  Medium(34, "ABSO_INS_C1", kINS1, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);
  Medium(54, "ABSO_INS_C2", kINS2, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);

  //
  //    Air
  auto kAir0 = Mixture(15, "AIR0$", aAir, zAir, dAir, 4, wAir);
  auto kAir1 = Mixture(35, "AIR1$", aAir, zAir, dAir, 4, wAir);
  auto kAir2 = Mixture(55, "AIR2$", aAir, zAir, dAir, 4, wAir);
  Medium(15, "ABSO_AIR_C0", kAir0, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);
  Medium(35, "ABSO_AIR_C1", kAir1, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);
  Medium(55, "ABSO_AIR_C2", kAir2, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);

  //
  //    Vacuum
  auto kVA0 = Mixture(16, "VACUUM0$", aAir, zAir, dAir1, 4, wAir);
  auto kVA1 = Mixture(36, "VACUUM1$", aAir, zAir, dAir1, 4, wAir);
  auto kVA2 = Mixture(56, "VACUUM2$", aAir, zAir, dAir1, 4, wAir);
  Medium(16, "ABSO_VA_C0", kVA0, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);
  Medium(36, "ABSO_VA_C1", kVA1, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);
  Medium(56, "ABSO_VA_C2", kVA2, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);

  //
  //    Concrete
  auto kCC0 = Mixture(17, "CONCRETE0$", aconc, zconc, 2.35, 10, wconc);
  auto kCC1 = Mixture(37, "CONCRETE1$", aconc, zconc, 2.35, 10, wconc);
  auto kCC2 = Mixture(57, "CONCRETE2$", aconc, zconc, 2.35, 10, wconc);
  Medium(17, "ABSO_CC_C0", kCC0, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);
  Medium(37, "ABSO_CC_C1", kCC1, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);
  Medium(57, "ABSO_CC_C2", kCC2, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);

  //
  //    Polyethilene CH2
  auto kCH2_0 = Mixture(18, "POLYETHYLEN0$", apoly, zpoly, .95, -2, wpoly);
  auto kCH2_1 = Mixture(38, "POLYETHYLEN1$", apoly, zpoly, .95, 2, wpoly);
  auto kCH2_2 = Mixture(58, "POLYETHYLEN2$", apoly, zpoly, .95, 2, wpoly);
  Medium(18, "ABSO_CH2_C0", kCH2_0, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);
  Medium(38, "ABSO_CH2_C1", kCH2_1, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);
  Medium(58, "ABSO_CH2_C2", kCH2_2, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);

  //
  //    Steel
  auto kST0 = Mixture(19, "STAINLESS STEEL0$", asteel, zsteel, 7.88, 4, wsteel);
  auto kST1 = Mixture(39, "STAINLESS STEEL1$", asteel, zsteel, 7.88, 4, wsteel);
  auto kST2 = Mixture(59, "STAINLESS STEEL2$", asteel, zsteel, 7.88, 4, wsteel);
  Medium(19, "ABSO_ST_C0", kST0, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);
  Medium(39, "ABSO_ST_C1", kST1, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);
  Medium(59, "ABSO_ST_C3", kST2, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);

  //
  // Polymer Concrete
  auto kPolyC0 = Mixture(20, "Poly Concrete0$", aPolyCc, zPolyCc, 3.53, -9, wPolyCc);
  auto kPolyC1 = Mixture(40, "Poly Concrete1$", aPolyCc, zPolyCc, 3.53, 9, wPolyCc);
  auto kPolyC2 = Mixture(60, "Poly Concrete2$", aPolyCc, zPolyCc, 3.53, 9, wPolyCc);
  Medium(20, "ABSO_PCc_C0", kPolyC0, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);
  Medium(40, "ABSO_PCc_C1", kPolyC1, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);
  Medium(60, "ABSO_PCc_C3", kPolyC2, 0, isxfld, sxmgmx, tmaxfd, stemax, deemax, epsil, stmin);
}

TGeoPcon* MakeShapeFromTemplate(const TGeoPcon* pcon, Float_t drMin, Float_t drMax)
{
  //
  // Returns new shape based on a template changing
  // the inner radii by drMin and the outer radii by drMax.
  //
  Int_t nz = pcon->GetNz();
  TGeoPcon* cpcon = new TGeoPcon(0., 360., nz);
  for (Int_t i = 0; i < nz; i++) {
    cpcon->DefineSection(i, pcon->GetZ(i), pcon->GetRmin(i) + drMin, pcon->GetRmax(i) + drMax);
  }
  return cpcon;
}

void Absorber::ConstructGeometry()
{
  createMaterials();

  //
  // Build muon shield geometry
  //
  //

  Float_t z, z0, dz;
  //
  // The top volume
  //
  TGeoVolume* top = gGeoManager->GetVolume("cave");

  auto GetMedium = [](const char* x) {
    assert(gGeoManager);
    auto med = gGeoManager->GetMedium(x);
    assert(med);
    return med;
  };

  //
  // Media
  //
  TGeoMedium* kMedNiW = GetMedium("ABSO_Ni/W0");
  TGeoMedium* kMedNiWsh = GetMedium("ABSO_Ni/W3");
  //
  TGeoMedium* kMedSteel = GetMedium("ABSO_ST_C0");
  TGeoMedium* kMedSteelSh = GetMedium("ABSO_ST_C3");
  //
  TGeoMedium* kMedAir = GetMedium("ABSO_AIR_C0");
  //
  TGeoMedium* kMedPb = GetMedium("ABSO_PB_C0");
  TGeoMedium* kMedPbSh = GetMedium("ABSO_PB_C2");
  //
  TGeoMedium* kMedConcSh = GetMedium("ABSO_CC_C2");
  //
  TGeoMedium* kMedCH2Sh = GetMedium("ABSO_CH2_C2");
  //
  TGeoMedium* kMedC = GetMedium("ABSO_C_C0");
  TGeoMedium* kMedCsh = GetMedium("ABSO_C_C2");
  //
  TGeoMedium* kMedAlu = GetMedium("ABSO_ALU_C0");
  //
  TGeoMedium* kMedMg = GetMedium("ABSO_MG_C0");
  //
  const Float_t kDegRad = TMath::Pi() / 180.;

  //
  TGeoRotation* rotxz = new TGeoRotation("rotxz", 90., 0., 90., 90., 180., 0.);
  ///////////////////////////////////
  //                               //
  //        Front Absorber         //
  //        Drawing ALIP2A__0106   //
  //                               //
  //                               //
  ///////////////////////////////////
  //
  // Pos  1 Steel Envelope
  // Pos  2 End Plate
  // Pos  3 Flange (wrong arrow in the drawing)
  // Pos  4 W Plate A
  // Pos  5 W Plate B
  // Pos  6 Tungsten Tube Part 1
  // Pos  7 Tungsten Tube Part 2
  // Pos  8 Tungsten Tube Part 3
  // Pos  9 Tungsten Tube Part 4
  // Pos 10 Tungsten Tail
  // Pos 11 Graphite Cone
  // Pos 12 Pb       Cone
  // Pos 13 Concrete Cone
  // Pos 14 Polyethylene Parts
  // Pos 15 Steel Plate 25 cm
  // Pos 16 Steel Plate 31 cm
  // Pos 17 Magnesium Ring
  // Pos 18 Composite Ring
  //
  //
  // Mimimum angle of the tracking region
  const Float_t angle02 = TMath::Tan(2. * kDegRad);
  // Maximum angle of the tracking region
  const Float_t angle10 = TMath::Tan(10. * kDegRad);
  // Opening angle of W rear plug
  const Float_t angle03 = TMath::Tan(3. * kDegRad);
  //
  const Float_t angle05 = TMath::Tan(5. * kDegRad);
  // Opening angle of the FA snout
  const Float_t angle24 = TMath::Tan(24. * kDegRad);
  // Opneing angle of the inner cone
  const Float_t angle71 = TMath::Tan(0.697 * kDegRad);
  // Starting position in z
  const Float_t zFa = 90.0;

  // Pos 1
  ///////////////////////////////////
  //    FA Steel Envelope          //
  //    Drawing ALIP2A__0036       //
  ///////////////////////////////////
  // Thickness of the envelope
  Float_t dSteelEnvelope = 1.5;
  // Front cover
  //
  // Length
  Float_t dzSteelEnvelopeFC = 4.00;
  // Inner Radius
  Float_t rInSteelEnvelopeFC1 = 35.90 / 2.;
  Float_t rInSteelEnvelopeFC2 = rInSteelEnvelopeFC1 + dzSteelEnvelopeFC * angle10;
  // Outer Radius
  Float_t rOuSteelEnvelopeFC1 = 88.97 / 2.;
  Float_t rOuSteelEnvelopeFC2 = rOuSteelEnvelopeFC1 + dzSteelEnvelopeFC * angle05;
  //
  // 5 deg cone
  Float_t dzSteelEnvelopeC5 = 168.9;
  Float_t rInSteelEnvelopeC5 = rOuSteelEnvelopeFC2 - dSteelEnvelope / TMath::Cos(5 * kDegRad);
  Float_t rOuSteelEnvelopeC5 = rOuSteelEnvelopeFC2;
  // 10 deg cone
  Float_t dzSteelEnvelopeC10 = 227.1 - 4.;
  Float_t rInSteelEnvelopeC10 = 116.22 / 2.;
  Float_t rOuSteelEnvelopeC10 = rInSteelEnvelopeC10 + dSteelEnvelope / TMath::Cos(10 * kDegRad);
  // Rear ring
  Float_t dzSteelEnvelopeR = 4.;
  Float_t rInSteelEnvelopeR2 = 196.3 / 2.;
  Float_t rOuSteelEnvelopeR2 = 212.0 / 2.;
  Float_t rInSteelEnvelopeR1 = rInSteelEnvelopeR2 - dzSteelEnvelopeR * angle10;
  Float_t rOuSteelEnvelopeR1 = rInSteelEnvelopeR1 + dSteelEnvelope / TMath::Cos(10 * kDegRad);
  // Front insert
  Float_t dzSteelEnvelopeFI = 1.;
  Float_t rInSteelEnvelopeFI = 42.0 / 2.;
  Float_t rOuSteelEnvelopeFI = 85.0 / 2. + 0.06;

  TGeoPcon* shFaSteelEnvelopeC = new TGeoPcon(0., 360., 7);
  z = 0.;
  // Front cover
  shFaSteelEnvelopeC->DefineSection(0, z, rInSteelEnvelopeFC1, rOuSteelEnvelopeFC1);
  z += dzSteelEnvelopeFC;
  shFaSteelEnvelopeC->DefineSection(1, z, rInSteelEnvelopeFC2, rOuSteelEnvelopeFC2);
  // 5 deg cone
  shFaSteelEnvelopeC->DefineSection(2, z, rInSteelEnvelopeC5, rOuSteelEnvelopeC5);
  z += dzSteelEnvelopeC5;
  shFaSteelEnvelopeC->DefineSection(3, z, rInSteelEnvelopeC10, rOuSteelEnvelopeC10);
  // 10 deg cone
  z += dzSteelEnvelopeC10;
  shFaSteelEnvelopeC->DefineSection(4, z, rInSteelEnvelopeR1, rOuSteelEnvelopeR1);
  // Rear Ring
  shFaSteelEnvelopeC->DefineSection(5, z, rInSteelEnvelopeR1, rOuSteelEnvelopeR2);
  z += dzSteelEnvelopeR;
  shFaSteelEnvelopeC->DefineSection(6, z, rInSteelEnvelopeR2, rOuSteelEnvelopeR2);

  // Insert
  shFaSteelEnvelopeC->SetName("steelEnvC");
  TGeoTube* shFaSteelEnvelopeT = new TGeoTube(rInSteelEnvelopeFI, rOuSteelEnvelopeFI, dzSteelEnvelopeFI);
  shFaSteelEnvelopeT->SetName("steelEnvT");
  TGeoCompositeShape* shFaSteelEnvelope = new TGeoCompositeShape("shFaSteelEnvelope", "steelEnvC-steelEnvT");

  TGeoVolume* voFaSteelEnvelope = new TGeoVolume("AFaSteelEnvelope", shFaSteelEnvelope, kMedSteel);

  // Pos 2
  ///////////////////////////////////
  //    FA End Plate               //
  //    Drawing ALIP2A__0037       //
  ///////////////////////////////////
  //
  //
  //
  //    Outer dimensions dx, dy, dz
  Float_t dxEndPlate = 220.0;
  Float_t dyEndPlate = 220.0;
  Float_t dzEndPlate = 6.0;
  //    Inner radius
  Float_t rInEndPlate = 52.5 / 2.;
  //    Insert
  Float_t rInEndPlateI = 175.3 / 2.;
  Float_t rOuEndPlateI = 212.2 / 2.;
  Float_t dzEndPlateI = 2.0;

  TGeoBBox* endPlate1 = new TGeoBBox(dxEndPlate / 2., dyEndPlate / 2., dzEndPlate / 2.);
  endPlate1->SetName("endPlate1");

  TGeoTube* endPlate2 = new TGeoTube(0., rInEndPlate, (dzEndPlate + 0.1) / 2.);
  endPlate2->SetName("endPlate2");
  TGeoTube* endPlate3 = new TGeoTube(rInEndPlateI, rOuEndPlateI, (dzEndPlateI + 0.1) / 2.);
  endPlate3->SetName("endPlate3");

  TGeoTranslation* tPlate = new TGeoTranslation("tPlate", 0., 0., -dzEndPlateI - 0.05);
  tPlate->RegisterYourself();

  TGeoCompositeShape* shFaEndPlate = new TGeoCompositeShape("shFaEndPlate", "endPlate1-(endPlate2+endPlate3:tPlate)");
  TGeoVolume* voFaEndPlate = new TGeoVolume("AFaEndPlate", shFaEndPlate, kMedSteel);

  // Pos 3
  ///////////////////////////////////
  //    FA Flange                  //
  //    Drawing ALIP2A__0038       //
  ///////////////////////////////////
  // Width of the Flange
  Float_t dzFaFlange = 2.;
  // Outer radius
  Float_t rOuFaFlange = 41.0 / 2.;
  // 1st section
  Float_t dzFaFlange1 = 0.8;
  Float_t rInFaFlange1 = 33.4 / 2.;
  // 2nd section
  Float_t dzFaFlange2 = 1.2;
  Float_t rInFaFlange2 = 36.4 / 2.;

  TGeoPcon* shFaFlange = new TGeoPcon(0., 360., 4);
  z = 0;
  shFaFlange->DefineSection(0, z, rInFaFlange1, rOuFaFlange);
  z += dzFaFlange1;
  shFaFlange->DefineSection(1, z, rInFaFlange1, rOuFaFlange);
  shFaFlange->DefineSection(2, z, rInFaFlange2, rOuFaFlange);
  z += dzFaFlange2;
  shFaFlange->DefineSection(3, z, rInFaFlange2, rOuFaFlange);

  TGeoVolume* voFaFlange = new TGeoVolume("AFaFlange", shFaFlange, kMedSteel);

  // Pos 4+5
  ///////////////////////////////////
  //    FA W Plate A+B             //
  //    Drawing ALIP2A__0043       //
  ///////////////////////////////////
  // Front Flange
  Float_t dzFaWPlateF = 2.00;
  Float_t rInFaQPlateF = 20.50;
  Float_t rOuFaQPlateF = 40.05;
  // 1st Central Part 24 deg
  Float_t dzFaWPlateC1 = 7.95;
  Float_t rInFaQPlateC1 = 16.35;
  Float_t rOuFaQPlateC1 = rOuFaQPlateF + dzFaWPlateF * angle24;
  // 2nd Central Part 5 deg
  Float_t dzFaWPlateC2 = 1.05;
  Float_t rInFaQPlateC2 = rInFaQPlateC1 + dzFaWPlateC1 * angle10;
  Float_t rOuFaQPlateC2 = rOuFaQPlateC1 + dzFaWPlateC1 * angle24;
  Float_t rInFaQPlateC3 = 17.94;
  Float_t rOuFaQPlateC3 = 44.49;
  // Rear Flange
  Float_t dzFaWPlateR = 1.00;
  Float_t rInFaQPlateR = 21.00;
  Float_t rOuFaQPlateR = 42.55;
  // Lenth of Plate - Rear Flange
  Float_t dzFaWPlate = dzFaWPlateF + dzFaWPlateC1 + dzFaWPlateC2;

  TGeoPcon* shFaWPlateA = new TGeoPcon(0., 360., 7);
  z = 0.;
  // Front Flange
  shFaWPlateA->DefineSection(0, z, rInFaQPlateF, rOuFaQPlateF);
  z += dzFaWPlateF;
  shFaWPlateA->DefineSection(1, z, rInFaQPlateF, rOuFaQPlateC1);
  // 24 deg cone
  shFaWPlateA->DefineSection(2, z, rInFaQPlateC1, rOuFaQPlateC1);
  z += dzFaWPlateC1;
  shFaWPlateA->DefineSection(3, z, rInFaQPlateC2, rOuFaQPlateC2);
  // 5 deg cone
  z += dzFaWPlateC2;
  shFaWPlateA->DefineSection(4, z, rInFaQPlateC3, rOuFaQPlateC3);
  // Rear Flange
  shFaWPlateA->DefineSection(5, z, rInFaQPlateR, rOuFaQPlateR);
  z += dzFaWPlateR;
  shFaWPlateA->DefineSection(6, z, rInFaQPlateR, rOuFaQPlateR);

  TGeoVolume* voFaWPlateA = new TGeoVolume("AFaWPlateA", shFaWPlateA, kMedNiW);
  // Inner region with higher transport cuts
  TGeoPcon* shFaWPlateAI = new TGeoPcon(0., 360., 5);
  z = 3.;
  shFaWPlateAI->DefineSection(0, z, rInFaQPlateF + z * angle10, rOuFaQPlateC1 + (z - dzFaWPlateF) * angle24);
  for (Int_t i = 1; i < 5; i++) {
    Float_t rmin = shFaWPlateA->GetRmin(i + 2);
    Float_t rmax = shFaWPlateA->GetRmax(i + 2) - 3.;
    Float_t zpos = shFaWPlateA->GetZ(i + 2);
    shFaWPlateAI->DefineSection(i, zpos, rmin, rmax);
  }
  TGeoVolume* voFaWPlateAI = new TGeoVolume("AFaWPlateAI", shFaWPlateAI, kMedNiWsh);
  voFaWPlateA->AddNode(voFaWPlateAI, 1, gGeoIdentity);

  //
  // Inner Tungsten Shield
  // Part 1  99.8 cm
  // Part 2 143.5 cm
  // Part 3  25.0 cm
  // Part 4  31.0 cm
  // ====================
  //        299.3 cm - 0.6 overlap between Part 1 and Part 2
  //        298.7 cm
  // Starting position 499.0 - 298.7 = 200.3
  // Within C cone:    200.3 -  92.0 = 108.3 = end of straight section of the Graphite Cone
  //

  // Pos 6
  ///////////////////////////////////
  //    FA Tungsten Tube Part 1    //
  //    Drawing ALIP2A__0045       //
  ///////////////////////////////////
  //
  // Inner radius
  Float_t rInFaWTube1C1 = 9.1 / 2.;
  // Central part
  Float_t dzFaWTube1C = 98.8;
  Float_t rOuFaWTube1C1 = 13.8 / 2.;
  Float_t rOuFaWTube1C2 = 20.7 / 2.;
  // Rear Flange
  Float_t dzFaWTube1R = 1.0;
  Float_t rOuFaWTube1R = 15.0 / 2.;
  // Total length
  Float_t dzFaWTube1 = dzFaWTube1C + dzFaWTube1R;

  TGeoPcon* shFaWTube1 = new TGeoPcon(0., 360., 4);
  z = 0.;
  // Central Part
  shFaWTube1->DefineSection(0, z, rInFaWTube1C1, rOuFaWTube1C1);
  z += dzFaWTube1C;
  shFaWTube1->DefineSection(1, z, rInFaWTube1C1, rOuFaWTube1C2);
  // Rear Flange
  shFaWTube1->DefineSection(2, z, rInFaWTube1C1, rOuFaWTube1R);
  z += dzFaWTube1R;
  shFaWTube1->DefineSection(3, z, rInFaWTube1C1, rOuFaWTube1R);

  TGeoVolume* voFaWTube1 = new TGeoVolume("AFaWTube1", shFaWTube1, kMedNiWsh);

  // Pos 7
  ///////////////////////////////////
  //    FA Tungsten Tube Part 2    //
  //    Drawing ALIP2A__0046       //
  ///////////////////////////////////
  //

  // Central part
  Float_t dzFaWTube2C = 142.9;
  Float_t rInFaWTube2C1 = 9.10 / 2.;
  Float_t rInFaWTube2C2 = 12.58 / 2.;
  Float_t rOuFaWTube2C1 = 20.70 / 2.;
  Float_t rOuFaWTube2C2 = 30.72 / 2. - 0.05;
  // Front Flange
  Float_t dzFaWTube2F = 0.6;
  Float_t rInFaWTube2F = 15.4 / 2.;
  // Total length
  Float_t dzFaWTube2 = dzFaWTube2C + dzFaWTube2F;

  TGeoPcon* shFaWTube2 = new TGeoPcon(0., 360., 4);
  z = 0.;
  // Front Flange
  shFaWTube2->DefineSection(0, z, rInFaWTube2F, rOuFaWTube2C1);
  z += dzFaWTube2F;
  shFaWTube2->DefineSection(1, z, rInFaWTube2F, rOuFaWTube2C1);
  // Central part
  shFaWTube2->DefineSection(2, z, rInFaWTube2C1, rOuFaWTube2C1);
  z += dzFaWTube2C;
  shFaWTube2->DefineSection(3, z, rInFaWTube2C2, rOuFaWTube2C2);

  TGeoVolume* voFaWTube2 = new TGeoVolume("AFaWTube2", shFaWTube2, kMedNiWsh);

  // Pos 8
  ///////////////////////////////////
  //    FA Tungsten Tube Part 3    //
  //    Drawing ALIP2A__0047       //
  ///////////////////////////////////
  Float_t dzFaWTube3 = 25.0;
  Float_t rInFaWTube3C1 = 12.59 / 2.;
  Float_t rInFaWTube3C2 = 13.23 / 2.;
  Float_t rOuFaWTube3C1 = 30.60 / 2.;
  Float_t rOuFaWTube3C2 = 32.35 / 2.;
  TGeoVolume* voFaWTube3 = new TGeoVolume(
    "AFaWTube3", new TGeoCone(dzFaWTube3 / 2., rInFaWTube3C1, rOuFaWTube3C1, rInFaWTube3C2, rOuFaWTube3C2), kMedNiWsh);

  // Pos 9
  ///////////////////////////////////
  //    FA Tungsten Tube Part 4    //
  //    Drawing ALIP2A__0048       //
  ///////////////////////////////////
  Float_t dzFaWTube4 = 31.0;
  Float_t rInFaWTube4C1 = 13.23 / 2.;
  Float_t rInFaWTube4C2 = 13.98 / 2.;
  Float_t rOuFaWTube4C1 = 48.80 / 2.;
  Float_t rOuFaWTube4C2 = 52.05 / 2.;
  TGeoVolume* voFaWTube4 = new TGeoVolume(
    "AFaWTube4", new TGeoCone(dzFaWTube4 / 2., rInFaWTube4C1, rOuFaWTube4C1, rInFaWTube4C2, rOuFaWTube4C2), kMedNiWsh);

  // Pos 10
  //
  // This section has been moved to AliSHILv3

  //
  // Pos 11
  ///////////////////////////////////
  //    FA Graphite Cone           //
  //    Drawing ALIP2_0002         //
  ///////////////////////////////////
  //
  // Total length
  Float_t dzFaGraphiteCone = 225.0;
  // Straight section = start of the 2deg inner cone
  Float_t dzFaGraphiteConeS = 108.3;
  // Inner radius at the front
  Float_t rInFaGraphiteCone1 = 4.5;
  // Outer radius at the front
  Float_t rOuFaGraphiteCone1 = (zFa + dzFaFlange) * angle10;
  // Inner radius at start of inner opening cone
  Float_t rInFaGraphiteCone2 = 7.0;
  // Outer radius at start of inner opening cone
  Float_t rOuFaGraphiteCone2 = (zFa + dzFaFlange + dzFaGraphiteConeS) * angle10;
  // Inner radius the rear
  Float_t rInFaGraphiteCone3 = 11.0;
  // Ouer radius at the rear
  Float_t rOuFaGraphiteCone3 = (zFa + dzFaFlange + dzFaGraphiteCone) * angle10;

  TGeoPcon* shFaGraphiteCone = new TGeoPcon(0., 360., 4);

  z = 0;
  // Straight section
  shFaGraphiteCone->DefineSection(0, z, rInFaGraphiteCone1, rOuFaGraphiteCone1);
  z += dzFaGraphiteConeS;
  shFaGraphiteCone->DefineSection(1, z, rInFaGraphiteCone1, rOuFaGraphiteCone2);
  // 2 deg opening cone
  shFaGraphiteCone->DefineSection(2, z, rInFaGraphiteCone2, rOuFaGraphiteCone2);
  z = dzFaGraphiteCone;
  shFaGraphiteCone->DefineSection(3, z, rInFaGraphiteCone3, rOuFaGraphiteCone3);

  TGeoVolume* voFaGraphiteCone = new TGeoVolume("AFaGraphiteCone", shFaGraphiteCone, kMedCsh);
  //
  // Outer region with lower transport cuts
  dz = 50.;
  TGeoCone* shFaGraphiteConeO = new TGeoCone(dz / 2., rInFaGraphiteCone1, rOuFaGraphiteCone1, rInFaGraphiteCone1,
                                             rOuFaGraphiteCone1 + dz * angle10);

  TGeoVolume* voFaGraphiteConeO = new TGeoVolume("AFaGraphiteConeO", shFaGraphiteConeO, kMedC);
  voFaGraphiteCone->AddNode(voFaGraphiteConeO, 1, new TGeoTranslation(0., 0., dz / 2.));

  // Pos 12
  ///////////////////////////////////
  //    FA Lead Cone               //
  //    Drawing ALIP2A__0077       //
  ///////////////////////////////////
  // 5 deg cone
  Float_t dzFaPbCone5 = 168.9;
  Float_t rInFaPbCone5 = 37.35 / 2.;
  Float_t rOuFaPbCone5 = 85.66 / 2.;
  // 10 deg cone
  Float_t dzFaPbCone10 = 25.9;
  Float_t rInFaPbCone10 = rInFaPbCone5 + dzFaPbCone5 * angle10;
  Float_t rOuFaPbCone10 = 115.2 / 2.;
  // end
  Float_t rInFaPbConeE = 106.05 / 2.;
  Float_t rOuFaPbConeE = 124.35 / 2.;
  // Total length
  Float_t dzFaPbCone = dzFaPbCone5 + dzFaPbCone10;

  TGeoPcon* shFaPbCone = new TGeoPcon(0., 360., 3);
  z = 0.;
  // 5 deg cone
  shFaPbCone->DefineSection(0, z, rInFaPbCone5, rOuFaPbCone5);
  z += dzFaPbCone5;
  // 10 deg cone
  shFaPbCone->DefineSection(1, z, rInFaPbCone10, rOuFaPbCone10);
  z += dzFaPbCone10;
  shFaPbCone->DefineSection(2, z, rInFaPbConeE, rOuFaPbConeE);

  TGeoVolume* voFaPbCone = new TGeoVolume("AFaPbCone", shFaPbCone, kMedPb);
  //
  // Inner region with higher transport cuts
  TGeoPcon* shFaPbConeI = MakeShapeFromTemplate(shFaPbCone, 0., -3.);
  TGeoVolume* voFaPbConeI = new TGeoVolume("AFaPbConeI", shFaPbConeI, kMedPbSh);
  voFaPbCone->AddNode(voFaPbConeI, 1, gGeoIdentity);

  // Pos 13
  ///////////////////////////////////
  //    FA Concrete Cone           //
  //    Drawing ALIP2A__00xx       //
  ///////////////////////////////////
  Float_t dzFaConcreteCone = 126.;
  Float_t rOuFaConcreteCone1 = rOuFaGraphiteCone3;
  Float_t rInFaConcreteCone1 = 11.;
  Float_t rOuFaConcreteCone2 = rOuFaConcreteCone1 + dzFaConcreteCone * angle10;
  Float_t rInFaConcreteCone2 = rInFaConcreteCone1 + dzFaConcreteCone * angle02;

  TGeoVolume* voFaConcreteCone = new TGeoVolume(
    "AFaConcreteCone",
    new TGeoCone(dzFaConcreteCone / 2., rInFaConcreteCone1, rOuFaConcreteCone1, rInFaConcreteCone2, rOuFaConcreteCone2),
    kMedConcSh);

  // Pos 14
  ///////////////////////////////////
  //    FA Polyethylene Parts      //
  //    Drawing ALIP2A__0034       //
  ///////////////////////////////////
  Float_t dzFaCH2Cone = 201.;
  Float_t rInFaCH2Cone1 = 106.0 / 2.;
  Float_t rInFaCH2Cone2 = 176.9 / 2.;
  Float_t dFaCH2Cone = 7.5 / TMath::Cos(10. * kDegRad);

  TGeoVolume* voFaCH2Cone =
    new TGeoVolume("AFaCH2Cone", new TGeoCone(dzFaCH2Cone / 2., rInFaCH2Cone1, rInFaCH2Cone1 + dFaCH2Cone,
                                              rInFaCH2Cone2, rInFaCH2Cone2 + dFaCH2Cone),
                   kMedCH2Sh);

  // Pos 15
  ///////////////////////////////////
  //    FA Steel Plate 250 mm      //
  //    Drawing ALIP2A__00xx       //
  ///////////////////////////////////
  Float_t dzFaSteelCone25 = 25.;
  Float_t eps = 0.001;
  Float_t rInFaSteelCone25A = rInFaConcreteCone2;
  Float_t rOuFaSteelCone25A = rOuFaConcreteCone2;
  Float_t rInFaSteelCone25B = rInFaSteelCone25A + dzFaSteelCone25 * angle02;
  Float_t rOuFaSteelCone25B = rOuFaSteelCone25A + dzFaSteelCone25 * angle10;

  TGeoVolume* voFaSteelCone25 = new TGeoVolume(
    "AFaSteelCone25", new TGeoCone(dzFaSteelCone25 / 2., rInFaSteelCone25A + eps, rOuFaSteelCone25A - eps,
                                   rInFaSteelCone25B + eps, rOuFaSteelCone25B - eps),
    kMedSteelSh);

  // Pos 16
  ///////////////////////////////////
  //    FA Steel Plate 310 mm      //
  //    Drawing ALIP2A__00xx       //
  ///////////////////////////////////
  Float_t dzFaSteelCone31 = 31.;
  Float_t rInFaSteelCone31A = rOuFaWTube4C1;
  ;
  Float_t rOuFaSteelCone31A = rOuFaSteelCone25B;
  Float_t rInFaSteelCone31B = rOuFaWTube4C2;
  Float_t rOuFaSteelCone31B = rOuFaSteelCone31A + dzFaSteelCone31 * angle10;

  TGeoVolume* voFaSteelCone31 = new TGeoVolume(
    "AFaSteelCone31", new TGeoCone(dzFaSteelCone31 / 2., rInFaSteelCone31A + eps, rOuFaSteelCone31A - eps,
                                   rInFaSteelCone31B + eps, rOuFaSteelCone31B - eps),
    kMedSteelSh);
  // Outer Region with higher transport cuts
  dz = 5.;
  TGeoVolume* voFaSteelCone31I =
    new TGeoVolume("AFaSteelCone31I",
                   new TGeoCone(dz / 2., rInFaSteelCone31B - dz * angle03 + eps, rOuFaSteelCone31B - dz * angle10 - eps,
                                rInFaSteelCone31B + eps, rOuFaSteelCone31B - eps),
                   kMedSteel);

  voFaSteelCone31->AddNode(voFaSteelCone31I, 1, new TGeoTranslation(0., 0., dzFaSteelCone31 / 2. - dz / 2.));

  ///////////////////////////////////
  //    FA Composite Ring          //
  //    Drawing ALIP2A__0126       //
  ///////////////////////////////////
  // 1st section
  Float_t dzFaCompRing1 = 0.8;
  Float_t rInFaCompRing1 = 11.0 / 2.;
  Float_t rOuFaCompRing1 = 32.4 / 2.;
  // 2nd section
  Float_t dzFaCompRing2 = 1.2;
  Float_t rInFaCompRing2 = 14.0 / 2.;
  Float_t rOuFaCompRing2 = 35.3 / 2.;

  TGeoPcon* shFaCompRing = new TGeoPcon(0., 360., 4);
  z = 0.;
  // 1st section
  shFaCompRing->DefineSection(0, z, rInFaCompRing1, rOuFaCompRing1);
  z += dzFaCompRing1;
  shFaCompRing->DefineSection(1, z, rInFaCompRing1, rOuFaCompRing1);
  // 2nd section
  shFaCompRing->DefineSection(2, z, rInFaCompRing2, rOuFaCompRing2);
  ;
  z += dzFaCompRing2;
  shFaCompRing->DefineSection(3, z, rInFaCompRing2, rOuFaCompRing2);

  TGeoVolume* voFaCompRing = new TGeoVolume("AFaCompRing", shFaCompRing, kMedC);

  ///////////////////////////////////
  //    FA Magnesium Ring          //
  //    Drawing ALIP2A__0127       //
  ///////////////////////////////////
  //
  // The inner radii
  // section 1+3
  Float_t dzFaMgRingO = 0.7;
  Float_t rInFaMgRingO = 3.0;
  // section 2
  Float_t dzFaMgRingI = 0.6;
  Float_t rInFaMgRingI = 3.5;

  TGeoPcon* shFaMgRing = new TGeoPcon(0., 360., 8);
  // 1st section
  z = 0.;
  shFaMgRing->DefineSection(0, z, rInFaMgRingO, rInFaCompRing1);
  z += dzFaMgRingO;
  shFaMgRing->DefineSection(1, z, rInFaMgRingO, rInFaCompRing1);
  // 2nd section
  shFaMgRing->DefineSection(2, z, rInFaMgRingI, rInFaCompRing1);
  z += dzFaMgRingI / 2.;
  shFaMgRing->DefineSection(3, z, rInFaMgRingI, rInFaCompRing1);
  // 3rd section
  shFaMgRing->DefineSection(4, z, rInFaMgRingI, rInFaCompRing2);
  z += dzFaMgRingI / 2.;
  shFaMgRing->DefineSection(5, z, rInFaMgRingI, rInFaCompRing2);
  // 4th section
  shFaMgRing->DefineSection(6, z, rInFaMgRingO, rInFaCompRing2);
  z += dzFaMgRingO;
  shFaMgRing->DefineSection(7, z, rInFaMgRingO, rInFaCompRing2);
  TGeoVolume* voFaMgRing = new TGeoVolume("AFaMgRing", shFaMgRing, kMedMg);

  //
  //    Absorber mother volume
  //
  //
  // Length of the absorber without endplate
  Float_t dzFa = dzFaFlange + dzFaGraphiteCone + dzFaConcreteCone + dzFaSteelCone25 + dzFaSteelCone31;
  TGeoPcon* shFaM = new TGeoPcon(0., 360., 16);
  // Front -> Flange (Mg Ring details)
  z = 0.;
  shFaM->DefineSection(0, z, rInFaMgRingO, rOuFaQPlateF);
  z += dzFaMgRingO;
  dz = dzFaMgRingO;
  shFaM->DefineSection(1, z, rInFaMgRingO, rOuFaQPlateF + dz * angle24);
  shFaM->DefineSection(2, z, rInFaMgRingI, rOuFaQPlateF + dz * angle24);
  z += dzFaMgRingI;
  dz += dzFaMgRingI;
  shFaM->DefineSection(3, z, rInFaMgRingI, rOuFaQPlateF + dz * angle24);
  shFaM->DefineSection(4, z, rInFaMgRingO, rOuFaQPlateF + dz * angle24);
  z += dzFaMgRingO;
  dz += dzFaMgRingO;
  shFaM->DefineSection(5, z, rInFaMgRingO, rOuFaQPlateF + dz * angle24);
  shFaM->DefineSection(6, z, rInFaGraphiteCone1, rOuFaQPlateF + dz * angle24);
  // Flange -> W-Plate B
  z += dzFaWPlateC1;
  shFaM->DefineSection(7, z, rInFaGraphiteCone1, rOuFaQPlateC2);
  z += dzFaWPlateC2;
  Float_t zFaSteelEnvelope = z;
  shFaM->DefineSection(8, z, rInFaGraphiteCone1, rOuFaQPlateC3);
  // 5 deg cone -> 10 deg cone
  z = zFaSteelEnvelope + dzSteelEnvelopeFC + dzSteelEnvelopeC5;
  shFaM->DefineSection(9, z, rInFaGraphiteCone1, rOuSteelEnvelopeC10);
  // 10 deg cone  up to end of straight section
  z0 = z;
  z = dzFaFlange + dzFaGraphiteConeS + dzFaWTube1C;
  dz = z - z0;
  shFaM->DefineSection(10, z, rInFaGraphiteCone1, rOuSteelEnvelopeC10 + dz * angle10);
  // 0.7 deg inner opening cone up to outer rear ring
  z0 = z;
  z = dzFa - dzSteelEnvelopeR / 2.;
  dz = (z - z0);
  shFaM->DefineSection(11, z, rInFaGraphiteCone1 + dz * angle71, rOuSteelEnvelopeR1);
  shFaM->DefineSection(12, z, rInFaGraphiteCone1 + dz * angle71, rOuSteelEnvelopeR2);
  z += dzSteelEnvelopeR / 2.;
  shFaM->DefineSection(13, z, rInFaWTube4C2, rOuSteelEnvelopeR2);
  // Recess for end plate
  dz = dzSteelEnvelopeR / 2;
  shFaM->DefineSection(14, z, rInFaCH2Cone2 - dz * angle10, rOuSteelEnvelopeR2);
  z += dzSteelEnvelopeR / 2.;
  shFaM->DefineSection(15, z, rInFaCH2Cone2, rOuSteelEnvelopeR2);

  TGeoVolume* voFaM = new TGeoVolume("AFaM", shFaM, kMedAir);
  voFaM->SetVisibility(0);

  //
  //    Assemble volumes inside acceptance
  TGeoPcon* shFaAccM = new TGeoPcon(0., 360., 7);
  for (Int_t i = 0; i < 4; i++) {
    Float_t zpos = shFaGraphiteCone->GetZ(i);
    Float_t rmin = shFaGraphiteCone->GetRmin(i);
    Float_t rmax = shFaGraphiteCone->GetRmax(i);
    shFaAccM->DefineSection(i, zpos, rmin, rmax);
  }
  z = dzFaGraphiteCone + dzFaConcreteCone + dzFaSteelCone25;
  z0 = z + zFa + dzFaFlange;
  shFaAccM->DefineSection(4, z, rOuFaWTube3C2, z0 * angle10);
  shFaAccM->DefineSection(5, z, rOuFaWTube4C1, z0 * angle10);
  z += dzFaSteelCone31;
  z0 += dzFaSteelCone31;
  shFaAccM->DefineSection(6, z, rOuFaWTube4C2, z0 * angle10);
  TGeoVolume* voFaAccM = new TGeoVolume("AFaAcc", shFaAccM, kMedAir);

  z = 0;
  voFaAccM->AddNode(voFaGraphiteCone, 1, gGeoIdentity);
  z += dzFaGraphiteCone;
  voFaAccM->AddNode(voFaConcreteCone, 1, new TGeoTranslation(0., 0., z + dzFaConcreteCone / 2.));
  z += dzFaConcreteCone;
  voFaAccM->AddNode(voFaSteelCone25, 1, new TGeoTranslation(0., 0., z + dzFaSteelCone25 / 2.));
  z += dzFaSteelCone25;
  voFaAccM->AddNode(voFaSteelCone31, 1, new TGeoTranslation(0., 0., z + dzFaSteelCone31 / 2.));

  //
  // Inner shield
  TGeoVolumeAssembly* voFaInnerShield = new TGeoVolumeAssembly("AFaInnerShield");
  voFaInnerShield->AddNode(voFaWTube1, 1, gGeoIdentity);
  z = dzFaWTube1 - 0.6;
  voFaInnerShield->AddNode(voFaWTube2, 1, new TGeoTranslation(0., 0., z));
  z += dzFaWTube2;
  voFaInnerShield->AddNode(voFaWTube3, 1, new TGeoTranslation(0., 0., z + dzFaWTube3 / 2.));
  z += dzFaWTube3;
  voFaInnerShield->AddNode(voFaWTube4, 1, new TGeoTranslation(0., 0., z + dzFaWTube4 / 2.));
  z = dzFaGraphiteConeS + dzFaFlange;
  voFaM->AddNode(voFaInnerShield, 1, new TGeoTranslation(0., 0., z));

  //
  //    Adding volumes to mother volume
  //
  z = 0.;
  voFaM->AddNode(voFaWPlateA, 1, gGeoIdentity);
  z += dzFaWPlate;
  voFaM->AddNode(voFaSteelEnvelope, 1, new TGeoTranslation(0., 0., z));
  z += dzSteelEnvelopeFC;
  voFaM->AddNode(voFaPbCone, 1, new TGeoTranslation(0., 0., z));
  z += (dzFaPbCone + dzFaCH2Cone / 2.);
  voFaM->AddNode(voFaCH2Cone, 1, new TGeoTranslation(0., 0., z));
  voFaM->AddNode(voFaFlange, 1, gGeoIdentity);
  voFaM->AddNode(voFaMgRing, 1, gGeoIdentity);
  voFaM->AddNode(voFaCompRing, 1, gGeoIdentity);
  voFaM->AddNode(voFaAccM, 1, new TGeoTranslation(0., 0., dzFaFlange));

  ////////////////////////////////////////////////////
  //                                                //
  //    Front Absorber Support Structure FASS       //
  //                                                //
  //    Drawing ALIP2A__0035                        //
  //    Drawing ALIP2A__0089                        //
  //    Drawing ALIP2A__0090                        //
  //    Drawing ALIP2A__0109                        //
  ////////////////////////////////////////////////////
  TGeoVolumeAssembly* voFass = new TGeoVolumeAssembly("AFass");
  const Float_t kFassUBFlangeH = 380.;
  const Float_t kFassUBFlangeW = 77.;

  const Float_t kFassUMFlangeH = 380.;
  const Float_t kFassUMFlangeB = 246.;
  const Float_t kFassUMFlangeT = 10.;
  const Float_t kFassUMFalpha = -TMath::ATan((kFassUMFlangeB - kFassUMFlangeT) / kFassUMFlangeH / 2.) / kDegRad;
  // Upper back   flange
  // B1
  // 380 x 77
  TGeoVolume* voFassUBFlange =
    new TGeoVolume("AFassUBFlange", new TGeoBBox(kFassUBFlangeW / 2., kFassUBFlangeH / 2., 3. / 2.), kMedSteel);
  voFass->AddNode(voFassUBFlange, 1,
                  new TGeoTranslation(+1.5 + kFassUBFlangeW / 2., 180. + kFassUBFlangeH / 2., kFassUMFlangeB - 1.5));
  voFass->AddNode(voFassUBFlange, 2,
                  new TGeoTranslation(-1.5 - kFassUBFlangeW / 2., 180. + kFassUBFlangeH / 2., kFassUMFlangeB - 1.5));

  // Lower back   flange
  // Upper median flange
  //    Drawing ALIP2A__0090                        //
  //    Drawing ALIP2A__0089                        //
  //    A2

  TGeoVolume* voFassUMFlange = new TGeoVolume(
    "AFassUMFlange", new TGeoTrap(kFassUMFlangeH / 2., kFassUMFalpha, 0., 1.5, kFassUMFlangeB / 2., kFassUMFlangeB / 2.,
                                  0., 1.5, kFassUMFlangeT / 2., kFassUMFlangeT / 2., 0.),
    kMedSteel);

  TGeoRotation* rotFass1 = new TGeoRotation("rotFass1", 180., 0., 90., 0., 90., 90.);
  voFass->AddNode(voFassUMFlange, 1,
                  new TGeoCombiTrans(0., 180. + kFassUMFlangeH / 2.,
                                     -(kFassUMFlangeB + kFassUMFlangeT) / 4. + kFassUMFlangeB, rotFass1));

  // Lower median flange
  //    Drawing ALIP2A__0090                        //
  //    Drawing ALIP2A__0089                        //
  //    A1
  const Float_t kFassLMFlangeH = 242.;
  const Float_t kFassLMFlangeB = 246.;
  const Float_t kFassLMFlangeT = 43.;
  const Float_t kFassLMFalpha = -TMath::ATan((kFassLMFlangeB - kFassLMFlangeT) / kFassLMFlangeH / 2.) / kDegRad;
  TGeoVolume* voFassLMFlange = new TGeoVolume(
    "AFassLMFlange", new TGeoTrap(kFassLMFlangeH / 2., kFassLMFalpha, 0., 1.5, kFassLMFlangeB / 2., kFassLMFlangeB / 2.,
                                  0., 1.5, kFassLMFlangeT / 2., kFassLMFlangeT / 2., 0.),
    kMedSteel);
  TGeoRotation* rotFass2 = new TGeoRotation("rotFass2", 180., 0., 90., 0., 90., 270.);
  voFass->AddNode(voFassLMFlange, 1,
                  new TGeoCombiTrans(0., -180. - kFassLMFlangeH / 2.,
                                     -(kFassLMFlangeB + kFassLMFlangeT) / 4. + kFassLMFlangeB, rotFass2));

  // Stiffeners
  // Support Plate
  //
  // Central cone
  TGeoPgon* shFassCone = new TGeoPgon(22.5, 360., 8, 4);
  shFassCone->DefineSection(0, 0., 0., 180.);
  shFassCone->DefineSection(1, 3., 0., 180.);
  shFassCone->DefineSection(2, 3., 177., 180.);
  shFassCone->DefineSection(3, 246., 177., 180.);
  shFassCone->SetName("FassCone");

  TGeoBBox* shFassWindow = new TGeoBBox(190., 53., 28.);
  shFassWindow->SetName("FassWindow");
  TGeoTranslation* tFassWindow = new TGeoTranslation("tFassWindow", 0., 0., 78.);
  tFassWindow->RegisterYourself();

  TGeoTube* shFassApperture = new TGeoTube(0., 104., 3.);
  shFassApperture->SetName("FassApperture");

  TGeoCompositeShape* shFassCentral =
    new TGeoCompositeShape("shFassCentral", "FassCone-(FassWindow:tFassWindow+FassApperture)");

  TGeoVolume* voFassCentral = new TGeoVolume("AFassCentral", shFassCentral, kMedSteel);
  voFass->AddNode(voFassCentral, 1, gGeoIdentity);

  //
  // Aluminum ring
  //
  TGeoVolume* voFassAlRing = new TGeoVolume("AFassAlRing", new TGeoTube(104., 180., 10.), kMedAlu);

  //
  // Assemble the FA
  //
  // Inside muon spectrometer acceptance
  //
  //    Composite  2 cm
  //    Graphite 225 cm
  //    Concrete 126 cm
  //    Steel     56 cm
  // ===================
  //             409 cm
  // should be   409 cm

  //
  // Absorber and Support
  TGeoVolumeAssembly* voFA = new TGeoVolumeAssembly("AFA");
  voFA->AddNode(voFaM, 1, gGeoIdentity);
  voFA->AddNode(voFaEndPlate, 1, new TGeoTranslation(0., 0., dzFa + dzEndPlate / 2.));
  voFA->AddNode(voFass, 1, new TGeoTranslation(0., 0., 388.45));
  voFA->AddNode(voFassAlRing, 1, new TGeoTranslation(0., 0., 382. - 3.56));
  top->AddNode(voFA, 1, new TGeoCombiTrans(0., 0., -90., rotxz));
}

FairModule* Absorber::CloneModule() const { return new Absorber(*this); }
ClassImp(o2::passive::Absorber);
