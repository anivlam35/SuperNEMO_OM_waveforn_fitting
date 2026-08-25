#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_set>
#include <stdexcept>

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


int main(int argc, char *argv[])
{
    std::string input_filename = "";
    std::string event_list_filename = "";
    std::string output_filename = "waveforms.root";


    // ------------------------------------------------------------
    // Command line arguments
    // ------------------------------------------------------------

    for (int iarg = 1; iarg < argc; ++iarg)
    {
        std::string arg(argv[iarg]);

        if (arg == "-i" || arg == "--input")
        {
            if (iarg + 1 >= argc)
            {
                std::cerr << "*** missing argument for " << arg << std::endl;
                return 1;
            }

            input_filename = std::string(argv[++iarg]);
        }
        else if (arg == "-e" || arg == "--event-list")
        {
            if (iarg + 1 >= argc)
            {
                std::cerr << "*** missing argument for " << arg << std::endl;
                return 1;
            }

            event_list_filename = std::string(argv[++iarg]);
        }
        else if (arg == "-o" || arg == "--output")
        {
            if (iarg + 1 >= argc)
            {
                std::cerr << "*** missing argument for " << arg << std::endl;
                return 1;
            }

            output_filename = std::string(argv[++iarg]);
        }
        else if (arg == "-h" || arg == "--help")
        {
            std::cout << std::endl;
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  -h, --help              Show this help" << std::endl;
            std::cout << "  -i, --input RED_FILE    Input RED file" << std::endl;
            std::cout << "  -e, --event-list FILE   ROOT file with event IDs" << std::endl;
            std::cout << "  -o, --output FILE       Output ROOT file" << std::endl;
            std::cout << std::endl;
            return 0;
        }
        else
        {
            std::cerr << "*** unknown option " << arg << std::endl;
            return 1;
        }
    }


    // ------------------------------------------------------------
    // Check input
    // ------------------------------------------------------------

    if (input_filename.empty())
    {
        std::cerr << "*** missing input filename!" << std::endl;
        return 1;
    }


    // ------------------------------------------------------------
    // Initialize SNFee
    // ------------------------------------------------------------

    snfee::initialize();


    // ------------------------------------------------------------
    // Load event ID list, if provided
    // ------------------------------------------------------------

    std::unordered_set<int32_t> selected_events;

    if (!event_list_filename.empty())
    {
        TFile *event_list_file =
            TFile::Open(event_list_filename.c_str(), "READ");

        if (!event_list_file || event_list_file->IsZombie())
        {
            std::cerr << "*** cannot open event list file: "
                      << event_list_filename << std::endl;

            snfee::terminate();
            return 1;
        }

        TTree *event_list_tree = nullptr;

        event_list_file->GetObject("events", event_list_tree);

        if (!event_list_tree)
        {
            std::cerr << "*** cannot find TTree 'events' in "
                      << event_list_filename << std::endl;

            event_list_file->Close();
            delete event_list_file;

            snfee::terminate();
            return 1;
        }

        int32_t list_event_id;

        event_list_tree->SetBranchAddress(
            "event_id",
            &list_event_id
        );

        for (Long64_t i = 0;
             i < event_list_tree->GetEntries();
             ++i)
        {
            event_list_tree->GetEntry(i);
            selected_events.insert(list_event_id);
        }

        event_list_file->Close();
        delete event_list_file;

        std::cout << "Loaded "
                  << selected_events.size()
                  << " events from "
                  << event_list_filename
                  << std::endl;
    }
    else
    {
        std::cout << "No event list specified. "
                  << "Processing all events."
                  << std::endl;
    }


    // ------------------------------------------------------------
    // Configuration for raw data reader
    // ------------------------------------------------------------

    snfee::io::multifile_data_reader::config_type reader_cfg;

    reader_cfg.filenames.push_back(input_filename);

    // Instantiate reader
    snfee::io::multifile_data_reader red_source(reader_cfg);


    // ------------------------------------------------------------
    // Working RED object
    // ------------------------------------------------------------

    snfee::data::raw_event_data red;


    // ------------------------------------------------------------
    // Counters
    // ------------------------------------------------------------

    std::size_t red_counter = 0;
    std::size_t selected_event_counter = 0;
    std::size_t waveform_counter = 0;


    // ------------------------------------------------------------
    // RED data
    // ------------------------------------------------------------

    int32_t red_run_id;
    int32_t red_event_id;

    int16_t om_num;

    std::vector<int16_t> waveform;

    int32_t ht;
    int32_t lt;


    // ------------------------------------------------------------
    // Output ROOT file
    // ------------------------------------------------------------

    TFile *file = new TFile(
        output_filename.c_str(),
        "RECREATE"
    );

    if (!file || file->IsZombie())
    {
        std::cerr << "*** cannot create output file: "
                  << output_filename << std::endl;

        delete file;

        snfee::terminate();
        return 1;
    }

    TTree *tree = new TTree(
        "t",
        "Calo waveforms"
    );

    tree->Branch("run_id", &red_run_id);
    tree->Branch("event_id", &red_event_id);
    tree->Branch("om_num", &om_num);
    tree->Branch("high_threshold", &ht);
    tree->Branch("low_threshold", &lt);
    tree->Branch("waveform", &waveform);


    // ------------------------------------------------------------
    // Process RED records
    // ------------------------------------------------------------

    while (red_source.has_record_tag())
    {
        // Check serialization tag
        DT_THROW_IF(
            !red_source.record_tag_is(
                snfee::data::raw_event_data::SERIAL_TAG
            ),
            std::logic_error,
            "Unexpected record tag '"
                << red_source.get_record_tag()
                << "'!"
        );


        // Load next RED object
        red_source.load(red);


        // Run number
        red_run_id = red.get_run_id();


        // Event number
        red_event_id = red.get_event_id();


        // --------------------------------------------------------
        // Event filter
        // --------------------------------------------------------

        if (!event_list_filename.empty() &&
            selected_events.find(red_event_id) == selected_events.end())
        {
            red_counter++;
            continue;
        }

        selected_event_counter++;


        // --------------------------------------------------------
        // Digitized calorimeter hits
        // --------------------------------------------------------

        const std::vector<snfee::data::calo_digitized_hit>
            red_calo_hits = red.get_calo_hits();


        // Print RED information
        std::cout << "Event #" << red_event_id
                  << " contains "
                  << red_calo_hits.size()
                  << " calo hit(s)"
                  << std::endl;


        // --------------------------------------------------------
        // Scan calorimeter hits
        // --------------------------------------------------------

        for (const snfee::data::calo_digitized_hit &red_calo_hit :
             red_calo_hits)
        {
            // ----------------------------------------------------
            // OM ID
            // ----------------------------------------------------

            sncabling::om_id om_id =
                red_calo_hit.get_om_id();

            int om_side = 0;
            int om_wall = 0;
            int om_column = 0;
            int om_row = 0;


            if (om_id.is_main())
            {
                om_side = om_id.get_side();
                om_column = om_id.get_column();
                om_row = om_id.get_row();

                om_num =
                    om_side * 20 * 13
                    + om_column * 13
                    + om_row;
            }
            else if (om_id.is_xwall())
            {
                om_side = om_id.get_side();
                om_wall = om_id.get_wall();
                om_column = om_id.get_column();
                om_row = om_id.get_row();

                om_num =
                    520
                    + om_side * 64
                    + om_wall * 32
                    + om_column * 16
                    + om_row;
            }
            else if (om_id.is_gveto())
            {
                om_side = om_id.get_side();
                om_wall = om_id.get_wall();
                om_column = om_id.get_column();

                om_num =
                    520
                    + 128
                    + om_side * 32
                    + om_wall * 16
                    + om_column;
            }
            else
            {
                std::cerr
                    << "*** unknown OM type in event "
                    << red_event_id
                    << std::endl;

                continue;
            }


            // ----------------------------------------------------
            // Threshold flags
            // ----------------------------------------------------

            ht =
                static_cast<int32_t>(
                    red_calo_hit.is_high_threshold()
                );

            lt =
                static_cast<int32_t>(
                    red_calo_hit.is_low_threshold_only()
                );


            // ----------------------------------------------------
            // Reference time (TDC)
            // ----------------------------------------------------

            const snfee::data::timestamp &reference_time =
                red_calo_hit.get_reference_time();

            int64_t calo_tdc =
                reference_time.get_ticks();

            // 1 calo TDC tick = 6.25E-9 sec


            // ----------------------------------------------------
            // Waveform
            // ----------------------------------------------------

            waveform =
                red_calo_hit.get_waveform();

            waveform_counter++;

            tree->Fill();

            std::cout
                << "Waveform #"
                << waveform_counter
                << " is read."
                << std::endl;
        }


        // Increment RED counter
        red_counter++;

    } // while (red_source.has_record_tag())


    // ------------------------------------------------------------
    // Write output
    // ------------------------------------------------------------

    file->Write();
    file->Close();

    delete file;


    // ------------------------------------------------------------
    // Summary
    // ------------------------------------------------------------

    std::cout
        << "Total RED objects processed = "
        << red_counter
        << std::endl;

    std::cout
        << "Selected events processed = "
        << selected_event_counter
        << std::endl;

    std::cout
        << "Total waveforms written = "
        << waveform_counter
        << std::endl;


    // ------------------------------------------------------------
    // Terminate SNFee
    // ------------------------------------------------------------

    snfee::terminate();


    return 0;
}

