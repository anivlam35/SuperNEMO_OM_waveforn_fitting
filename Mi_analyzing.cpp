// Mi headers
#include "./MiModule/include/MiEvent.h" 

R__LOAD_LIBRARY(./MiModule/lib/libMiModule.so);

void analyze()
{
	TFile* f = new TFile("./0nu/sensitivity_1000.root");
	TTree* s = (TTree*) f->Get("Event");

	MiEvent* Eve = new MiEvent();
	s->SetBranchAddress("Eventdata", &Eve);
    
    int pas_1 = 0;
    int pas_2 = 0;
    int pas_3 = 0;
    int pas_4 = 0;
    
    int N = s->GetEntries();
    
    int n_calo;
    int n_calov;
    int n_rec = 0;
    int n_neg;
    double energy;

	for(UInt_t i=0; i < N; i++)	// Loop over events
	{
		s->GetEntry(i);
        
        n_calo = 0;
        n_calov = 0;
        n_neg = 0;
        energy = 0;
        
        n_rec = Eve->getPTD()->getpartv()->size();
        
        for(int j = 0; j < n_rec; j++)
        {
            n_calo = Eve->getPTD()->getpartv()->at(j).getcalohitv()->size();
            if(Eve->getPTD()->getpartv()->at(j).getcharge() == -1) n_neg++;
            for(int k = 0; k < n_calo; k++)
            {
                energy += Eve->getPTD()->getpartv()->at(j).getcalohitv()->at(k).getE();
            }
            n_calov += n_calo;
        }
                                        
        if (n_calov == 2) pas_1++;
        if (n_calov == 2 && n_rec == 2) pas_2++;
        if (n_calov == 2 && n_rec == 2 && n_neg == 2) pas_3++;
        if (n_calov == 2 && n_rec == 2 && n_neg == 2 && energy > 2000.0) pas_4++;
	}
    cout << endl << "EFFICIENCIES :" << endl;
	cout << "eps1 = " << (100.0 * pas_1) / N << "% +- " << (100.0 * sqrt(double(pas_1)) ) / N << "%" << endl;
	cout << "eps2 = " << (100.0 * pas_2) / N << "% +- " << (100.0 * sqrt(double(pas_2)) ) / N << "%" << endl;
    cout << "eps3 = " << (100.0 * pas_3) / N << "% +- " << (100.0 * sqrt(double(pas_3)) ) / N << "%" << endl;
    cout << "eps4 = " << (100.0 * pas_4) / N << "% +- " << (100.0 * sqrt(double(pas_4)) ) / N << "%" << endl;
}





