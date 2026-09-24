/*
 * Copyright (c) 2020 Inria
 * Copyright (c) 2016 Georgia Institute of Technology
 * Copyright (c) 2008 Princeton University
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


#include "mem/ruby/network/garnet/VirtualChannel.hh"
#include <fstream> // dhruv -> to write in custom csv file. 
#include "base/statistics.hh" // dhruv

namespace gem5
{

namespace ruby
{

namespace garnet
{

VirtualChannel::VirtualChannel()
  : inputBuffer(), m_vc_state(IDLE_, Tick(0)), m_output_port(-1),
    m_enqueue_time(INFINITE_), m_output_vc(-1), 
    // added initialisers. dhruv. 
    last_state_change_tick(0),
    idle_ticks(0),
    active_ticks(0),
    allocation_count(0)
{
    // NEW: Register a callback to dump our formatted stats exactly when stats.txt is generated       -> dhruv.
    gem5::statistics::registerDumpCallback([this]() {
        std::ofstream vc_out;
        // Changing to .txt since we are adding descriptive text
        vc_out.open("m5out/vc_summary.txt", std::ios_base::app); 
        
        if (vc_out.is_open()) {
            Tick total_ticks = this->idle_ticks + this->active_ticks;
            
            // Only print if the VC was actually instantiated and tracked
            if (total_ticks > 0) { 
                double idle_percent = ((double)this->idle_ticks / total_ticks) * 100.0;
                
                vc_out << "VC_Ptr: " << this 
                       << " | Idle_Ticks: " << this->idle_ticks 
                       << " | Active_Ticks: " << this->active_ticks 
                       << " | Allocations: " << this->allocation_count 
                       << " | Idle_Percent: " << idle_percent << "%\n";
            }
            vc_out.close();
        }
    });
}

void
VirtualChannel::set_idle(Tick curTime)
{

    Tick time_spent_active = curTime - last_state_change_tick; // dhruv
    active_ticks += time_spent_active;
    // totalActiveTicks += time_spent_active;
    last_state_change_tick = curTime;

    m_vc_state.first = IDLE_;
    m_vc_state.second = curTime;
    m_enqueue_time = Tick(INFINITE_);
    m_output_port = -1;
    m_output_vc = -1;
}

void
VirtualChannel::set_active(Tick curTime)
{
    Tick time_spent_idle = curTime - last_state_change_tick; // dhruv
    idle_ticks += time_spent_idle;
    // totalIdleTicks += time_spent_idle; 
    allocation_count++;
    last_state_change_tick = curTime;

    m_vc_state.first = ACTIVE_;
    m_vc_state.second = curTime;
    m_enqueue_time = curTime;
}

bool
VirtualChannel::need_stage(flit_stage stage, Tick time)
{
    if (inputBuffer.isReady(time)) {
        assert(m_vc_state.first == ACTIVE_ && m_vc_state.second <= time);
        flit *t_flit = inputBuffer.peekTopFlit();
        return(t_flit->is_stage(stage, time));
    }
    return false;
}

bool
VirtualChannel::functionalRead(Packet *pkt, WriteMask &mask)
{
    return inputBuffer.functionalRead(pkt, mask);
}

uint32_t
VirtualChannel::functionalWrite(Packet *pkt)
{
    return inputBuffer.functionalWrite(pkt);
}

VirtualChannel::~VirtualChannel()
{
    // std::ofstream vc_out;
    // vc_out.open("m5out/vc_summary.csv", std::ios_base::app); 
    // if (vc_out.is_open()) {
    //     if (allocation_count > 0 || idle_ticks > 0) { 
    //         vc_out << "VC_Instance_" << this << "," 
    //                << idle_ticks << "," 
    //                << active_ticks << "," 
    //                << allocation_count << "\n";
    //     }
    //     vc_out.close();
    // }
}

} // namespace garnet
} // namespace ruby
} // namespace gem5
