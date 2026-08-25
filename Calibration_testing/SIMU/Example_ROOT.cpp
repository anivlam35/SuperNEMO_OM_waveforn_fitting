// Mi headers
#include "./include/MiEvent.h" 

R__LOAD_LIBRARY(/sps/nemo/scratch/ykozina/Falaise/tutorial/MiModule/lib/libMiModule.so);

void Example_ROOT()
{
	TFile* f = new TFile("Default.root");
	TTree* s = (TTree*) f->Get("Event");

	MiEvent* Eve = new MiEvent();
	s->SetBranchAddress("Eventdata", &Eve);

	for(UInt_t i=0; i < s->GetEntries(); i++)	// Loop over events
	{
		s->GetEntry(i);
		Eve->print();
	}	
}






