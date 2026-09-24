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


#ifndef __MEM_RUBY_NETWORK_GARNET_0_ROUTINGUNIT_HH__
#define __MEM_RUBY_NETWORK_GARNET_0_ROUTINGUNIT_HH__

#include "mem/ruby/common/Consumer.hh"
#include "mem/ruby/common/NetDest.hh"
#include "mem/ruby/network/garnet/CommonTypes.hh"
#include "mem/ruby/network/garnet/GarnetNetwork.hh"
#include "mem/ruby/network/garnet/flit.hh"

namespace gem5
{

namespace ruby
{

namespace garnet
{

class InputUnit;
class Router;

class RoutingUnit
{
  public:
    RoutingUnit(Router *router);
    int outportCompute(RouteInfo route,
                      int inport,
                      PortDirection inport_dirn);

    // Topology-agnostic Routing Table based routing (default)
    void addRoute(std::vector<NetDest>& routing_table_entry);
    void addWeight(int link_weight);

    // get output port from routing table
    int  lookupRoutingTable(int vnet, NetDest net_dest);

    // Topology-specific direction based routing
    void addInDirection(PortDirection inport_dirn, int inport);
    void addOutDirection(PortDirection outport_dirn, int outport);

    // Routing for Mesh
    int outportComputeXY(RouteInfo route,
                         int inport,
                         PortDirection inport_dirn);

    // Custom Routing Algorithm using Port Directions
    int outportComputeCustom(RouteInfo route,
                             int inport,
                             PortDirection inport_dirn);

    // Returns true if vnet is present in the vector
    // of vnets or if the vector supports all vnets.
    bool supportsVnet(int vnet, std::vector<int> sVnets);

    // Routing for ODD-EVEN
    int outportComputeODD_EVEN(RouteInfo route,
                         int inport,
                         PortDirection inport_dirn);
    
    // Routing for DyXY
    int outportComputeDyXY(RouteInfo route,
                         int inport,
                         PortDirection inport_dirn);
    
    int getFreeCredits(PortDirection dirn, int vnet);

    int outportComputeBOFAR(RouteInfo route, int inport, PortDirection inport_dirn);
    int getDownstreamBOCSum(PortDirection dirn);

    int outportComputeFVC(RouteInfo route, int inport, PortDirection inport_dirn);
    int outportComputeFON(RouteInfo route, int inport, PortDirection inport_dirn);
    int outportComputeNOP(RouteInfo route, int inport, PortDirection inport_dirn);
    
    int getDownstreamFluiditySum(PortDirection dirn);
    int getNOPScore(PortDirection dirn, RouteInfo route);
    // Add these alongside the other outportCompute functions:
    int outportComputeTRACKER(RouteInfo route,
                              int inport,
                              PortDirection inport_dirn);

    // Add the helper function to fetch the Cumulative Flit Count
    double getDownstreamCFC(PortDirection dirn);


  private:
    Router *m_router;

    // Routing Table
    std::vector<std::vector<NetDest>> m_routing_table;
    std::vector<int> m_weight_table;

    // Inport and Outport direction to idx maps
    std::map<PortDirection, int> m_inports_dirn2idx;
    std::map<int, PortDirection> m_inports_idx2dirn;
    std::map<int, PortDirection> m_outports_idx2dirn;
    std::map<PortDirection, int> m_outports_dirn2idx;

    // --- R2D2: REACTIVE REROUTING REGISTERS ---
    int m_trojan_id; 
    bool m_r2d2_initialized;
    std::map<PortDirection, int> m_r2d2_penalty_table; 
    
    void calculateR2D2Penalties();
    int outportComputeR2D2(RouteInfo route, int inport, PortDirection inport_dirn);


    
  // MPAD SENSOR REGISTERS 
    bool m_alarm_triggered;
    
    // Proof 1: Dest Entropy
    int m_last_dest;
    int m_dest_entropy_count;
    
    // Window Tracking (For Entropy Reset)
    gem5::Tick m_window_start;
    int m_packets_in_window;

    // Proof 3: Consecutive Burst Tracker
    gem5::Tick m_last_injection_cycle;
    int m_consecutive_injections;


    // The MPAD Engine
    void runMPADDetection(RouteInfo route, PortDirection inport_dirn);
    // ------------------------------------------
};

} // namespace garnet
} // namespace ruby
} // namespace gem5

#endif // __MEM_RUBY_NETWORK_GARNET_0_ROUTINGUNIT_HH__
