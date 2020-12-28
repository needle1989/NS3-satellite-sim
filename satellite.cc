/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <iostream>
#include <fstream>
#include <string>
#include <cassert>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/netanim-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("satellite simulation");

int 
main (int argc, char *argv[])
{
  NS_LOG_INFO ("Create nodes.");
  NodeContainer lefts,rights,routers,nodes;
  lefts.Create(1);
  rights.Create(1);
  routers.Create(2);
  nodes = NodeContainer(lefts,rights,routers);
  
  NodeContainer serv = NodeContainer(lefts,routers.Get(0));
  NodeContainer cli = NodeContainer(rights,routers.Get(1));
  NodeContainer sat = NodeContainer(routers.Get(0),routers.Get(1));

  NS_LOG_INFO ("Create channels.");
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute("DataRate",StringValue("10Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue("1ns"));
  NetDeviceContainer serv1 = p2p.Install(serv);
  NetDeviceContainer cli1 = p2p.Install(cli);
  NetDeviceContainer sat1 = p2p.Install(sat);
  
  
  NS_LOG_INFO ("Create stacks.");
  InternetStackHelper internet;
  internet.Install(nodes);
  
  NS_LOG_INFO ("Assign IP Addresses.");
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer iserv = ipv4.Assign (serv1);
  ipv4.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer icli = ipv4.Assign (cli1);
  ipv4.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer isat = ipv4.Assign (sat1);
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  NS_LOG_INFO ("Create Applications.");
  
  Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpHybla::GetTypeId()));
  
  uint16_t port = 9;
  OnOffHelper onoff ("ns3::TcpSocketFactory",
                     Address (InetSocketAddress (icli.GetAddress (0), port)));
  onoff.SetConstantRate (DataRate ("2Mbps"));
  ApplicationContainer apps = onoff.Install (lefts.Get (0));
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (10.0));
  
  PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         Address (InetSocketAddress (Ipv4Address::GetAny (), port)));
  apps = sink.Install (rights.Get (0));
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (10.0));

  NS_LOG_INFO ("Create Error Model.");
  Ptr<RateErrorModel> em = CreateObjectWithAttributes<RateErrorModel> ("ErrorRate",DoubleValue(0.05),"ErrorUnit",EnumValue(RateErrorModel::ERROR_UNIT_PACKET));
  sat1.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));

  NS_LOG_INFO ("Run Simulation.");
  AnimationInterface anim ("sat.xml");
  anim.SetConstantPosition (lefts.Get (0), 20, 40);
  anim.SetConstantPosition (rights.Get (0), 80, 40);
  anim.SetConstantPosition (routers.Get (0), 40, 40);
  anim.SetConstantPosition (routers.Get (1), 60, 40);
  for (uint32_t i = 0; i < routers.GetN (); ++i)
     {
       anim.UpdateNodeDescription (routers.Get (i), "ROUTER");
       anim.UpdateNodeColor (routers.Get (i), 255, 0, 0);
     }
  anim.UpdateNodeDescription(lefts.Get(0),"SERVER");
  anim.UpdateNodeDescription(rights.Get(0),"CLIENT");
  Simulator::Run ();
  
  NS_LOG_INFO ("Done.");


  Simulator::Destroy ();
  return 0;
  
  
  
}

