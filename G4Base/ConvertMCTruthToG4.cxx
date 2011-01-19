////////////////////////////////////////////////////////////////////////
/// \file  ConvertMCTruthToG4.cxx
/// \brief Convert MCTruth to G4Event; Geant4 event generator
///
/// \version $Id: ConvertMCTruthToG4.cxx,v 1.2 2011-01-19 16:45:41 p-nusoftart Exp $
/// \author  seligman@nevis.columbia.edu, brebel@fnal.gov
////////////////////////////////////////////////////////////////////////

#include "G4Base/ConvertMCTruthToG4.h"
#include "G4Base/PrimaryParticleInformation.h"
#include "SimulationBase/simbase.h"

#include <G4Event.hh>
#include <G4PrimaryVertex.hh>
#include <G4ParticleDefinition.hh>

#include <TParticle.h>

#include <CLHEP/Vector/LorentzVector.h>

#include <iostream>
#include <vector>
#include <map>

const G4bool debug = false;

using namespace g4b;

// Static variables used by class.

// Geant4's particle table.
G4ParticleTable* ConvertMCTruthToG4::fParticleTable = 0;

//-----------------------------------------------------
// Constructor and destructor.
ConvertMCTruthToG4::ConvertMCTruthToG4() 
{
}

//-----------------------------------------------------
ConvertMCTruthToG4::~ConvertMCTruthToG4() 
{
  // Print out a list of "unknown" PDG codes we saw in the input.
  if ( ! fUnknownPDG.empty() ){
      std::cerr << __FILE__ << ", line " << __LINE__ << ":" << std::endl;
      std::cerr << "   The following unknown PDG codes were present in the simb::MCTruth input. They were not simulated." << std::endl;
      for ( std::map<G4int, G4int>::iterator i = fUnknownPDG.begin(); i != fUnknownPDG.end(); ++i ){
	std::cerr << "   Unknown PDG code = " << (*i).first
		    << ", # times=" << (*i).second
		  << std::endl;
      }
  }

}

//-----------------------------------------------------
void ConvertMCTruthToG4::Reset()
{
  fConvertList.clear();
}

//-----------------------------------------------------
void ConvertMCTruthToG4::Append( art::Ptr<simb::MCTruth>& mct )
{
  fConvertList.push_back( mct );
}

//-----------------------------------------------------
void ConvertMCTruthToG4::GeneratePrimaries( G4Event* event )
{
  // Take the contents of MCTruth objects and use them to
  // initialize the G4Event.
  
  // A G4Event organizes its particles in terms of "vertices" and
  // "particles", like HepMC.  Unfortunately, ROOT doesn't use
  // HepMC, so the MCTruth objects aren't organized that way.
  // For most of the work that we'll ever do, there'll be only one
  // vertex in the event.  However, just in case there are multiple
  // vertices (e.g., overlays, double vertex studies) I want the
  // code to function properly.
  
  // So create a map of particle positions and associated
  // G4PrimaryVertex*.  Note that the map must use CLHEP's
  // LorentzVector, and not ROOT's, since ROOT does not define an
  // operator< for its physics vectors.
  std::map< CLHEP::HepLorentzVector, G4PrimaryVertex* >                  vertexMap;
  std::map< CLHEP::HepLorentzVector, G4PrimaryVertex* >::const_iterator  vi; 
  // For each MCTruth (probably only one, but you never know):
  for( art::PtrVector<simb::MCTruth>::const_iterator mci = fConvertList.begin(); mci != fConvertList.end(); ++mci ){
    
    art::Ptr<simb::MCTruth> mct(*mci);

    // For each simb::MCParticle in the MCTruth:
    for ( int p = 0; p != mct->NParticles(); ++p ){
      // Implementation note: the following statement copies the
      // TParticle from the MCTruth, instead of getting a
      // const reference.  The reason why we have to do it this
      // way is that the TParticle::GetPolarisation() method is
      // not const, so if we're going to get the polarization we
      // have to get it from a non-const copy.  
      simb::MCParticle particle = mct->GetParticle(p);

      // status code == 1 means "track this particle."  Any
      // other status code should be ignored by the Monte Carlo.
      if ( particle.StatusCode() != 1 ) continue;
      
      // Get the Particle Data Group code for the particle.
      G4int pdgCode = particle.PdgCode();
      
      // Get the vertex.  Note that LArSoft/ROOT uses cm, but
      // Geant4/CLHEP uses mm.
      G4double x = particle.Vx() * cm;
      G4double y = particle.Vy() * cm;
      G4double z = particle.Vz() * cm;
      G4double t = particle.T()  * ns;
      
      // Create a CLHEP four-vector from the particle's vertex.
      CLHEP::HepLorentzVector fourpos(x,y,z,t);
      
      // Is this vertex already in our map?
      G4PrimaryVertex* vertex = 0;
      std::map< CLHEP::HepLorentzVector, G4PrimaryVertex* >::const_iterator result = vertexMap.find( fourpos );
      if ( result == vertexMap.end() ){
	// No, it's not, so create a new vertex and add it to the
	// map.
	vertex = new G4PrimaryVertex(x, y, z, t);
	vertexMap[ fourpos ] = vertex;

	// Add the vertex to the G4Event.
	event->AddPrimaryVertex( vertex );
      }
      else{
	// Yes, it is, so use the existing vertex.
	vertex = (*result).second;
      }
      
      // Get additional particle information.
      TLorentzVector momentum = particle.Momentum(); // (px,py,pz,E)
      TVector3 polarization = particle.Polarization();
      
      // Get the particle table if necessary.  (Note: we're
      // doing this "late" because I'm not sure at what point
      // the G4 particle table is initialized in the LArG4
      // vs. Geant4 loading process.
      if ( fParticleTable == 0 ){
	fParticleTable = G4ParticleTable::GetParticleTable();
      }


      // Get Geant4's definition of the particle.
      G4ParticleDefinition* particleDefinition = fParticleTable->FindParticle(pdgCode);
      
      // What if the PDG code is unknown?  This has been a known
      // issue with GENIE.
      if ( particleDefinition == 0 ){
	fUnknownPDG[ pdgCode ] += 1;
	continue;
      }
      
      // Create a Geant4 particle to add to the vertex.
      G4PrimaryParticle* g4particle = new G4PrimaryParticle( particleDefinition,
							     momentum.Px() * GeV,
							     momentum.Py() * GeV,
							     momentum.Pz() * GeV);

      // Add more particle information the Geant4 particle.
      G4double charge = particleDefinition->GetPDGCharge();
      g4particle->SetCharge( charge );
      g4particle->SetPolarization( polarization.x(),
				   polarization.y(),
				   polarization.z() );
      
      // Add the particle to the vertex.
      vertex->SetPrimary( g4particle );

      // Create a PrimaryParticleInformation object, and save
      // the MCTruth pointer in it.  This will allow the
      // ParticleActionList class to access MCTruth
      // information during Geant4's tracking.
      PrimaryParticleInformation* primaryParticleInfo = new PrimaryParticleInformation;
      primaryParticleInfo->SetMCTruth( mct );
	  
      // Save the PrimaryParticleInformation in the
      // G4PrimaryParticle for access during tracking.
      g4particle->SetUserInformation( primaryParticleInfo );

    } // for each particle in MCTruth
  } // for each MCTruth
}///GeneratePrimaries
