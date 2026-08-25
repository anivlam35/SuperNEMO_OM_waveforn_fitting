#ifndef MISDCALOHIT_HH
#define MISDCALOHIT_HH

// MiHeaders
#include "MiGID.h"

// ROOT headers
#include "TObject.h"

using namespace std;

class MiSDCaloHit: public TObject
{
	public:
		//! Constructor
		MiSDCaloHit();

		//! Destructor
		~MiSDCaloHit();

		double  getE();
		MiGID*  getGID();
		string  getname();				// Added 13.11.2020
		string  getcategory();			// Added for SD calohit category

		int  setE(double in_e);
		int  setGID(MiGID& in_GID);
		int  setname(string in_name);			// Added 13.11.2020
		int  setcategory(string in_category);	// Added for SD calohit category
		
	private:

		double 		E;			// Particle energy / energy deposit
		MiGID 		GID;			// Hit GID
		string     	name;		// Particle name, Added 13.11.2020
		string     	category;	// SD step-hit category: calo/xcalo/gveto/bi207_calib_source

	ClassDef(MiSDCaloHit,2);		
};

#endif // MISDCALOHIT_HH
