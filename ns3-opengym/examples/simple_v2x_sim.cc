/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Simple V2X OpenGym Simulation with optional SUMO/TraCI mobility replay
 */

#include "ns3/core-module.h"
#include "ns3/opengym-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SimpleV2X");

struct VehicleMetrics
{
  VehicleMetrics () : position (Vector (0.0, 0.0, 0.0)), speed (0.0), active (false) {}
  Vector position;
  double speed;
  bool active;
};

struct SumoVehicleState
{
  SumoVehicleState () : position (Vector (0.0, 0.0, 0.0)), speed (0.0), vehicleType ("passenger") {}
  Vector position;
  double speed;
  std::string vehicleType;
};

// Global variables
NodeContainer g_nodes;
uint32_t g_nodeNum = 0;
uint32_t g_currentStep = 0;
double g_envStepTime = 0.1;

bool g_useSumoMobility = false;
std::vector<std::map<uint32_t, SumoVehicleState>> g_sumoTrajectory;
std::unordered_map<std::string, uint32_t> g_sumoIdToNodeIndex;
std::unordered_map<uint32_t, SumoVehicleState> g_currentSumoState;
uint32_t g_sumoVehicleCount = 0;
double g_sumoMaxTime = 0.0;

std::vector<VehicleMetrics> g_vehicleMetrics;

std::string
ExtractAttribute (const std::string& line, const std::string& key)
{
  std::string token = key + "=\"";
  auto pos = line.find (token);
  if (pos == std::string::npos)
    {
      return "";
    }
  pos += token.size ();
  auto end = line.find ("\"", pos);
  if (end == std::string::npos)
    {
      return "";
    }
  return line.substr (pos, end - pos);
}

bool
LoadSumoTrajectory (const std::string& filePath)
{
  std::ifstream in (filePath.c_str ());
  if (!in.is_open ())
    {
      NS_LOG_UNCOND ("[SUMO] Failed to open mobility trace: " << filePath);
      return false;
    }

  std::string line;
  double currentTime = 0.0;
  g_sumoTrajectory.clear ();
  g_sumoIdToNodeIndex.clear ();
  g_sumoVehicleCount = 0;

  while (std::getline (in, line))
    {
      if (line.find ("<timestep") != std::string::npos)
        {
          std::string timeStr = ExtractAttribute (line, "time");
          if (!timeStr.empty ())
            {
              currentTime = std::stod (timeStr);
            }
        }
      else if (line.find ("<vehicle") != std::string::npos)
        {
          std::string idStr = ExtractAttribute (line, "id");
          std::string xStr = ExtractAttribute (line, "x");
          std::string yStr = ExtractAttribute (line, "y");
          std::string speedStr = ExtractAttribute (line, "speed");
          std::string typeStr = ExtractAttribute (line, "type");

          if (idStr.empty () || xStr.empty () || yStr.empty ())
            {
              continue;
            }

          double x = std::stod (xStr);
          double y = std::stod (yStr);
          double speed = speedStr.empty () ? 0.0 : std::stod (speedStr);
          std::string vehicleType = typeStr.empty () ? "passenger" : typeStr;

          if (g_envStepTime <= 0.0)
            {
              g_envStepTime = 0.1;
            }
          uint32_t timestepIndex = static_cast<uint32_t> (std::round (currentTime / g_envStepTime));
          if (g_sumoTrajectory.size () <= timestepIndex)
            {
              g_sumoTrajectory.resize (timestepIndex + 1);
            }

          uint32_t nodeIndex;
          auto it = g_sumoIdToNodeIndex.find (idStr);
          if (it == g_sumoIdToNodeIndex.end ())
            {
              nodeIndex = g_sumoIdToNodeIndex.size ();
              g_sumoIdToNodeIndex[idStr] = nodeIndex;
            }
          else
            {
              nodeIndex = it->second;
            }

          SumoVehicleState state;
          state.position = Vector (x, y, 0.0);
          state.speed = speed;
          state.vehicleType = vehicleType;

          g_sumoTrajectory[timestepIndex][nodeIndex] = state;
        }
    }

  g_sumoVehicleCount = g_sumoIdToNodeIndex.size ();
  if (!g_sumoTrajectory.empty ())
    {
      g_sumoMaxTime = g_envStepTime * (g_sumoTrajectory.size () - 1);
    }

  NS_LOG_UNCOND ("[SUMO] Loaded mobility trace: " << g_sumoVehicleCount << " vehicles, "
                 << g_sumoTrajectory.size () << " timesteps from " << filePath);

  return (g_sumoVehicleCount > 0 && !g_sumoTrajectory.empty ());
}

void
InitializeVehicleMetrics ()
{
  g_vehicleMetrics.clear ();
  g_vehicleMetrics.resize (g_nodes.GetN ());
}

void
UpdateVehicleMetrics ()
{
  if (g_vehicleMetrics.size () != g_nodes.GetN ())
    {
      InitializeVehicleMetrics ();
    }

  if (g_useSumoMobility)
    {
      for (uint32_t i = 0; i < g_vehicleMetrics.size (); ++i)
        {
          auto it = g_currentSumoState.find (i);
          if (it != g_currentSumoState.end ())
            {
              g_vehicleMetrics[i].position = it->second.position;
              g_vehicleMetrics[i].speed = it->second.speed;
              g_vehicleMetrics[i].active = true;
            }
          else
            {
              g_vehicleMetrics[i].position = Vector (0.0, 0.0, 0.0);
              g_vehicleMetrics[i].speed = 0.0;
              g_vehicleMetrics[i].active = false;
            }
        }
    }
  else
    {
      for (uint32_t i = 0; i < g_nodes.GetN (); ++i)
        {
          Ptr<Node> node = g_nodes.Get (i);
          Ptr<MobilityModel> mobility = node->GetObject<MobilityModel> ();

          if (mobility)
            {
              Vector pos = mobility->GetPosition ();
              Vector vel = mobility->GetVelocity ();
              double speed = std::sqrt (vel.x * vel.x + vel.y * vel.y);

              g_vehicleMetrics[i].position = pos;
              g_vehicleMetrics[i].speed = speed;
              g_vehicleMetrics[i].active = true;
            }
          else
            {
              g_vehicleMetrics[i].position = Vector (0.0, 0.0, 0.0);
              g_vehicleMetrics[i].speed = 0.0;
              g_vehicleMetrics[i].active = false;
            }
        }
    }
}

void
ApplySumoMobility (uint32_t timestep)
{
  if (!g_useSumoMobility || g_sumoTrajectory.empty ())
    {
      UpdateVehicleMetrics ();
      return;
    }

  uint32_t clampedStep = std::min (timestep, static_cast<uint32_t> (g_sumoTrajectory.size () - 1));
  
  // 디버깅: 처음 10 스텝만 로그
  static int logCount = 0;
  if (logCount < 10)
    {
      NS_LOG_UNCOND ("🔍 ApplySumoMobility: timestep=" << timestep << ", clampedStep=" << clampedStep 
                     << ", trajectorySize=" << g_sumoTrajectory.size ());
      logCount++;
    }
  
  const auto& states = g_sumoTrajectory[clampedStep];
  
  // 디버깅: 첫 번째 차량의 위치 로그
  if (logCount <= 10 && !states.empty())
    {
      const auto& firstVehicle = states.begin()->second;
      NS_LOG_UNCOND ("   → Vehicle 0 position: (" << firstVehicle.position.x 
                     << ", " << firstVehicle.position.y << "), speed=" << firstVehicle.speed);
    }

  g_currentSumoState.clear ();

  for (const auto& entry : states)
    {
      uint32_t nodeIndex = entry.first;
      if (nodeIndex >= g_nodes.GetN ())
        {
          continue;
        }

      Ptr<Node> node = g_nodes.Get (nodeIndex);
      Ptr<MobilityModel> mobility = node->GetObject<MobilityModel> ();
      Ptr<ConstantPositionMobilityModel> constant = DynamicCast<ConstantPositionMobilityModel> (mobility);

      if (!constant)
        {
          constant = CreateObject<ConstantPositionMobilityModel> ();
          node->AggregateObject (constant);
        }

      constant->SetPosition (entry.second.position);
      g_currentSumoState[nodeIndex] = entry.second;
    }

  UpdateVehicleMetrics ();
}

Ptr<OpenGymSpace>
MyGetObservationSpace (void)
{
  uint32_t obsSize = 4 + (g_nodeNum * 3);
  float low = -1000.0;
  float high = 1000.0;
  std::vector<uint32_t> shape = {obsSize};
  std::string dtype = TypeNameGet<float> ();
  Ptr<OpenGymBoxSpace> space = CreateObject<OpenGymBoxSpace> (low, high, shape, dtype);
  NS_LOG_UNCOND ("MyGetObservationSpace: " << space);
  return space;
}

Ptr<OpenGymSpace>
MyGetActionSpace (void)
{
  uint32_t actionNum = 4;
  float low = 0.0;
  float high = 10.0;
  std::vector<uint32_t> shape = {actionNum};
  std::string dtype = TypeNameGet<float> ();
  Ptr<OpenGymBoxSpace> space = CreateObject<OpenGymBoxSpace> (low, high, shape, dtype);
  NS_LOG_UNCOND ("MyGetActionSpace: " << space);
  return space;
}

bool
MyGetGameOver (void)
{
  bool isGameOver = (g_currentStep >= 1000);
  NS_LOG_UNCOND ("MyGetGameOver: " << isGameOver);
  return isGameOver;
}

Ptr<OpenGymDataContainer>
MyGetObservation (void)
{
  NS_LOG_UNCOND ("MyGetObservation: Step " << g_currentStep);

  // ✅ 수정: observation 직전에 최신 SUMO 상태 적용
  if (g_useSumoMobility)
    {
      ApplySumoMobility (g_currentStep);
    }

  uint32_t obsSize = 4 + (g_nodeNum * 3);
  std::vector<uint32_t> shape = {obsSize};
  Ptr<OpenGymBoxContainer<float>> box = CreateObject<OpenGymBoxContainer<float>> (shape);

  uint32_t activeNodes = 0;
  double totalSpeed = 0.0;
  double avgX = 0.0;
  double avgY = 0.0;

  for (const auto& metrics : g_vehicleMetrics)
    {
      if (metrics.active)
        {
          activeNodes++;
          totalSpeed += metrics.speed;
          avgX += metrics.position.x;
          avgY += metrics.position.y;
        }
    }

  double avgSpeed = (activeNodes > 0) ? totalSpeed / activeNodes : 0.0;
  double avgPosX = (activeNodes > 0) ? avgX / activeNodes : 0.0;
  double avgPosY = (activeNodes > 0) ? avgY / activeNodes : 0.0;

  box->AddValue (static_cast<float> (activeNodes));
  box->AddValue (static_cast<float> (avgSpeed));
  box->AddValue (static_cast<float> (avgPosX));
  box->AddValue (static_cast<float> (avgPosY));

  for (uint32_t i = 0; i < g_nodeNum; ++i)
    {
      if (i < g_vehicleMetrics.size () && g_vehicleMetrics[i].active)
        {
          const auto& metrics = g_vehicleMetrics[i];
          box->AddValue (static_cast<float> (metrics.position.x));
          box->AddValue (static_cast<float> (metrics.position.y));
          box->AddValue (static_cast<float> (metrics.speed));
        }
      else
        {
          box->AddValue (0.0f);
          box->AddValue (0.0f);
          box->AddValue (0.0f);
        }
    }

  NS_LOG_UNCOND ("MyGetObservation: Active vehicles=" << activeNodes << " avg_speed=" << avgSpeed);
  return box;
}

float
MyGetReward (void)
{
  uint32_t activeNodes = 0;
  for (const auto& metrics : g_vehicleMetrics)
    {
      if (metrics.active)
        {
          activeNodes++;
        }
    }

  float reward = static_cast<float> (g_currentStep * 0.05 + activeNodes * 0.1);
  NS_LOG_UNCOND ("MyGetReward: " << reward << " (active=" << activeNodes << ")");
  return reward;
}

std::string
MyGetExtraInfo (void)
{
  std::ostringstream info;
  info << "step:" << g_currentStep << ";vehicles:" << g_vehicleMetrics.size ();
  NS_LOG_UNCOND ("MyGetExtraInfo: " << info.str ());
  return info.str ();
}

bool
MyExecuteActions (Ptr<OpenGymDataContainer> action)
{
  NS_LOG_UNCOND ("MyExecuteActions: Step " << g_currentStep
                 << " at sim time " << Simulator::Now ().GetSeconds ());
  NS_LOG_UNCOND ("MyExecuteActions: Received action: " << action);

  g_currentStep++;
  NS_LOG_UNCOND ("MyExecuteActions: Moving to step " << g_currentStep);

  // ✅ 수정: SUMO 상태 업데이트는 MyGetObservation에서 처리

  return true;
}

void
ScheduleNextStateRead (double envStepTime, Ptr<OpenGymInterface> openGym)
{
  NS_LOG_UNCOND ("ScheduleNextStateRead: Sim time=" << Simulator::Now ().GetSeconds () << "s");

  // ✅ 수정: SUMO 상태는 MyExecuteActions에서 이미 업데이트됨
  if (!g_useSumoMobility)
    {
      UpdateVehicleMetrics ();
    }

  Simulator::Schedule (Seconds (envStepTime), &ScheduleNextStateRead, envStepTime, openGym);
  openGym->NotifyCurrentState ();
}

int
main (int argc, char* argv[])
{
  double simulationTime = 30.0; // seconds
  uint32_t openGymPort = 5555;
  double envStepTime = 0.1;     // seconds
  std::string sumoTracePath = "";

  CommandLine cmd;
  cmd.AddValue ("openGymPort", "Port number for OpenGym env. Default: 5555", openGymPort);
  cmd.AddValue ("simTime", "Simulation time", simulationTime);
  cmd.AddValue ("sumoTrace", "Path to SUMO FCD mobility trace (.xml)", sumoTracePath);
  cmd.Parse (argc, argv);

  g_envStepTime = envStepTime;

  if (!sumoTracePath.empty ())
    {
      g_useSumoMobility = LoadSumoTrajectory (sumoTracePath);
      if (g_useSumoMobility)
        {
          g_nodeNum = g_sumoVehicleCount;
        }
    }

  if (!g_useSumoMobility)
    {
      g_nodeNum = 10; // default when SUMO trace is not provided
    }

  NS_LOG_UNCOND ("=== Simple V2X OpenGym Simulation ===");
  NS_LOG_UNCOND ("Simulation time: " << simulationTime << "s");
  NS_LOG_UNCOND ("OpenGym port: " << openGymPort);
  NS_LOG_UNCOND ("Environment step time: " << envStepTime << "s");
  NS_LOG_UNCOND ("SUMO mobility enabled: " << (g_useSumoMobility ? "yes" : "no"));
  NS_LOG_UNCOND ("Num vehicles: " << g_nodeNum);

  g_nodes.Create (g_nodeNum);
  InitializeVehicleMetrics ();
  NS_LOG_UNCOND ("Created " << g_nodeNum << " vehicle nodes");

  if (g_useSumoMobility)
    {
      MobilityHelper mobility;
      mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
      mobility.Install (g_nodes);
      NS_LOG_UNCOND ("Installed SUMO-driven constant position mobility models");

      ApplySumoMobility (0);
    }
  else
    {
      MobilityHelper mobility;
      mobility.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator",
                                     "X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=500.0]"),
                                     "Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=500.0]"));

      mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                                 "Bounds", RectangleValue (Rectangle (0, 500, 0, 500)),
                                 "Speed", StringValue ("ns3::UniformRandomVariable[Min=10.0|Max=30.0]"),
                                 "Direction", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=6.28]"));
      mobility.Install (g_nodes);
      NS_LOG_UNCOND ("Installed random walk mobility models");

      UpdateVehicleMetrics ();
    }

  Ptr<OpenGymInterface> openGym = CreateObject<OpenGymInterface> (openGymPort);
  openGym->SetGetActionSpaceCb (MakeCallback (&MyGetActionSpace));
  openGym->SetGetObservationSpaceCb (MakeCallback (&MyGetObservationSpace));
  openGym->SetGetGameOverCb (MakeCallback (&MyGetGameOver));
  openGym->SetGetObservationCb (MakeCallback (&MyGetObservation));
  openGym->SetGetRewardCb (MakeCallback (&MyGetReward));
  openGym->SetGetExtraInfoCb (MakeCallback (&MyGetExtraInfo));
  openGym->SetExecuteActionsCb (MakeCallback (&MyExecuteActions));
  NS_LOG_UNCOND ("OpenGym callbacks configured");

  // ✅ 수정: 초기 상태(step 0) 준비
  if (g_useSumoMobility)
    {
      ApplySumoMobility (0);
    }
  else
    {
      UpdateVehicleMetrics ();
    }

  Simulator::Schedule (Seconds (envStepTime), &ScheduleNextStateRead, envStepTime, openGym);

  NS_LOG_UNCOND ("=== Starting Simple V2X Simulation ===");
  NS_LOG_UNCOND ("Connecting to Python OpenGym server on port " << openGymPort << "...");
  openGym->NotifyCurrentState ();

  Simulator::Stop (Seconds (simulationTime));
  Simulator::Run ();

  NS_LOG_UNCOND ("=== Simulation Complete ===");
  openGym->NotifySimulationEnd ();
  Simulator::Destroy ();

  return 0;
}