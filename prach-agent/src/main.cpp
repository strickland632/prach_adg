//main.cpp
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <complex>
#include <sys/time.h>
#include <unistd.h>
#include <yaml-cpp/yaml.h>


#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/types/tune_request.hpp>
#include <uhd/types/metadata.hpp>


extern "C" {
#include "srsran/srsran.h"
}


#include "args.h"
#include "source.h"


#define MAX_LEN 70176






// // UHD TX: send PRACH CP+sequence -> replace w source
// static void tx_send_prach(const uhd::usrp::multi_usrp::sptr& usrp,
//                          const cf_t* buf,
//                          size_t nsamps_total,
//                          double tx_rate,
//                          double center_freq_hz,
//                          double tx_gain_db,
//                          double seconds_in_future = 0.050 /* 50 ms */)
// {
//  // Configure USRP (rate/freq/gain)
//  usrp->set_tx_rate(tx_rate);
//  usrp->set_tx_gain(tx_gain_db);
//  usrp->set_tx_freq(uhd::tune_request_t(center_freq_hz));


//  // TX streamer: fc32 (host) -> sc16 (wire)
//  uhd::stream_args_t sargs("fc32", "sc16");
//  auto tx = usrp->get_tx_stream(sargs);
//  const size_t mtu = tx->get_max_num_samps();


//  // Schedule start a bit in the future for timing
//  const auto now     = usrp->get_time_now();
//  const auto tx_time = now + uhd::time_spec_t(seconds_in_future);


//  uhd::tx_metadata_t md{};
//  md.has_time_spec  = true;
//  md.time_spec      = tx_time;
//  md.start_of_burst = true;
//  md.end_of_burst   = false;


//  // Chunked send
//  size_t offset = 0;
//  while (offset < nsamps_total) {
//    const size_t to_send = std::min(mtu, nsamps_total - offset);
//    const void*  chunk   = static_cast<const void*>(buf + offset);
//    const size_t sent    = tx->send(chunk, to_send, md);


//    // Keep this safety check; if it trips, you probably want to know.
//    if (sent != to_send) {
//      throw std::runtime_error("Short send on PRACH burst");
//    }


//    md.start_of_burst = false;  // only first packet has SoB + time
//    md.has_time_spec  = false;
//    offset += sent;
//  }


//  // End of burst
//  md.end_of_burst = true;
//  int* end[1] = {nullptr};
//  tx->send(end, 0, md);
// }


#include <fstream>
#include <complex>

//do this in source file intead?
void save_iq(std::ofstream& out, const cf_t* buf, std::size_t nsamps)
{
    if (!out) {
        throw std::runtime_error("IQ output stream is not open");
    }

    out.write(reinterpret_cast<const char*>(buf),
              nsamps * sizeof(std::complex<float>));
}




int main(int argc, char** argv)
{
 // parse
 std::string config_file = "basic_prach.yaml";
 for (int i = 1; i < argc; ++i) {
   if ((std::strcmp(argv[i], "--config") == 0 ||
        std::strcmp(argv[i], "-c") == 0) && i + 1 < argc) {
     config_file = argv[++i];
     break;
   }
 }
 if (config_file.empty()) {
   // std::fprintf(stderr, "Usage: %s --config <basic_prach.yaml>\n", argv[0]);
   return EXIT_FAILURE;
 }



 // load app config
 all_args_t args{};
 args = parseConfig(config_file);


 // Create USRP
 std::unique_ptr<Source> src = create_source_instance(args.rf.device);

 //cheat to match data types
 YAML::Node rf;
 rf["device_args"] = YAML::Load(args.rf.device_args);

  /////if uhd/zmq/what changes 
 source_error_t uspr = src->create(rf);
//  auto usrp = uhd::usrp::multi_usrp::make(args.rf.device_args);
 // Simple time base init
//  usrp->set_time_now(uhd::time_spec_t(0.0));


 // Build PRACH config
 srsran_prach_t     prach;
 srsran_prach_cfg_t prach_cfg;
 ZERO_OBJECT(prach_cfg);


 prach_cfg.is_nr            = args.is_nr;
 prach_cfg.config_idx       = args.config_idx;
 prach_cfg.hs_flag          = false;
 prach_cfg.freq_offset      = args.freq_offset;
 prach_cfg.root_seq_idx     = args.root_seq_idx;
 prach_cfg.zero_corr_zone   = args.zero_corr_zone;
 prach_cfg.num_ra_preambles = args.num_ra_preambles;


 // PRACH buffer (cf_t is srsRAN complex float)
 cf_t preamble[MAX_LEN]; //sarah
 typedef std::complex<float> cf_t_1;
 cf_t_1 preamble1{__real__ preamble,__imag__ preamble}; 

 memset(preamble, 0, sizeof(preamble));


 // Init PRACH
 if (srsran_prach_init(&prach, srsran_symbol_sz(args.nof_prb)) != SRSRAN_SUCCESS) {
   fprintf(stderr, "Failed to init PRACH object\n");
   return EXIT_FAILURE;
 }
 fprintf(stderr, "Sucessfully init PRACH object\n");


 // Configure PRACH
 timeval t0{}, t1{};
 gettimeofday(&t0, nullptr);
 if (srsran_prach_set_cfg(&prach, &prach_cfg, args.nof_prb) != SRSRAN_SUCCESS) {
   fprintf(stderr, "Error configuring PRACH object\n");
   srsran_prach_free(&prach);
   return EXIT_FAILURE;
 }
 fprintf(stderr, "Sucessfully config PRACH object\n");
 // gettimeofday(&t1, nullptr);
 //  printf("PRACH configured in %llu us\n", us_since(t0, t1));


 // Generate & TX preambles


 uint32_t max_preambles = (args.num_ra_preambles > 0)
                            ? args.num_ra_preambles
                            : 64u;


 const size_t nsamps_total = [&]() {
   // generate once to learn N_cp + N_seq
   if (srsran_prach_gen(&prach, 0, 0, preamble) != SRSRAN_SUCCESS) {
     srsran_prach_free(&prach);
     std::exit(EXIT_FAILURE);
   }
   return (size_t)(prach.N_cp + prach.N_seq);
 }();

    std::ofstream iq_out(args.rf.output_file, std::ios::binary);
    if (!iq_out) {
        throw std::runtime_error("Failed to open ./out/prach_dump2.cf32");
    }
 // simple spacing between bursts
 double base_offset_s  = 0.050;   // first burst 50 ms in the future
 double burst_spacing  = 0.005;   // 5 ms between bursts

 const std::size_t gap_samps = static_cast<std::size_t>(
  std::llround(burst_spacing * args.g_tx_rate)
 );

 ///make a blank gap tp put between bursts in iq file
 std::vector<cf_t> gap(gap_samps);
 std::fill(gap.begin(), gap.end(), (cf_t)0.0f);


    
      
  
   for (uint32_t seq_index = 0; seq_index < max_preambles; ++seq_index) {
      //save zeros for space between transmissions
      save_iq(iq_out, gap.data(), gap.size());
     // Generate: preamble = [ CP (N_cp) | sequence (N_seq) ]
     if (srsran_prach_gen(&prach, seq_index, 0 /* freq-shift idx */, preamble) != SRSRAN_SUCCESS) {
       // fprintf(stderr, "srsran_prach_gen failed at seq=%u\n", seq_index);
       srsran_prach_free(&prach);
       return EXIT_FAILURE;
     }

     preamble1{__real__ preamble,__imag__ preamble}; 

     double offset_s = base_offset_s + burst_spacing * seq_index;

     src->send(preamble1, nsamps_total);


    //  uhd_source.send(preamble, nsamps_total,
    //            args.g_tx_rate, args.g_center_freq_hz, args.g_tx_gain_db,
    //            offset_s);


     // Transmit CP + sequence over UHD
    //  tx_send_prach(usrp,
    //                preamble,
    //                nsamps_total,
    //                args.g_tx_rate,
    //                args.g_center_freq_hz,
    //                args.g_tx_gain_db,
    //                offset_s);
     save_iq(iq_out, preamble, nsamps_total);
    

   }
//  }


 // No detection / logging — just short bursts then exit.
 srsran_prach_free(&prach);
 return EXIT_SUCCESS;
}





