// Copyright 2023-2099 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/** @author Christian Holm Christensen <cholm@nbi.dk> */

#include <Framework/AnalysisHelpers.h>
#include <Framework/AnalysisTask.h>
#include <Generators/AODToHepMC.h>
// #define HEPMC_PROCESS_AUX

#ifndef HEPMC_PROCESS_AUX
//--------------------------------------------------------------------
using o2::framework::ConfigParamKind;
using o2::framework::ConfigParamSpec;

// -------------------------------------------------------------------
void customize(std::vector<ConfigParamSpec>& workflowOptions)
{
  using o2::framework::VariantType;

  workflowOptions.emplace_back(ConfigParamSpec{"hepmc-aux",               //
                                               VariantType::Bool,         //
                                               false,                     //
                                               {"Also process auxiliary " //
                                                "HepMC tables"},
                                               ConfigParamKind::kProcessFlag});
}
#endif

//--------------------------------------------------------------------
// This _must_ be included after our "customize" function above, or
// that function will not be taken into account.
#include <Framework/runDataProcessing.h>

//--------------------------------------------------------------------
/** Task to convert AOD MC tables into HepMC event structure
 *
 *  This assumes that the following tables are available on the input:
 *
 *  - @c o2::aod::McCollisions
 *  - @c o2::aod::McParticles
 *  - @c o2::aod::HepMCXSections
 *  - @c o2::aod::HepMCPdfInfos
 *  - @c o2::aod::HepMCHeavyIons
 *
 *  The application @c o2-sim-mcevent-to-aod publishes these tables.
 *
 *  Ideally, processing auxiliary information should be optional, as
 *  in @c Task2 below.  However, that causes problems.  See @c Task2.
 */
struct Task1 {
  /** Alias the converter */
  using Converter = o2::eventgen::AODToHepMC;

  /** Our converter */
  Converter mConverter;

  /** @{
   * @name Container types */
  /** Alias converter header table type */
  using Headers = Converter::Headers;
  /** Alias converter header type */
  using Header = Converter::Header;
  /** Alias converter track table type */
  using Tracks = Converter::Tracks;
  /** Alias converter cross-section table type */
  using XSections = Converter::XSections;
  /** Alias converter cross-section type */
  using XSection = Converter::XSection;
  /** Alias converter parton distribution function table type */
  using PdfInfos = Converter::PdfInfos;
  /** Alias converter parton distribution function type */
  using PdfInfo = Converter::PdfInfo;
  /** Alias converter heavy-ions table type */
  using HeavyIons = Converter::HeavyIons;
  /** Alias converter heavy-ions type */
  using HeavyIon = Converter::HeavyIon;
  /** @} */

  /** Initialize the job */
  void init(o2::framework::InitContext& ic)
  {
    mConverter.init();
  }
  /** Default processing of an event
   *
   *  @param collision  Event header
   *  @param tracks     Tracks of the event
   */
  void process(Header const& collision,
               XSections const& xsections,
               PdfInfos const& pdfs,
#ifdef AODTOHEPMC_WITH_HEAVYION
               HeavyIons const& heavyions,
#endif
               Tracks const& tracks)
  {
    LOG(debug) << "=== Processing everything ===";
    mConverter.startEvent();
    mConverter.process(collision,
                       xsections,
                       pdfs
#ifdef AODTOHEPMC_WITH_HEAVYION
                       ,
                       heavyions
#endif
    );
    mConverter.process(collision, tracks);
    mConverter.endEvent();
  }
};

//--------------------------------------------------------------------
/**
 * Same as Task1 above, except only header and tracks are processed.
 *
 *  - @c o2::aod::McCollisions
 *  - @c o2::aod::McParticles
 *
 */
struct Task2 {
  /** Alias the converter type */
  using Converter = o2::eventgen::AODToHepMC;

  /** Our converter */
  Converter mConverter;

  /** @{
   * @name Container types */
  /** Alias converter header table type */
  using Headers = Converter::Headers;
  /** Alias converter header type */
  using Header = Converter::Header;
  /** Alias converter track table type */
  using Tracks = Converter::Tracks;
  /** Alias converter cross-section table type */
  using XSections = Converter::XSections;
  /** Alias converter cross-section type */
  using XSection = Converter::XSection;
  /** Alias converter parton distribution function table type */
  using PdfInfos = Converter::PdfInfos;
  /** Alias converter parton distribution function type */
  using PdfInfo = Converter::PdfInfo;
  /** Alias converter heavy-ions table type */
  using HeavyIons = Converter::HeavyIons;
  /** Alias converter heavy-ions type */
  using HeavyIon = Converter::HeavyIon;
  /** @} */

  /** Initialize the job */
  void init(o2::framework::InitContext& ic)
  {
    mConverter.init();
  }
  /** Default processing of an event
   *
   *  @param collision  Event header
   *  @param tracks     Tracks of the event
   */
  void process(Header const& collision,
               Tracks const& tracks)
  {
    LOG(debug) << "=== Processing only tracks ===";
    mConverter.startEvent();
    mConverter.process(collision, tracks);
    mConverter.endEvent();
  }
};

#ifdef HEPMC_PROCESS_AUX
//--------------------------------------------------------------------
/**
 *  Ideally, this application should work with the case where only
 *
 *  - @c o2::aod::McCollisions
 *  - @c o2::aod::McParticles
 *
 *  is available, through the use of @c
 *  o2::framework::ProcessConfigurable, but that seems to fail
 *  consistently.  The issue seems that the application @c
 *  o2-sim-mcevent-to-aod @c SIGSEGV since it stops publishing the
 *  tables when the main process of the client (this application) does
 *  not require those tables.
 *
 *  I tried various combinations of options for @c
 *  o2-sim-mcevent-to-aod but nothing seems to work.
 *
 *  The error is
 *
 *  @verbatim
 *  Exception caught: Unable to find OutputSpec with label HepMCXSections. Available Routes:
 *  - McCollisions: AOD/MCCOLLISION/0
 *  - McParticles: AOD/MCPARTICLE/1
 *  - : TFF/TFFilename/0
 *  - : TFN/TFNumber/0
 *  @endverbatim
 *
 * Or
 *
 * @verbatim
 * InputRecord::get: no input with binding HepMCHeavyIons found. Available inputs: McCollisions, McParticles
 * @endverbatim
 *
 *  Interstingly, the application @c o2-sim-mcevent-to-aod works fine
 *  on its own, e.g., like
 *
 *  @verbatim
 *  ./o2-sim-kine-publisher \
 *    --aggregate-timeframe 1 \
 *    --kineFileName pythia8pp |
 *  ./o2-sim-mcevent-to-aod \
 *    --aod-writer-keep dangling
 *  @endverbatim
 *
 *  works fine.
 *
 * Actually, it is not likely that this will ever work.  The various
 * process are done out of sync.  That is, first all input events of
 * the timeframe are passed to the regular `process` method - i.e.,
 * tracks and collision headers are processed.  Then all input events
 * of the timeframe are passed to the optional `processAux` method -
 * i.e., auxiliary tables and collision headers.
 *
 * That means that we cannot correlate the tracks and aux tables into
 * one event, which is what we need to format a proper HepMC
 * event. The reason why it could work in the above example is because
 * we only process one timeframe at a time.
 */
struct Task3 {
  /** Alias the converter type */
  using Converter = o2::eventgen::AODToHepMC;

  /** Our converter */
  Converter mConverter;

  /** @{
   * @name Container types */
  /** Alias converter header table type */
  using Headers = Converter::Headers;
  /** Alias converter header type */
  using Header = Converter::Header;
  /** Alias converter track table type */
  using Tracks = Converter::Tracks;
  /** Alias converter cross-section table type */
  using XSections = Converter::XSections;
  /** Alias converter cross-section type */
  using XSection = Converter::XSection;
  /** Alias converter parton distribution function table type */
  using PdfInfos = Converter::PdfInfos;
  /** Alias converter parton distribution function type */
  using PdfInfo = Converter::PdfInfo;
  /** Alias converter heavy-ions table type */
  using HeavyIons = Converter::HeavyIons;
  /** Alias converter heavy-ions type */
  using HeavyIon = Converter::HeavyIon;
  /** @} */

  /** Initialize the job */
  void init(o2::framework::InitContext& ic)
  {
    mConverter.init();
  }
  /** Process tracks of an event
   *
   *  @param collision  Event header
   *  @param tracks     Tracks of the event
   */
  void processTracks(Header const& collision,
                     Tracks const& tracks)
  {
    LOG(debug) << "=== Processing event tracks ===";
    mConverter.process(collision, tracks);
  }
  /** Optional processing of event to extract extra HepMC information
   *
   *  @param collision Event header
   *  @param xsections Cross-section information
   *  @param heavyions Heavy ion (geometry) information
   */
  void processAux(Header const& collision,
                  XSections const& xsections,
                  PdfInfos const& pdfs,
                  HeavyIons const& heavyions)
  {
    LOG(debug) << "=== Processing event auxiliaries ===";
    mConverter.process(collision,
                       xsections,
                       pdfs,
                       heavyions);
  }
  /** Default processing of an event
   *
   *  @param collision  Event header
   *  @param tracks     Tracks of the event
   */
  void process(Header const& collision,
               Tracks const& tracks)
  {
    LOG(debug) << "=== Processing only tracks ===";
    processTracks(collision, tracks);
  }
  /**
   * Make a process option.
   *
   * Instead of using the provided preprocessor macro, we instantise
   * the template directly here.  This is so that we can specify the
   * command line argument (@c --hepmc-aux) rather than to rely on an
   * auto-generated name (would be @ --processAux).
   */
  decltype(o2::framework::ProcessConfigurable{&Task3::processAux,
                                              "hepmc-aux", false,
                                              "Process auxilirary "
                                              "information"})
    doAux = o2::framework::ProcessConfigurable{&Task3::processAux,
                                               "hepmc-aux", false,
                                               "Process auxilirary "
                                               "information"};
};
#endif

//--------------------------------------------------------------------
using WorkflowSpec = o2::framework::WorkflowSpec;
using TaskName = o2::framework::TaskName;
using DataProcessorSpec = o2::framework::DataProcessorSpec;
using ConfigContext = o2::framework::ConfigContext;

/** Entry point of @a o2-sim-mcevent-to-hepmc */
WorkflowSpec defineDataProcessing(ConfigContext const& cfg)
{
  using o2::framework::adaptAnalysisTask;

  // Task1: One entry: header, tracks, auxiliary
  // Task2: One entry: header, tracks
  // Task3: Two entry: header, tracks, and auxiliary
#ifndef HEPMC_PROCESS_AUX
  if (cfg.options().get<bool>("hepmc-aux")) {
    LOG(info) << "Creating full o2-aod-to-hepmc processor";
    return WorkflowSpec{
      adaptAnalysisTask<Task1>(cfg, TaskName{"o2-aod-to-hepmc"})};
  }
  LOG(info) << "Creating minimal o2-aod-to-hepmc processor";
  return WorkflowSpec{
    adaptAnalysisTask<Task2>(cfg, TaskName{"o2-aod-to-hepmc"})};
#else
  return WorkflowSpec{
    adaptAnalysisTask<Task3>(cfg, TaskName{"o2-aod-mc-to-hepmc"})};
#endif
}
//
// EOF
//
