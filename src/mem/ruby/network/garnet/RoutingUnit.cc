/*
 * Copyright (c) 2008 Princeton University
 * Copyright (c) 2016 Georgia Institute of Technology
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "mem/ruby/network/garnet/RoutingUnit.hh"

#include "base/cast.hh"
#include "base/compiler.hh"
#include "debug/RubyNetwork.hh"
#include "mem/ruby/network/garnet/InputUnit.hh"
#include "mem/ruby/network/garnet/Router.hh"
#include "mem/ruby/slicc_interface/Message.hh"
#include "mem/ruby/network/garnet/OutputUnit.hh"

namespace gem5
{

namespace ruby
{

namespace garnet
{

RoutingUnit::RoutingUnit(Router *router)
{
    m_router = router;
    m_routing_table.clear();
    m_weight_table.clear();

    

    m_r2d2_initialized = false;

   // MPAD Sensor Initialization
    m_alarm_triggered = false;
    m_last_dest = -1;
    m_dest_entropy_count = 0;
    
    // Initialize Window Trackers
    m_window_start = 0;
    m_packets_in_window = 0;
    
    // Initialize Burst Trackers
    m_last_injection_cycle = 0;
    m_consecutive_injections = 0;
  
}

void
RoutingUnit::addRoute(std::vector<NetDest>& routing_table_entry)
{
    if (routing_table_entry.size() > m_routing_table.size()) {
        m_routing_table.resize(routing_table_entry.size());
    }
    for (int v = 0; v < routing_table_entry.size(); v++) {
        m_routing_table[v].push_back(routing_table_entry[v]);
    }
}

void
RoutingUnit::addWeight(int link_weight)
{
    m_weight_table.push_back(link_weight);
}

bool
RoutingUnit::supportsVnet(int vnet, std::vector<int> sVnets)
{
    // If all vnets are supported, return true
    if (sVnets.size() == 0) {
        return true;
    }

    // Find the vnet in the vector, return true
    if (std::find(sVnets.begin(), sVnets.end(), vnet) != sVnets.end()) {
        return true;
    }

    // Not supported vnet
    return false;
}

int
RoutingUnit::getDownstreamBOCSum(PortDirection dirn)
{
    int num_cols = m_router->get_net_ptr()->getNumCols();
    int my_id = m_router->get_id();
    int neighbor_id = -1;

    if (dirn == "East") neighbor_id = my_id + 1;
    else if (dirn == "West") neighbor_id = my_id - 1;
    else if (dirn == "North") neighbor_id = my_id + num_cols;
    else if (dirn == "South") neighbor_id = my_id - num_cols;

    int total_boc = 0;
    
    if (neighbor_id >= 0 && neighbor_id < m_router->get_net_ptr()->getNumRouters()) {
        Router* next_hop = m_router->get_net_ptr()->get_router(neighbor_id);
        for (int i = 0; i < next_hop->get_num_outports(); i++) {
            total_boc += next_hop->get_BOC(i);
        }
    }
    return total_boc;
}

int RoutingUnit::getDownstreamFluiditySum(PortDirection dirn) {
    int num_cols = m_router->get_net_ptr()->getNumCols();
    int neighbor_id = -1;
    
    if (dirn == "East") neighbor_id = m_router->get_id() + 1;
    else if (dirn == "West") neighbor_id = m_router->get_id() - 1;
    else if (dirn == "North") neighbor_id = m_router->get_id() + num_cols;
    else if (dirn == "South") neighbor_id = m_router->get_id() - num_cols;

    int total_fluidity = 0;
    if (neighbor_id >= 0 && neighbor_id < m_router->get_net_ptr()->getNumRouters()) {
        Router* next_hop = m_router->get_net_ptr()->get_router(neighbor_id);
        for (int i = 0; i < next_hop->get_num_outports(); i++) {
            total_fluidity += next_hop->get_fluidity(i);
        }
    }
    return total_fluidity;
}

int RoutingUnit::getNOPScore(PortDirection dirn, RouteInfo route) {
    int num_cols = m_router->get_net_ptr()->getNumCols();
    int neighbor_id = -1;
    
    if (dirn == "East") neighbor_id = m_router->get_id() + 1;
    else if (dirn == "West") neighbor_id = m_router->get_id() - 1;
    else if (dirn == "North") neighbor_id = m_router->get_id() + num_cols;
    else if (dirn == "South") neighbor_id = m_router->get_id() - num_cols;

    if (neighbor_id < 0 || neighbor_id >= m_router->get_net_ptr()->getNumRouters()) return 0;
    if (neighbor_id == route.dest_router) return 9999; // Prioritize direct delivery

    // NOP logic: Calculate valid Odd-Even paths FROM the neighbor TO the destination
    Router* next_hop = m_router->get_net_ptr()->get_router(neighbor_id);
    int n_x = neighbor_id % num_cols;
    int dest_x = route.dest_router % num_cols;
    int x_hops = abs(dest_x - n_x);
    bool x_dirn = (dest_x >= n_x);
    bool y_dirn = ((route.dest_router / num_cols) >= (neighbor_id / num_cols));

    std::vector<PortDirection> valid_2hop_ports;
    if (x_hops == 0) valid_2hop_ports.push_back(y_dirn ? "North" : "South");
    else if ((route.dest_router / num_cols) == (neighbor_id / num_cols)) valid_2hop_ports.push_back(x_dirn ? "East" : "West");
    else {
        if (x_dirn) {
            if (n_x % 2 != 0 || n_x == dest_x) valid_2hop_ports.push_back(y_dirn ? "North" : "South");
            if (dest_x % 2 != 0 || x_hops != 1) valid_2hop_ports.push_back("East");
        } else {
            valid_2hop_ports.push_back("West");
            if (n_x % 2 == 0) valid_2hop_ports.push_back(y_dirn ? "North" : "South");
        }
    }

    int score = 0;
    for (const auto& port : valid_2hop_ports) {
        // Direct Next-Hop Credit Counting
        if (m_outports_dirn2idx.find(port) != m_outports_dirn2idx.end()) {
            int out_idx = m_outports_dirn2idx[port];
            // THE FIX: Ensure the out_idx is physically valid for the neighbor router
            if (out_idx < 0 || out_idx >= next_hop->get_num_outports()) {
                continue; // Skip this port, it doesn't exist on the neighbor!
}
            OutputUnit* out_unit = next_hop->getOutputUnit(out_idx);
            int vcs_per_vnet = m_router->get_vc_per_vnet();
            for (int vc = route.vnet * vcs_per_vnet; vc < (route.vnet + 1) * vcs_per_vnet; vc++) {
                score += out_unit->get_credit_count(vc);
            }
        }
    }
    return score;
}

double
RoutingUnit::getDownstreamCFC(PortDirection dirn)
{
    int num_cols = m_router->get_net_ptr()->getNumCols();
    int neighbor_id = -1;

    if (dirn == "East") neighbor_id = m_router->get_id() + 1;
    else if (dirn == "West") neighbor_id = m_router->get_id() - 1;
    else if (dirn == "North") neighbor_id = m_router->get_id() + num_cols;
    else if (dirn == "South") neighbor_id = m_router->get_id() - num_cols;

    double total_cfc = 0.0;
    
    // Make sure the neighbor physically exists on the chip
    if (neighbor_id >= 0 && neighbor_id < m_router->get_net_ptr()->getNumRouters()) {
        Router* next_hop = m_router->get_net_ptr()->get_router(neighbor_id);
        
        // Sum up the EWMA CFC from all output ports of the downstream router
        for (int i = 0; i < next_hop->get_num_outports(); i++) {
            total_cfc += next_hop->getOutputUnit(i)->get_cfc();
        }
    }
    return total_cfc;
}



void RoutingUnit::runMPADDetection(RouteInfo route, PortDirection inport_dirn)
{
    if (m_alarm_triggered) return; 
    if (inport_dirn != "Local") return; 

    gem5::Tick cur_cycle = m_router->curCycle();


    if (cur_cycle - m_window_start > 500) { 
        m_packets_in_window = 0;
        m_dest_entropy_count = 0; // Reset entropy every window
        m_window_start = cur_cycle;
    }
    m_packets_in_window++;


    // PROOF 1: Destination Entropy

    if (route.dest_router == m_last_dest) {
        m_dest_entropy_count++;
    } else {
        m_dest_entropy_count = 0; 
        m_last_dest = route.dest_router;
    }


    // PROOF 2: VC Saturation 
    int total_free_vcs = 0;
    for (int i = 0; i < m_router->get_num_outports(); i++) {
        OutputUnit* out_unit = m_router->getOutputUnit(i);
        for (int vc = 0; vc < m_router->get_num_vcs(); vc++) {
            total_free_vcs += out_unit->get_credit_count(vc);
        }
    }
    int vc_saturation_penalty = (total_free_vcs < 20) ? 10 : 0; 


    // Normal node sends ~20 packets to the same destination in 500 cycles.
    // Trojan sends ~50 packets to the same destination.
    int p1_score = (m_dest_entropy_count / 10); // Normal: 2. Trojan: 5.
    
    // Divide by 2 so it doesn't overpower the other metrics
    int p3_score = (m_packets_in_window / 2); // Normal: 10. Trojan: 25.
    
    int p2_score = vc_saturation_penalty;     // Normal: 0. Trojan: 10.

    // Threat = (P1 * 4) + (P2 * 2) + P3
    // Normal Threat = (2*4) + (0) + 10 = 18
    // Trojan Threat = (5*4) + (10*2) + 25 = 65
    int threat_score = (p1_score << 2) + (p2_score << 1) + p3_score;


    // Any score over 10 will print, so you can watch both Normal and Trojan traffic.
    if (threat_score > 10 && m_packets_in_window % 5 == 0) {
        std::cout << "[DEBUG] Cycle " << cur_cycle 
                  << " | Node " << m_router->get_id() 
                  << " Threat Score: " << threat_score << "\n";
    }


    if (threat_score > 45) { 
        m_alarm_triggered = true;
        int my_id = m_router->get_id();


        std::cout << "MPAD ALARM: Hardware Trojan Detected at Cycle " << cur_cycle << "!\n";
        std::cout << " Malicious Node Quarantined: Router ID " << my_id << "\n";
        std::cout << "Calculated Threat Score: " << threat_score << "\n";


        m_router->get_net_ptr()->set_detected_trojan_id(my_id);
    }
}

void RoutingUnit::calculateR2D2Penalties()
{
    int num_cols = m_router->get_net_ptr()->getNumCols();
    int my_id = m_router->get_id();
    
    // DYNAMIC FETCH: Get the ID caught by the MPAD sensor
    int dynamic_threat_id = m_router->get_net_ptr()->get_detected_trojan_id();

    std::vector<int> active_trojans;
    if (dynamic_threat_id != -1) {
        active_trojans.push_back(dynamic_threat_id);
    }

    // Reset table
    m_r2d2_penalty_table["North"] = 0;
    m_r2d2_penalty_table["South"] = 0;
    m_r2d2_penalty_table["East"]  = 0;
    m_r2d2_penalty_table["West"]  = 0;

    std::vector<PortDirection> dirs = {"North", "South", "East", "West"};

    for (int trojan_id : active_trojans) {
        int trojan_x = trojan_id % num_cols;
        int trojan_y = trojan_id / num_cols;

        for (PortDirection dir : dirs) {
            if (m_outports_dirn2idx.find(dir) == m_outports_dirn2idx.end()) continue;

            int next_id = -1;
            if (dir == "East")       next_id = my_id + 1;
            else if (dir == "West")  next_id = my_id - 1;
            else if (dir == "North") next_id = my_id + num_cols;
            else if (dir == "South") next_id = my_id - num_cols;

            int next_x = next_id % num_cols;
            int next_y = next_id / num_cols;

            int distance = std::abs(next_x - trojan_x) + std::abs(next_y - trojan_y);

            if (distance == 0) m_r2d2_penalty_table[dir] += 999;
            else if (distance == 1) m_r2d2_penalty_table[dir] += 10;
            else if (distance == 2) m_r2d2_penalty_table[dir] += 2;
        }
    }
    m_r2d2_initialized = true;
}

int
RoutingUnit::getFreeCredits(PortDirection dirn, int vnet)
{
    if (m_outports_dirn2idx.find(dirn) == m_outports_dirn2idx.end()) {
        return 0; // Port doesn't exist
    }
    
    int outport_idx = m_outports_dirn2idx[dirn];
    OutputUnit* out_unit = m_router->getOutputUnit(outport_idx);
    
    int total_credits = 0;
    
    // Rigorous Vnet isolation: Only poll VCs belonging to the current vnet
    int vcs_per_vnet = m_router->get_vc_per_vnet();
    int start_vc = vnet * vcs_per_vnet;
    int end_vc = start_vc + vcs_per_vnet;
    
    for (int vc = start_vc; vc < end_vc; vc++) {
        total_credits += out_unit->get_credit_count(vc);
    }
    
    return total_credits;
}

/*
 * This is the default routing algorithm in garnet.
 * The routing table is populated during topology creation.
 * Routes can be biased via weight assignments in the topology file.
 * Correct weight assignments are critical to provide deadlock avoidance.
 */
int
RoutingUnit::lookupRoutingTable(int vnet, NetDest msg_destination)
{
    // First find all possible output link candidates
    // For ordered vnet, just choose the first
    // (to make sure different packets don't choose different routes)
    // For unordered vnet, randomly choose any of the links
    // To have a strict ordering between links, they should be given
    // different weights in the topology file

    int output_link = -1;
    int min_weight = INFINITE_;
    std::vector<int> output_link_candidates;
    int num_candidates = 0;

    // Identify the minimum weight among the candidate output links
    for (int link = 0; link < m_routing_table[vnet].size(); link++) {
        if (msg_destination.intersectionIsNotEmpty(
            m_routing_table[vnet][link])) {

        if (m_weight_table[link] <= min_weight)
            min_weight = m_weight_table[link];
        }
    }

    // Collect all candidate output links with this minimum weight
    for (int link = 0; link < m_routing_table[vnet].size(); link++) {
        if (msg_destination.intersectionIsNotEmpty(
            m_routing_table[vnet][link])) {

            if (m_weight_table[link] == min_weight) {
                num_candidates++;
                output_link_candidates.push_back(link);
            }
        }
    }

    if (output_link_candidates.size() == 0) {
        fatal("Fatal Error:: No Route exists from this Router.");
        exit(0);
    }

    // Randomly select any candidate output link
    int candidate = 0;
    if (!(m_router->get_net_ptr())->isVNetOrdered(vnet))
        candidate = rand() % num_candidates;

    output_link = output_link_candidates.at(candidate);
    return output_link;
}


void
RoutingUnit::addInDirection(PortDirection inport_dirn, int inport_idx)
{
    m_inports_dirn2idx[inport_dirn] = inport_idx;
    m_inports_idx2dirn[inport_idx]  = inport_dirn;
}

void
RoutingUnit::addOutDirection(PortDirection outport_dirn, int outport_idx)
{
    m_outports_dirn2idx[outport_dirn] = outport_idx;
    m_outports_idx2dirn[outport_idx]  = outport_dirn;
}

// outportCompute() is called by the InputUnit
// It calls the routing table by default.
// A template for adaptive topology-specific routing algorithm
// implementations using port directions rather than a static routing
// table is provided here.

int
RoutingUnit::outportCompute(RouteInfo route, int inport,
                            PortDirection inport_dirn)
{
    int outport = -1;

    if (route.dest_router == m_router->get_id()) {

        // Multiple NIs may be connected to this router,
        // all with output port direction = "Local"
        // Get exact outport id from table
        outport = lookupRoutingTable(route.vnet, route.net_dest);
        return outport;
    }

    // Routing Algorithm set in GarnetNetwork.py
    // Can be over-ridden from command line using --routing-algorithm = 1
    RoutingAlgorithm routing_algorithm =
        (RoutingAlgorithm) m_router->get_net_ptr()->getRoutingAlgorithm();

    switch (routing_algorithm) {
        case TABLE_:  outport =
            lookupRoutingTable(route.vnet, route.net_dest); break;
        case XY_:     outport =
            outportComputeXY(route, inport, inport_dirn); break;
        case BOF_: outport = 
            outportComputeBOFAR(route, inport, inport_dirn); break;
        // any custom algorithm
        // case CUSTOM_: outport =
        //     outportComputeCustom(route, inport, inport_dirn); break;
        case FVC_: outport = 
            outportComputeFVC(route, inport, inport_dirn); break;
        case FON_: outport =
            outportComputeFON(route, inport, inport_dirn); break;
        case NOP_: outport = 
            outportComputeNOP(route, inport, inport_dirn); break;
        case CFC_: outport = 
            outportComputeTRACKER(route, inport, inport_dirn); break;
        case R2D2_: outport =
            outportComputeR2D2(route, inport, inport_dirn); break;
        case ODD_EVEN_: outport =
            outportComputeODD_EVEN(route, inport, inport_dirn); break;
        // DyXY algorithm
        case DyXY_: outport =
            outportComputeDyXY(route, inport, inport_dirn); break;
        default: outport =
            lookupRoutingTable(route.vnet, route.net_dest); break;
    }

    assert(outport != -1);
    return outport;
}

// XY routing implemented using port directions
// Only for reference purpose in a Mesh
// By default Garnet uses the routing table
int
RoutingUnit::outportComputeXY(RouteInfo route,
                              int inport,
                              PortDirection inport_dirn)
{
    PortDirection outport_dirn = "Unknown";

    [[maybe_unused]] int num_rows = m_router->get_net_ptr()->getNumRows();
    int num_cols = m_router->get_net_ptr()->getNumCols();
    assert(num_rows > 0 && num_cols > 0);

    int my_id = m_router->get_id();
    int my_x = my_id % num_cols;
    int my_y = my_id / num_cols;

    int dest_id = route.dest_router;
    int dest_x = dest_id % num_cols;
    int dest_y = dest_id / num_cols;

    int x_hops = abs(dest_x - my_x);
    int y_hops = abs(dest_y - my_y);

    bool x_dirn = (dest_x >= my_x);
    bool y_dirn = (dest_y >= my_y);

    // already checked that in outportCompute() function
    assert(!(x_hops == 0 && y_hops == 0));

    if (x_hops > 0) {
        if (x_dirn) {
            assert(inport_dirn == "Local" || inport_dirn == "West");
            outport_dirn = "East";
        } else {
            assert(inport_dirn == "Local" || inport_dirn == "East");
            outport_dirn = "West";
        }
    } else if (y_hops > 0) {
        if (y_dirn) {
            // "Local" or "South" or "West" or "East"
            assert(inport_dirn != "North");
            outport_dirn = "North";
        } else {
            // "Local" or "North" or "West" or "East"
            assert(inport_dirn != "South");
            outport_dirn = "South";
        }
    } else {
        // x_hops == 0 and y_hops == 0
        // this is not possible
        // already checked that in outportCompute() function
        panic("x_hops == y_hops == 0");
    }

    return m_outports_dirn2idx[outport_dirn];
}

int
RoutingUnit::outportComputeBOFAR(RouteInfo route, int inport, PortDirection inport_dirn)
{
    PortDirection outport_dirn = "Unknown";
    int num_cols = m_router->get_net_ptr()->getNumCols();
    int my_id = m_router->get_id();
    int my_x = my_id % num_cols;
    int my_y = my_id / num_cols;
    
    int dest_id = route.dest_router;
    int dest_x = dest_id % num_cols;
    int dest_y = dest_id / num_cols;

    int x_hops = abs(dest_x - my_x);
    int y_hops = abs(dest_y - my_y);
    bool x_dirn = (dest_x >= my_x);
    bool y_dirn = (dest_y >= my_y);

    std::vector<PortDirection> valid_ports;

    // Deadlock-Free Odd-Even Turn Model Path Constraints
    if (x_hops > 0 && y_hops > 0) {
        if (x_dirn) { // Eastbound
            if (my_x % 2 != 0 || my_x == dest_x) valid_ports.push_back(y_dirn ? "North" : "South");
            if (dest_x % 2 != 0 || x_hops != 1) valid_ports.push_back("East");
        } else { // Westbound
            valid_ports.push_back("West");
            if (my_x % 2 == 0) valid_ports.push_back(y_dirn ? "North" : "South");
        }
    } else if (x_hops > 0) {
        valid_ports.push_back(x_dirn ? "East" : "West");
    } else if (y_hops > 0) {
        valid_ports.push_back(y_dirn ? "North" : "South");
    }

    if (valid_ports.empty()) {
        return outportComputeXY(route, inport, inport_dirn);
    }

    int min_boc = 999999;
    bool found_valid_port = false;

    for (const auto& port : valid_ports) {
        if (m_outports_dirn2idx.find(port) == m_outports_dirn2idx.end()) continue;

        int boc = getDownstreamBOCSum(port);
        if (boc < min_boc) {
            min_boc = boc;
            outport_dirn = port;
            found_valid_port = true;
        }
    }

    if (!found_valid_port) {
        return outportComputeXY(route, inport, inport_dirn);
    }

    return m_outports_dirn2idx[outport_dirn];
}

// --- FVC Routing ---
int RoutingUnit::outportComputeFVC(RouteInfo route, int inport, PortDirection inport_dirn) {
    PortDirection outport_dirn = "Unknown";
    int num_cols = m_router->get_net_ptr()->getNumCols();
    int my_id = m_router->get_id();
    int my_x = my_id % num_cols;
    int my_y = my_id / num_cols;
    
    int dest_id = route.dest_router;
    int dest_x = dest_id % num_cols;
    int dest_y = dest_id / num_cols;

    int x_hops = abs(dest_x - my_x);
    int y_hops = abs(dest_y - my_y);
    bool x_dirn = (dest_x >= my_x);
    bool y_dirn = (dest_y >= my_y);

    std::vector<PortDirection> valid_ports;

    if (x_hops > 0 && y_hops > 0) {
        if (x_dirn) { 
            if (my_x % 2 != 0 || my_x == dest_x) valid_ports.push_back(y_dirn ? "North" : "South");
            if (dest_x % 2 != 0 || x_hops != 1) valid_ports.push_back("East");
        } else { 
            valid_ports.push_back("West");
            if (my_x % 2 == 0) valid_ports.push_back(y_dirn ? "North" : "South");
        }
    } else if (x_hops > 0) {
        valid_ports.push_back(x_dirn ? "East" : "West");
    } else if (y_hops > 0) {
        valid_ports.push_back(y_dirn ? "North" : "South");
    }

    if (valid_ports.empty()) {
        return outportComputeXY(route, inport, inport_dirn);
    }

    int max_score = -1;
    bool found_valid_port = false;

    for (const auto& port : valid_ports) {
        if (m_outports_dirn2idx.find(port) == m_outports_dirn2idx.end()) continue;

        int score = 0;
        int out_idx = m_outports_dirn2idx[port];
        OutputUnit* out_unit = m_router->getOutputUnit(out_idx);
        int vcs_per_vnet = m_router->get_vc_per_vnet();
        
        for (int vc = route.vnet * vcs_per_vnet; vc < (route.vnet + 1) * vcs_per_vnet; vc++) {
            score += out_unit->get_credit_count(vc);
        }
        
        if (score > max_score) { 
            max_score = score; 
            outport_dirn = port; 
            found_valid_port = true;
        }
    }

    if (!found_valid_port) {
        return outportComputeXY(route, inport, inport_dirn);
    }

    return m_outports_dirn2idx[outport_dirn];
}

// --- FON Routing ---
int RoutingUnit::outportComputeFON(RouteInfo route, int inport, PortDirection inport_dirn) {
    PortDirection outport_dirn = "Unknown";
    int num_cols = m_router->get_net_ptr()->getNumCols();
    int my_id = m_router->get_id();
    int my_x = my_id % num_cols;
    int my_y = my_id / num_cols;
    
    int dest_id = route.dest_router;
    int dest_x = dest_id % num_cols;
    int dest_y = dest_id / num_cols;

    int x_hops = abs(dest_x - my_x);
    int y_hops = abs(dest_y - my_y);
    bool x_dirn = (dest_x >= my_x);
    bool y_dirn = (dest_y >= my_y);

    std::vector<PortDirection> valid_ports;

    if (x_hops > 0 && y_hops > 0) {
        if (x_dirn) { 
            if (my_x % 2 != 0 || my_x == dest_x) valid_ports.push_back(y_dirn ? "North" : "South");
            if (dest_x % 2 != 0 || x_hops != 1) valid_ports.push_back("East");
        } else { 
            valid_ports.push_back("West");
            if (my_x % 2 == 0) valid_ports.push_back(y_dirn ? "North" : "South");
        }
    } else if (x_hops > 0) {
        valid_ports.push_back(x_dirn ? "East" : "West");
    } else if (y_hops > 0) {
        valid_ports.push_back(y_dirn ? "North" : "South");
    }

    if (valid_ports.empty()) {
        return outportComputeXY(route, inport, inport_dirn);
    }

    int max_score = -1;
    bool found_valid_port = false;

    for (const auto& port : valid_ports) {
        if (m_outports_dirn2idx.find(port) == m_outports_dirn2idx.end()) continue;

        int score = getDownstreamFluiditySum(port);
        if (score > max_score) { 
            max_score = score; 
            outport_dirn = port; 
            found_valid_port = true;
        }
    }

    if (!found_valid_port) {
        return outportComputeXY(route, inport, inport_dirn);
    }

    return m_outports_dirn2idx[outport_dirn];
}

// --- NOP Routing ---
int RoutingUnit::outportComputeNOP(RouteInfo route, int inport, PortDirection inport_dirn) {
    PortDirection outport_dirn = "Unknown";
    int num_cols = m_router->get_net_ptr()->getNumCols();
    int my_id = m_router->get_id();
    int my_x = my_id % num_cols;
    int my_y = my_id / num_cols;
    
    int dest_id = route.dest_router;
    int dest_x = dest_id % num_cols;
    int dest_y = dest_id / num_cols;

    int x_hops = abs(dest_x - my_x);
    int y_hops = abs(dest_y - my_y);
    bool x_dirn = (dest_x >= my_x);
    bool y_dirn = (dest_y >= my_y);

    std::vector<PortDirection> valid_ports;

    if (x_hops > 0 && y_hops > 0) {
        if (x_dirn) { 
            if (my_x % 2 != 0 || my_x == dest_x) valid_ports.push_back(y_dirn ? "North" : "South");
            if (dest_x % 2 != 0 || x_hops != 1) valid_ports.push_back("East");
        } else { 
            valid_ports.push_back("West");
            if (my_x % 2 == 0) valid_ports.push_back(y_dirn ? "North" : "South");
        }
    } else if (x_hops > 0) {
        valid_ports.push_back(x_dirn ? "East" : "West");
    } else if (y_hops > 0) {
        valid_ports.push_back(y_dirn ? "North" : "South");
    }

    if (valid_ports.empty()) {
        return outportComputeXY(route, inport, inport_dirn);
    }

    int max_score = -1;
    bool found_valid_port = false;

    for (const auto& port : valid_ports) {
        if (m_outports_dirn2idx.find(port) == m_outports_dirn2idx.end()) continue;

        int score = getNOPScore(port, route);
        if (score > max_score) { 
            max_score = score; 
            outport_dirn = port; 
            found_valid_port = true;
        }
    }

    if (!found_valid_port) {
        return outportComputeXY(route, inport, inport_dirn);
    }

    return m_outports_dirn2idx[outport_dirn];
}

// --- TRACKER Routing (ICCAD 2012) ---
int RoutingUnit::outportComputeTRACKER(RouteInfo route, int inport, PortDirection inport_dirn) {
    PortDirection outport_dirn = "Unknown";
    int num_cols = m_router->get_net_ptr()->getNumCols();
    int my_id = m_router->get_id();
    int my_x = my_id % num_cols;
    int my_y = my_id / num_cols;
    
    int dest_id = route.dest_router;
    int dest_x = dest_id % num_cols;
    int dest_y = dest_id / num_cols;

    int x_hops = abs(dest_x - my_x);
    int y_hops = abs(dest_y - my_y);
    bool x_dirn = (dest_x >= my_x);
    bool y_dirn = (dest_y >= my_y);

    std::vector<PortDirection> valid_ports;

    // Phase 1: Odd-Even Routing Restrictions (Ensures Deadlock Freedom)
    if (x_hops > 0 && y_hops > 0) {
        if (x_dirn) { 
            if (my_x % 2 != 0 || my_x == dest_x) valid_ports.push_back(y_dirn ? "North" : "South");
            if (dest_x % 2 != 0 || x_hops != 1) valid_ports.push_back("East");
        } else { 
            valid_ports.push_back("West");
            if (my_x % 2 == 0) valid_ports.push_back(y_dirn ? "North" : "South");
        }
    } else if (x_hops > 0) {
        valid_ports.push_back(x_dirn ? "East" : "West");
    } else if (y_hops > 0) {
        valid_ports.push_back(y_dirn ? "North" : "South");
    }

    // Safety Fallback: If Odd-Even fails to find a path, default to XY
    if (valid_ports.empty()) {
        return outportComputeXY(route, inport, inport_dirn);
    }

    // Phase 2: TRACKER Selection (Load Balancing)
    // Goal: MINIMIZE the smoothed CFC to route away from congested links
    double min_cfc = 999999.0; // Start with an arbitrarily high threshold
    bool found_valid_port = false;

    for (const auto& port : valid_ports) {
        if (m_outports_dirn2idx.find(port) == m_outports_dirn2idx.end()) continue;

        int out_idx = m_outports_dirn2idx[port];
        OutputUnit* out_unit = m_router->getOutputUnit(out_idx);
        
        double cfc_score = out_unit->get_cfc(); 
        
        // Hunt for the LEAST utilized port
        if (cfc_score < min_cfc) { 
            min_cfc = cfc_score; 
            outport_dirn = port; 
            found_valid_port = true;
        }
    }

    // Safety Fallback: If port mapping fails, default to XY
    if (!found_valid_port) {
        return outportComputeXY(route, inport, inport_dirn);
    }

    return m_outports_dirn2idx[outport_dirn];
}

int RoutingUnit::outportComputeR2D2(RouteInfo route, int inport, PortDirection inport_dirn)
{
    // 1. Run the MPAD Sensor in parallel
    runMPADDetection(route, inport_dirn);

    // 2. Read the dynamic hardware flag from the Network
    int current_threat_id = m_router->get_net_ptr()->get_detected_trojan_id();

    // 3. OPTIMIZATION: Peace-Time Fast Path
    // If no Trojan has been detected YET (-1), bypass R2D2 and use baseline XY
    if (current_threat_id == -1) {
        return outportComputeXY(route, inport, inport_dirn); 
    }

    // 4. One-Time R2D2 Initialization (Runs only once after Alarm is triggered)
    if (!m_r2d2_initialized) {
        calculateR2D2Penalties();
    }

    PortDirection outport_dirn = "Unknown";
    int num_cols = m_router->get_net_ptr()->getNumCols();
    int my_id = m_router->get_id();
    int my_x = my_id % num_cols;
    int my_y = my_id / num_cols;
    
    int dest_id = route.dest_router;
    int dest_x = dest_id % num_cols;
    int dest_y = dest_id / num_cols;

    int x_hops = abs(dest_x - my_x);
    int y_hops = abs(dest_y - my_y);
    bool x_dirn = (dest_x >= my_x);
    bool y_dirn = (dest_y >= my_y);

    std::vector<PortDirection> valid_ports;


    if (x_hops > 0 && y_hops > 0) {
        if (x_dirn) { 
            if (my_x % 2 != 0 || my_x == dest_x) valid_ports.push_back(y_dirn ? "North" : "South");
            if (dest_x % 2 != 0 || x_hops != 1) valid_ports.push_back("East");
        } else { 
            valid_ports.push_back("West");
            if (my_x % 2 == 0) valid_ports.push_back(y_dirn ? "North" : "South");
        }
    } else if (x_hops > 0) {
        valid_ports.push_back(x_dirn ? "East" : "West");
    } else if (y_hops > 0) {
        valid_ports.push_back(y_dirn ? "North" : "South");
    }

    if (valid_ports.empty()) {
        return outportComputeXY(route, inport, inport_dirn);
    }



    int max_effective_score = -999999;
    bool found_valid_port = false;

    for (const auto& port : valid_ports) {
        if (m_outports_dirn2idx.find(port) == m_outports_dirn2idx.end()) continue;

        int hazard_penalty = m_r2d2_penalty_table[port];
        
        // CRITICAL FIX 1: The "continue" pruner is removed. 
        // The negative effective_score naturally pushes traffic away, but 
        // allows the router to step into the hazard if it is the absolute ONLY way out.

        int score = 0;
        int out_idx = m_outports_dirn2idx[port];
        OutputUnit* out_unit = m_router->getOutputUnit(out_idx);
        int vcs_per_vnet = m_router->get_vc_per_vnet();
        
        for (int vc = route.vnet * vcs_per_vnet; vc < (route.vnet + 1) * vcs_per_vnet; vc++) {
            score += out_unit->get_credit_count(vc);
        }

        int effective_score = score - hazard_penalty;
        
        if (effective_score > max_effective_score) { 
            max_effective_score = effective_score; 
            outport_dirn = port; 
            found_valid_port = true;
        }
    }

    if (found_valid_port) {
        return m_outports_dirn2idx[outport_dirn];
    }

    PortDirection failsafe_dir = "Unknown";
    if (x_hops > 0) {
        failsafe_dir = x_dirn ? "East" : "West";
    } else {
        failsafe_dir = y_dirn ? "North" : "South";
    }
    
    return m_outports_dirn2idx[failsafe_dir];
}


// ODD-EVEN
int
RoutingUnit::outportComputeODD_EVEN(RouteInfo route,
                              int inport,
                              PortDirection inport_dirn)
{
    PortDirection outport_dirn = "Unknown";
    int num_cols = m_router->get_net_ptr()->getNumCols();
    int my_id = m_router->get_id();
    int my_x = my_id % num_cols;
    int my_y = my_id / num_cols;
    int dest_id = route.dest_router;
    int dest_x = dest_id % num_cols;
    int dest_y = dest_id / num_cols;

    int x_hops = abs(dest_x - my_x);
    int y_hops = abs(dest_y - my_y);
    bool x_dirn = (dest_x >= my_x);
    bool y_dirn = (dest_y >= my_y);

    if (x_hops == 0) {
        outport_dirn = y_dirn ? "North" : "South";
    } else if (y_hops == 0) {
        outport_dirn = x_dirn ? "East" : "West";
    } else {
        std::vector<PortDirection> valid_ports;
        
        if (x_dirn) { 
            // Moving East: East->North and East->South forbidden at Even columns
            if (my_x % 2 != 0 || my_x == dest_x) {
                valid_ports.push_back(y_dirn ? "North" : "South");
            }
            if (dest_x % 2 != 0 || x_hops != 1) {
                valid_ports.push_back("East");
            }
        } else { 
            // Moving West: North->West and South->West forbidden at Odd columns
            valid_ports.push_back("West");
            if (my_x % 2 == 0) {
                valid_ports.push_back(y_dirn ? "North" : "South");
            }
        }

        // Adaptive Selection based on queue congestion
        int max_credits = -1;
        for (const auto& port : valid_ports) {
            int credits = getFreeCredits(port, route.vnet);
            if (credits > max_credits) {
                max_credits = credits;
                outport_dirn = port;
            }
        }
    }
    
    return m_outports_dirn2idx[outport_dirn];
}

// DyXY
int
RoutingUnit::outportComputeDyXY(RouteInfo route,
                                int inport,
                                PortDirection inport_dirn)
{
    PortDirection outport_dirn = "Unknown";
    int num_cols = m_router->get_net_ptr()->getNumCols();
    int my_id = m_router->get_id();
    int my_x = my_id % num_cols;
    int my_y = my_id / num_cols;
    int dest_id = route.dest_router;
    int dest_x = dest_id % num_cols;
    int dest_y = dest_id / num_cols;

    if (my_x == dest_x) {
        outport_dirn = (dest_y > my_y) ? "North" : "South";
    } else if (my_y == dest_y) {
        outport_dirn = (dest_x > my_x) ? "East" : "West";
    } else {
        // Diagonal Adaptive Evaluation
        PortDirection dir_x = (dest_x > my_x) ? "East" : "West";
        PortDirection dir_y = (dest_y > my_y) ? "North" : "South";

        int credits_x = getFreeCredits(dir_x, route.vnet);
        int credits_y = getFreeCredits(dir_y, route.vnet);

        if (credits_x > credits_y) {
            outport_dirn = dir_x;
        } else if (credits_y > credits_x) {
            outport_dirn = dir_y;
        } else {
            outport_dirn = dir_x; // Tie-breaker
        }
    }

    return m_outports_dirn2idx[outport_dirn];
}

// Template for implementing custom routing algorithm
// using port directions. (Example adaptive)
// int
// RoutingUnit::outportComputeCustom(RouteInfo route,
//                                  int inport,
//                                  PortDirection inport_dirn)
// {
//     panic("%s placeholder executed", __FUNCTION__);
// }

} // namespace garnet
} // namespace ruby
} // namespace gem5
