/*
 * ==========================================================================
 *  Mobile Broadband D2D Content Offloading Simulation
 * ==========================================================================
 *  Simulates a cluster of UEs where a "Cluster Head" downloads a file from
 *  the gNB and distributes it to other UEs via D2D sidelink multicast.
 *
 *  Generates CSV data for 10 required plots.
 *
 *  Compile:  g++ -std=c++17 -O2 -o d2d_sim d2d_simulation.cpp
 *  Run:      ./d2d_sim          (creates results/ directory with CSV files)
 * ==========================================================================
 */

#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <random>
#include <algorithm>
#include <numeric>
#include <string>
#include <iomanip>
#include <cstdlib>   // system()



// ---------------------------------------------------------------------------
//  Global constants & system parameters
// ---------------------------------------------------------------------------
constexpr double PI               = 3.14159265358979323846;
constexpr double SPEED_OF_LIGHT   = 3.0e8;      // m/s
constexpr double CARRIER_FREQ_GHZ = 3.5;         // NR n78 band
constexpr double SIDELINK_FREQ_GHZ= 5.9;         // NR sidelink (V2X band)
constexpr double GNB_TX_POWER_DBM = 46.0;        // gNB Tx power
constexpr double UE_TX_POWER_DBM  = 23.0;        // UE sidelink Tx power
constexpr double NOISE_FIGURE_DB  = 7.0;         // UE noise figure
constexpr double THERMAL_NOISE_DENSITY = -174.0; // dBm/Hz
constexpr double FILE_SIZE_MB     = 100.0;        // content file size
constexpr double PACKET_SIZE_BYTES= 1500.0;       // IP MTU
constexpr int    NUM_MONTE_CARLO  = 200;          // Monte-Carlo iterations

// ---------------------------------------------------------------------------
//  Utility helpers
// ---------------------------------------------------------------------------
static std::mt19937 rng(42);   // fixed seed for reproducibility

double dBm_to_W(double dBm) { return std::pow(10.0, (dBm - 30.0) / 10.0); }
double W_to_dBm(double W)   { return 10.0 * std::log10(W) + 30.0; }
double dB_to_linear(double dB) { return std::pow(10.0, dB / 10.0); }

// 3GPP TR 38.901 Urban-Micro LOS path-loss (simplified)
double pathLossUMi_LOS(double dist_m, double freq_ghz) {
    if (dist_m < 1.0) dist_m = 1.0;
    double fc = freq_ghz * 1e9;
    double PL = 32.4 + 21.0 * std::log10(dist_m) + 20.0 * std::log10(fc / 1e9);
    return PL;
}

// Sidelink path-loss (3GPP TR 37.885 / Winner-II D2D model)
double pathLossSidelink(double dist_m, double freq_ghz) {
    if (dist_m < 1.0) dist_m = 1.0;
    double PL = 32.4 + 20.0 * std::log10(dist_m) + 20.0 * std::log10(freq_ghz);
    // Shadowing std-dev σ = 4 dB for LOS sidelink
    std::normal_distribution<double> shadow(0.0, 4.0);
    PL += shadow(rng);
    return PL;
}

// Shannon capacity  C = BW * log2(1 + SINR)
double shannonCapacity(double bw_hz, double sinr_linear) {
    return bw_hz * std::log2(1.0 + sinr_linear);
}

// Compute SINR (linear) given Tx power (dBm), path-loss (dB), BW (Hz), interference (W)
double computeSINR(double txPower_dBm, double pathLoss_dB, double bw_hz,
                   double interference_W = 0.0) {
    double rxPower_W = dBm_to_W(txPower_dBm - pathLoss_dB);
    double noise_W   = dBm_to_W(THERMAL_NOISE_DENSITY + 10.0*std::log10(bw_hz) + NOISE_FIGURE_DB);
    return rxPower_W / (noise_W + interference_W);
}

// Jain's fairness index
double jainsFairness(const std::vector<double>& x) {
    double sum = 0, sumSq = 0;
    for (auto v : x) { sum += v; sumSq += v * v; }
    int n = (int)x.size();
    if (n == 0 || sumSq == 0.0) return 1.0;
    return (sum * sum) / (n * sumSq);
}

// ---------------------------------------------------------------------------
//  PLOT 1:  Backhaul Load Reduction (%) vs. Cluster Size
// ---------------------------------------------------------------------------
void plot1_backhaulLoadReduction(const std::string& dir) {
    std::ofstream f(dir + "/plot1_backhaul_load.csv");
    f << "ClusterSize,BackhaulReduction_percent\n";

    // Without D2D: every UE downloads individually → load = N * FILE_SIZE
    // With D2D  : only the Cluster Head (CH) downloads → load = 1 * FILE_SIZE
    //             + small signalling overhead per additional UE
    for (int N = 1; N <= 30; ++N) {
        double loadWithout = N * FILE_SIZE_MB;
        double signallingOverhead = 0.02 * FILE_SIZE_MB * (N - 1); // 2 % signalling per extra UE
        double loadWith = FILE_SIZE_MB + signallingOverhead;
        double reduction = (1.0 - loadWith / loadWithout) * 100.0;
        if (reduction < 0) reduction = 0;
        f << N << "," << std::fixed << std::setprecision(2) << reduction << "\n";
    }
    f.close();
    std::cout << "  [OK] Plot 1 data written.\n";
}

// ---------------------------------------------------------------------------
//  PLOT 2:  Multicast Throughput vs. Group Diameter (m)
// ---------------------------------------------------------------------------
void plot2_multicastThroughput(const std::string& dir) {
    std::ofstream f(dir + "/plot2_multicast_throughput.csv");
    f << "GroupDiameter_m,Throughput_Mbps\n";

    double bw_hz = 10e6; // 10 MHz sidelink BW
    for (int diam = 10; diam <= 200; diam += 5) {
        double avgThroughput = 0.0;
        for (int mc = 0; mc < NUM_MONTE_CARLO; ++mc) {
            // Worst-case user is at max radius = diam/2
            double maxDist = diam / 2.0;
            // Multicast rate limited by worst-case user
            std::uniform_real_distribution<double> distDist(1.0, maxDist);
            double worstDist = maxDist; // conservative: use edge user
            double pl = pathLossSidelink(worstDist, SIDELINK_FREQ_GHZ);
            double sinr = computeSINR(UE_TX_POWER_DBM, pl, bw_hz);
            double cap  = shannonCapacity(bw_hz, sinr) / 1e6; // Mbps
            avgThroughput += cap;
        }
        avgThroughput /= NUM_MONTE_CARLO;
        f << diam << "," << std::fixed << std::setprecision(3) << avgThroughput << "\n";
    }
    f.close();
    std::cout << "  [OK] Plot 2 data written.\n";
}

// ---------------------------------------------------------------------------
//  PLOT 3:  PDR (Packet Delivery Ratio) vs. Distance from Cluster Head
// ---------------------------------------------------------------------------
void plot3_pdrVsDistance(const std::string& dir) {
    std::ofstream f(dir + "/plot3_pdr_distance.csv");
    f << "Distance_m,PDR\n";

    double bw_hz = 10e6;
    double sinrThreshold_dB = 3.0; // minimum SINR for successful decode
    double sinrThreshold_lin = dB_to_linear(sinrThreshold_dB);

    for (int d = 5; d <= 150; d += 5) {
        int success = 0;
        for (int mc = 0; mc < NUM_MONTE_CARLO; ++mc) {
            double pl   = pathLossSidelink((double)d, SIDELINK_FREQ_GHZ);
            double sinr = computeSINR(UE_TX_POWER_DBM, pl, bw_hz);
            if (sinr >= sinrThreshold_lin) ++success;
        }
        double pdr = (double)success / NUM_MONTE_CARLO;
        f << d << "," << std::fixed << std::setprecision(4) << pdr << "\n";
    }
    f.close();
    std::cout << "  [OK] Plot 3 data written.\n";
}

// ---------------------------------------------------------------------------
//  PLOT 4:  Download Completion Time vs. Number of Re-transmissions
// ---------------------------------------------------------------------------
void plot4_completionTimeRetx(const std::string& dir) {
    std::ofstream f(dir + "/plot4_completion_time.csv");
    f << "NumRetransmissions,CompletionTime_s\n";

    double bw_hz = 10e6;
    double dist_ch_gnb = 200.0;  // CH distance from gNB
    double dist_d2d    = 30.0;   // avg D2D distance

    // gNB → CH download time
    double pl_gnb  = pathLossUMi_LOS(dist_ch_gnb, CARRIER_FREQ_GHZ);
    double sinr_gnb= computeSINR(GNB_TX_POWER_DBM, pl_gnb, 20e6); // 20 MHz DL
    double rate_gnb= shannonCapacity(20e6, sinr_gnb);              // bps
    double t_download = (FILE_SIZE_MB * 8e6) / rate_gnb;           // seconds

    // CH → UE D2D distribution time
    double pl_d2d  = pathLossSidelink(dist_d2d, SIDELINK_FREQ_GHZ);
    double sinr_d2d= computeSINR(UE_TX_POWER_DBM, pl_d2d, bw_hz);
    double rate_d2d= shannonCapacity(bw_hz, sinr_d2d);
    double t_d2d   = (FILE_SIZE_MB * 8e6) / rate_d2d;

    for (int retx = 0; retx <= 10; ++retx) {
        // Each retransmission adds time proportional to fraction of lost packets
        // Simple model: fraction needing retx decreases with each round
        double totalTime = t_download + t_d2d;
        double lostFraction = 0.15; // 15 % initial loss
        for (int r = 0; r < retx; ++r) {
            totalTime += t_d2d * lostFraction;
            lostFraction *= 0.3; // each retx recovers ~70 %
        }
        f << retx << "," << std::fixed << std::setprecision(3) << totalTime << "\n";
    }
    f.close();
    std::cout << "  [OK] Plot 4 data written.\n";
}

// ---------------------------------------------------------------------------
//  PLOT 5:  Energy Saved at gNB (%) vs. D2D Traffic Volume (%)
// ---------------------------------------------------------------------------
void plot5_energySaved(const std::string& dir) {
    std::ofstream f(dir + "/plot5_energy_saved.csv");
    f << "D2D_Traffic_Percent,EnergySaved_Percent\n";

    // gNB power model (3GPP / EARTH): P = P0 + delta_p * P_tx * load
    double P0      = 130.0;  // baseline power (W) at zero load
    double deltaP   = 4.7;
    double Pmax_tx  = dBm_to_W(GNB_TX_POWER_DBM);
    double fullLoad = P0 + deltaP * Pmax_tx;

    for (int pct = 0; pct <= 100; pct += 5) {
        double offloadFrac = pct / 100.0;
        double remainLoad  = 1.0 - offloadFrac;
        double Pwith       = P0 + deltaP * Pmax_tx * remainLoad;
        double saved        = (1.0 - Pwith / fullLoad) * 100.0;
        f << pct << "," << std::fixed << std::setprecision(2) << saved << "\n";
    }
    f.close();
    std::cout << "  [OK] Plot 5 data written.\n";
}

// ---------------------------------------------------------------------------
//  PLOT 6:  SINR (dB) vs. Multicast Feedback Rate (feedback/s)
// ---------------------------------------------------------------------------
void plot6_sinrFeedback(const std::string& dir) {
    std::ofstream f(dir + "/plot6_sinr_feedback.csv");
    f << "FeedbackRate_per_s,SINR_dB\n";

    // Higher feedback → more accurate MCS selection → better effective SINR
    // But excessive feedback → sidelink congestion → reduced SINR
    double bw_hz = 10e6;
    double dist  = 40.0; // metres
    double basePL = pathLossSidelink(dist, SIDELINK_FREQ_GHZ);
    // reset RNG for deterministic PL
    rng.seed(100);

    for (int fbRate = 1; fbRate <= 50; ++fbRate) {
        double avgSINR = 0.0;
        rng.seed(100 + fbRate);
        for (int mc = 0; mc < NUM_MONTE_CARLO; ++mc) {
            double pl = pathLossSidelink(dist, SIDELINK_FREQ_GHZ);
            // Feedback overhead consumes part of the BW
            double fbOverhead = std::min(0.8, 0.005 * fbRate); // fraction
            double usableBW   = bw_hz * (1.0 - fbOverhead);
            // MCS gain from feedback (diminishing returns)
            double mcsGain_dB = 3.0 * (1.0 - std::exp(-0.15 * fbRate));
            double sinr = computeSINR(UE_TX_POWER_DBM, pl - mcsGain_dB, usableBW);
            avgSINR += 10.0 * std::log10(sinr);
        }
        avgSINR /= NUM_MONTE_CARLO;
        f << fbRate << "," << std::fixed << std::setprecision(2) << avgSINR << "\n";
    }
    f.close();
    std::cout << "  [OK] Plot 6 data written.\n";
}

// ---------------------------------------------------------------------------
//  PLOT 7:  Fairness Index vs. User Position in Cluster
// ---------------------------------------------------------------------------
void plot7_fairnessPosition(const std::string& dir) {
    std::ofstream f(dir + "/plot7_fairness_position.csv");
    f << "NumUsers,NormalizedPosition,Throughput_Mbps,FairnessIndex\n";

    double bw_hz = 10e6;
    int clusterSize = 15;
    double maxRadius = 60.0; // cluster radius

    // Place users uniformly along the radius
    std::vector<double> throughputs(clusterSize);
    for (int i = 0; i < clusterSize; ++i) {
        double dist = maxRadius * (i + 1.0) / clusterSize;
        double avgTp = 0.0;
        rng.seed(200 + i);
        for (int mc = 0; mc < NUM_MONTE_CARLO; ++mc) {
            double pl = pathLossSidelink(dist, SIDELINK_FREQ_GHZ);
            double sinr = computeSINR(UE_TX_POWER_DBM, pl, bw_hz);
            avgTp += shannonCapacity(bw_hz, sinr) / 1e6;
        }
        avgTp /= NUM_MONTE_CARLO;
        throughputs[i] = avgTp;
    }

    double fairness = jainsFairness(throughputs);
    for (int i = 0; i < clusterSize; ++i) {
        double normPos = (i + 1.0) / clusterSize;
        f << clusterSize << "," << std::fixed << std::setprecision(3)
          << normPos << "," << throughputs[i] << "," << fairness << "\n";
    }
    f.close();
    std::cout << "  [OK] Plot 7 data written.\n";
}

// ---------------------------------------------------------------------------
//  PLOT 8:  Throughput (Mbps) vs. Sidelink Bandwidth (MHz)
// ---------------------------------------------------------------------------
void plot8_throughputBandwidth(const std::string& dir) {
    std::ofstream f(dir + "/plot8_throughput_bw.csv");
    f << "SidelinkBW_MHz,Throughput_Mbps\n";

    double dist = 30.0;
    for (int bw_mhz = 1; bw_mhz <= 40; ++bw_mhz) {
        double bw_hz = bw_mhz * 1e6;
        double avgTp = 0.0;
        rng.seed(300 + bw_mhz);
        for (int mc = 0; mc < NUM_MONTE_CARLO; ++mc) {
            double pl   = pathLossSidelink(dist, SIDELINK_FREQ_GHZ);
            double sinr = computeSINR(UE_TX_POWER_DBM, pl, bw_hz);
            avgTp += shannonCapacity(bw_hz, sinr) / 1e6;
        }
        avgTp /= NUM_MONTE_CARLO;
        f << bw_mhz << "," << std::fixed << std::setprecision(3) << avgTp << "\n";
    }
    f.close();
    std::cout << "  [OK] Plot 8 data written.\n";
}

// ---------------------------------------------------------------------------
//  PLOT 9:  Impact of Human Blockage on Cluster Throughput
// ---------------------------------------------------------------------------
void plot9_humanBlockage(const std::string& dir) {
    std::ofstream f(dir + "/plot9_blockage_throughput.csv");
    f << "BlockageProbability,ClusterThroughput_Mbps\n";

    double bw_hz = 10e6;
    int clusterSize = 10;
    double maxRadius = 50.0;
    double blockageLoss_dB = 20.0; // typical human-body blockage at 5.9 GHz

    for (int bp = 0; bp <= 100; bp += 5) {
        double blockProb = bp / 100.0;
        double totalTp = 0.0;
        rng.seed(400 + bp);

        for (int mc = 0; mc < NUM_MONTE_CARLO; ++mc) {
            double sumTp = 0.0;
            for (int u = 0; u < clusterSize; ++u) {
                double dist = maxRadius * (u + 1.0) / clusterSize;
                double pl = pathLossSidelink(dist, SIDELINK_FREQ_GHZ);
                // Apply blockage with given probability
                std::bernoulli_distribution blocked(blockProb);
                if (blocked(rng)) pl += blockageLoss_dB;
                double sinr = computeSINR(UE_TX_POWER_DBM, pl, bw_hz);
                sumTp += shannonCapacity(bw_hz, sinr) / 1e6;
            }
            totalTp += sumTp;
        }
        totalTp /= NUM_MONTE_CARLO;
        f << std::fixed << std::setprecision(2) << blockProb << "," << totalTp << "\n";
    }
    f.close();
    std::cout << "  [OK] Plot 9 data written.\n";
}

// ---------------------------------------------------------------------------
//  PLOT 10: Buffer Delay (ms) vs. Content Popularity (Zipf rank)
// ---------------------------------------------------------------------------
void plot10_bufferDelay(const std::string& dir) {
    std::ofstream f(dir + "/plot10_buffer_delay.csv");
    f << "PopularityRank,BufferDelay_ms\n";

    // More popular content → more likely cached at CH → lower buffer delay
    // Zipf distribution with exponent α = 0.8
    double alpha = 0.8;
    int numContents = 50;
    double cacheSizeFrac = 0.2; // CH caches top 20 % of content

    // Pre-compute Zipf probabilities
    double normConst = 0.0;
    for (int k = 1; k <= numContents; ++k) normConst += 1.0 / std::pow(k, alpha);

    for (int rank = 1; rank <= numContents; ++rank) {
        double prob = (1.0 / std::pow(rank, alpha)) / normConst;

        // If content is in cache (rank <= cacheSizeFrac * N), delay is just D2D latency
        double d2d_latency_ms = 5.0;   // D2D one-hop latency
        double gnb_latency_ms = 50.0;  // fetch from gNB adds backhaul RTT
        double retx_delay_ms  = 15.0;  // avg re-tx delay

        bool cached = (rank <= (int)(cacheSizeFrac * numContents));
        double bufferDelay;
        if (cached) {
            bufferDelay = d2d_latency_ms + 2.0 * rank; // slight increase with rank
        } else {
            // Must fetch from gNB first, then distribute
            double congestionFactor = 1.0 + 0.5 * (1.0 - prob); // less popular → less optimised
            bufferDelay = gnb_latency_ms + d2d_latency_ms + retx_delay_ms * congestionFactor;
        }
        f << rank << "," << std::fixed << std::setprecision(2) << bufferDelay << "\n";
    }
    f.close();
    std::cout << "  [OK] Plot 10 data written.\n";
}

// ---------------------------------------------------------------------------
//  main
// ---------------------------------------------------------------------------
int main() {
    std::cout << "=============================================================\n";
    std::cout << " Mobile Broadband D2D Content Offloading Simulation\n";
    std::cout << "=============================================================\n\n";

    std::string outDir = "results";
#ifdef _WIN32
    system(("mkdir " + outDir + " 2>nul").c_str());
#else
    system(("mkdir -p " + outDir).c_str());
#endif

    std::cout << "Generating simulation data…\n\n";

    plot1_backhaulLoadReduction(outDir);
    plot2_multicastThroughput(outDir);
    plot3_pdrVsDistance(outDir);
    plot4_completionTimeRetx(outDir);
    plot5_energySaved(outDir);
    plot6_sinrFeedback(outDir);
    plot7_fairnessPosition(outDir);
    plot8_throughputBandwidth(outDir);
    plot9_humanBlockage(outDir);
    plot10_bufferDelay(outDir);

    std::cout << "\nAll CSV files written to '" << outDir << "/' directory.\n";
    std::cout << "Run:  python plot_results.py   to generate the 10 plots.\n";
    return 0;
}
