// MiHeaders
#include "MiSDCaloHit.h"

ClassImp(MiSDCaloHit);

MiSDCaloHit::MiSDCaloHit()
{
}

MiSDCaloHit::~MiSDCaloHit()
{
}
	
double MiSDCaloHit::getE()
{
	return E;
}

MiGID* MiSDCaloHit::getGID()
{
	return &GID;
}

string  MiSDCaloHit::getname()
{
	return name;
}

string MiSDCaloHit::getcategory()
{
	return category;
}

int MiSDCaloHit::setE(double in_e)
{
	E = in_e;
	return 0;
}

int MiSDCaloHit::setGID(MiGID& in_GID)
{
	GID = in_GID;
	return 0;
}

int MiSDCaloHit::setname(string in_name)
{
	name = in_name;
	return 0;
}

int MiSDCaloHit::setcategory(string in_category)
{
	category = in_category;
	return 0;
}

