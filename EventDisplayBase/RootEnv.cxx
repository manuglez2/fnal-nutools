////////////////////////////////////////////////////////////////////////
/// \file  RootEnv.cxx
/// \brief Configure the ROOT environment
///
/// \version $Id: RootEnv.cxx,v 1.2 2011-01-20 16:43:29 p-nusoftart Exp $
/// \author  messier@indiana.edu
////////////////////////////////////////////////////////////////////////
#include "EventDisplayBase/RootEnv.h"
#include <iostream>
#include <string>
#include <cassert>
#include <cstdlib>
#include "TROOT.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TApplication.h"
#include "TRootApplication.h"
#include "TGClient.h"
#include "TGX11.h"
#include "TRint.h"
#include "TSystem.h"
#include "TSysEvtHandler.h"
#include "TInterpreter.h"
using namespace evdb;

// The top level ROOT object
static TROOT root("TROOT","TROOT"); 

RootEnv::RootEnv(int /*argc*/, char** argv) 
{
//======================================================================
// Setup the root environment for a program started with command line
// options argc and argv
//======================================================================
  assert(gROOT);

  TApplication* app = gROOT->GetApplication();
  if (app == 0) {
    int    largc = 0;
    char** largv = 0;
    TRint* rapp = new TRint("TAPP",&largc, largv, 0, 0, kTRUE);
    
    //std::string p = gSystem->BaseName(argv[0]); p+= " [%d] ";
    //rapp->SetPrompt(p.c_str());
  }
  else {
    gROOT->SetBatch(kFALSE);
    if (gClient==0) {
      gSystem->Load("libGX11.so");
      gVirtualX = new TGX11("X11","X11 session");
      new TGClient(getenv("DISPLAY"));
    }
  }

  this->SignalConfig();
  this->InterpreterConfig();
  this->LoadIncludes();
  this->LoadClasses();
  
  gROOT->SetStyle("Plain");
  gStyle->SetOptStat(0);
}

//......................................................................

RootEnv::~RootEnv() 
{
  // ROOT takes care of the following delete so don't do it twice
  // if (fTheApp) { delete fTheApp; fTheApp = 0; }
}

//......................................................................

int RootEnv::Run() 
{
//======================================================================
// Turn control of the application over to ROOT's event loop
//======================================================================
  TApplication* app = gROOT->GetApplication();
  if (app) {
    app->Run(kFALSE); // kTRUE == "Return from run" request...
    return 1;
  }
  return 0;
}

//......................................................................

void RootEnv::InterpreterConfig()
{
//======================================================================
// Configure the root interpreter
//======================================================================
  if (gInterpreter) { // gInterpreter from TInterpreter.h
    gInterpreter->SaveContext();
    gInterpreter->SaveGlobalsContext();
  }
}

//......................................................................

void RootEnv::SignalConfig()
{
//======================================================================
// Configure root's signale handlers
//======================================================================
  return;
  if (gSystem) { // gSystem from TSystem.h
    // Reset ROOT's signal handling to the defaults...
    gSystem->ResetSignal(kSigBus,                  kTRUE);
    gSystem->ResetSignal(kSigSegmentationViolation,kTRUE);
    gSystem->ResetSignal(kSigSystem,               kTRUE);
    gSystem->ResetSignal(kSigPipe,                 kTRUE);
    gSystem->ResetSignal(kSigIllegalInstruction,   kTRUE);
    gSystem->ResetSignal(kSigQuit,                 kTRUE);
    gSystem->ResetSignal(kSigInterrupt,            kTRUE);
    gSystem->ResetSignal(kSigWindowChanged,        kTRUE);
    gSystem->ResetSignal(kSigAlarm,                kTRUE);
    gSystem->ResetSignal(kSigChild,                kTRUE);
    gSystem->ResetSignal(kSigUrgent,               kTRUE);
    gSystem->ResetSignal(kSigFloatingException,    kTRUE);
    gSystem->ResetSignal(kSigTermination,          kTRUE);
    gSystem->ResetSignal(kSigUser1,                kTRUE);
    gSystem->ResetSignal(kSigUser2,                kTRUE);
  }
}

//......................................................................

void RootEnv::LoadIncludes()
{
//======================================================================
// Load include files to make the root session more covenient
//======================================================================
  TApplication* app = gROOT->GetApplication();
  if (app) {
    // Load a set of useful C++ includes.
    // app->ProcessLine("#include <iostream>"); // Root gets this one itself
    app->ProcessLine("#include <iomanip>");
    app->ProcessLine("#include <string>");
    
    // Load experiment include files
    std::string mp = gROOT->GetMacroPath();
    std::string ip;
    const char* p;
    p = gSystem->Getenv("SRT_PRIVATE_CONTEXT");
    if (p) {
      mp += ":";
      mp += p;
      mp += ":";
      mp += p;
      mp += "/macros";
      ip += " -I";
      ip += p;
    }
    p = gSystem->Getenv("SRT_PUBLIC_CONTEXT");
    if (p) {
      mp += ":";
      mp += p;
      mp += "/macros";
      ip += " -I";
      ip += p;
    }
    
    gROOT->SetMacroPath(mp.c_str());
    gSystem->SetIncludePath(ip.c_str());
    
    std::string dip = ".include ";
    dip += gSystem->Getenv("SRT_PRIVATE_CONTEXT");
    gROOT->ProcessLine(dip.c_str());
    dip = ".include ";
    dip += gSystem->Getenv("SRT_PUBLIC_CONTEXT");
    gROOT->ProcessLine(dip.c_str());
  }
}

//......................................................................

void RootEnv::LoadClasses()
{
//======================================================================
// Load classes to make the root session more covenient
//======================================================================
  if (gROOT) {
    gROOT->LoadClass("TGeometry",   "Graf3d");
    gROOT->LoadClass("TTree",       "Tree");
    gROOT->LoadClass("TMatrix",     "Matrix");
    gROOT->LoadClass("TMinuit",     "Minuit");
    gROOT->LoadClass("TPostScript", "Postscript");
    gROOT->LoadClass("TCanvas",     "Gpad");
    gROOT->LoadClass("THtml",       "Html");
  }
}

////////////////////////////////////////////////////////////////////////
