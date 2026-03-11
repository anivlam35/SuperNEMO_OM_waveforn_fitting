#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include <TFile.h>
#include <TTree.h>

#include <snfee/snfee.h>
#include <snfee/io/multifile_data_reader.h>

#include <sncabling/om_id.h>
#include <sncabling/gg_cell_id.h>
#include <sncabling/label.h>

#include <snfee/data/raw_event_data.h>
#include <snfee/data/calo_digitized_hit.h>
#include <snfee/data/tracker_digitized_hit.h>

const int WAVEFORMS_NEEDED = 10;

int main (int argc, char *argv[])
{
  std::string input_filename = "";

  for (int iarg=1; iarg<argc; ++iarg)
    {
      std::string arg (argv[iarg]);
      if (arg[0] == '-')
	{
	  if (arg=="-i" || arg=="--input")
	    input_filename = std::string(argv[++iarg]);

	  else if (arg=="-h" || arg=="--help")
	    {
	      std::cout << std::endl;
	      std::cout << "Usage:   " << argv[0] << " [options]" << std::endl;
	      std::cout << std::endl;
	      std::cout << "Options:   -h / --help" << std::endl;
	      std::cout << "           -i / --input  RED_FILE" << std::endl;
	      std::cout << std::endl;
	      return 0;
	    }

	  else
	    std::cerr << "*** unkown option " << arg << std::endl;
	}
    }

  if (input_filename.empty())
    {
      std::cerr << "*** missing input filename !" << std::endl;
      return 1;
    }

  snfee::initialize();

  /// Configuration for raw data reader
  snfee::io::multifile_data_reader::config_type reader_cfg;
  reader_cfg.filenames.push_back(input_filename);

  // Instantiate a reader
  snfee::io::multifile_data_reader red_source (reader_cfg);

  // Working RED object
  snfee::data::raw_event_data red;
    
  // RED counter
  std::size_t red_counter = 0;

  // Waveforms counter
  std::size_t waveform_counter = 0;

  // Run number
  int32_t red_run_id;

  // Event number
  int32_t red_event_id;

  // OM id
  int16_t om_num;

  // Waveform
  std::vector<int16_t> waveform;

  // High and low thresholds
  int32_t ht, lt;

  // File initialization
  TFile *file = new TFile("waveforms.root", "RECREATE");
  TTree *tree = new TTree("t", "Calo waveforms");

  tree->Branch("run_id", &red_run_id);
  tree->Branch("event_id", &red_event_id);
  tree->Branch("om_num", &om_num);
  tree->Branch("high_threshold", &ht);
  tree->Branch("low_threshold", &lt);
  tree->Branch("waveform", &waveform);

  while (red_source.has_record_tag())
    {
      // Check the serialization tag of the next record:
      DT_THROW_IF(!red_source.record_tag_is(snfee::data::raw_event_data::SERIAL_TAG),
                  std::logic_error, "Unexpected record tag '" << red_source.get_record_tag() << "'!");

      // Load the next RED object:
      red_source.load(red);

      // Run number
      red_run_id   = red.get_run_id();

      // Event number
      red_event_id = red.get_event_id();

      // Reference time from trigger
      const snfee::data::timestamp & red_reference_time = red.get_reference_time();
      // snfee::data::timestamp is a generic class
      // for storing TDC timestamp (clock + TDC ticks) 
      // but red_reference_time is currently not set

      // Digitized calo hits
      const std::vector<snfee::data::calo_digitized_hit> red_calo_hits = red.get_calo_hits();

      // Print RED infos
      std::cout << "Event #" << red_event_id << " contains "
		<< red_calo_hits.size() << " calo hit(s) and "
		<< std::endl;

      // Scan calo hits
      for (const snfee::data::calo_digitized_hit & red_calo_hit : red_calo_hits)
		{
		// if (waveform_counter >= WAVEFORMS_NEEDED) break;
		// Origin of the hit in RTD file
		// const snfee::data::calo_digitized_hit::rtd_origin & origin = red_calo_hit.get_origin();
		// origin.get_trigger_id()
		// origin.get_hit_number()

		// OM ID from SNCabling
		sncabling::om_id om_id = red_calo_hit.get_om_id();
		int om_side, om_wall, om_column, om_row;
		if(om_id.is_main())
		{
			om_side   = om_id.get_side();
			om_column = om_id.get_column();
			om_row    = om_id.get_row();
			om_num = om_side*20*13 + om_column*13 + om_row;
		}
		else if(om_id.is_xwall())
		{
			om_side   = om_id.get_side();
			om_wall   = om_id.get_wall();
			om_column = om_id.get_column();
			om_row    = om_id.get_row();
			om_num = 520 + om_side*64 +  om_wall*32 + om_column*16 + om_row;
		}
		else if(om_id.is_gveto())
		{
			om_side = om_id.get_side();
			om_wall = om_id.get_wall();
			om_column = om_id.get_column();
			om_num = 520 + 128 + om_side*32 + om_wall*16 + om_column;
		}
 	   	ht = (int)red_calo_hit.is_high_threshold();
    		lt = (int)red_calo_hit.is_low_threshold_only();
		// om_id.is_main(), om_id.get_side(), etc. => see sncabling method's

		// Reference time (TDC)
		const snfee::data::timestamp & reference_time = red_calo_hit.get_reference_time();
		int64_t calo_tdc = reference_time.get_ticks(); // >>> 1 calo TDC tick = 6.25E-9 sec

		// Digitized waveform
		waveform = red_calo_hit.get_waveform();
		waveform_counter++;
		tree->Fill();
		std::cout << "Waveform #" << waveform_counter << " is read." << std::endl;

		// // High/Low threshold flags
		// red_calo_hit.is_high_threshold();
		// red_calo_hit.is_low_threshold_only();

		// // Wavecatcher firmware measurement
		// int16_t baseline       = red_calo_hit.get_fwmeas_baseline();
		// int16_t peak_amplitude = red_calo_hit.get_fwmeas_peak_amplitude();
		// int16_t peak_cell      = red_calo_hit.get_fwmeas_peak_cell();
		// int32_t charge         = red_calo_hit.get_fwmeas_charge();
		// int32_t rising_cell    = red_calo_hit.get_fwmeas_rising_cell();
		// int32_t falling_cell   = red_calo_hit.get_fwmeas_falling_cell()
		}


      // Increment the counter
      red_counter++;

    } // (while red_source.has_record_tag())

  file->Write();
  file->Close();
 
  std::cout << "Total RED object processed = " << red_counter << std::endl;

  snfee::terminate();

  return 0;
}

