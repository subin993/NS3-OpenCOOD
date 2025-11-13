/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Training-focused V2X OpenGym Simulation with configurable SUMO replay support
 */

#include "ns3/core-module.h"
#include "ns3/opengym-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/random-variable-stream.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TrainingV2XDatasetSim");

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

// Global state
NodeContainer g_nodes;
uint32_t g_nodeNum = 0;
uint32_t g_currentStep = 0;
double g_envStepTime = 0.1;

double g_simulationTimeLimit = 0.0;
uint32_t g_maxSteps = 0;
bool g_loopSumoTrajectory = false;
uint32_t g_logInterval = 10; // 0 disables per-step logs, 1 logs every step

bool g_useSumoMobility = false;
std::vector<std::map<uint32_t, SumoVehicleState>> g_sumoTrajectory;
std::unordered_map<std::string, uint32_t> g_sumoIdToNodeIndex;
std::unordered_map<uint32_t, SumoVehicleState> g_currentSumoState;
uint32_t g_sumoVehicleCount = 0;
double g_sumoMaxTime = 0.0;

std::vector<VehicleMetrics> g_vehicleMetrics;

inline void
LogStepMessage (const std::string& message, bool force = false)
{
  if (force)
    {
      NS_LOG_UNCOND (message);
      return;
    }

  if (g_logInterval == 0)
    {
      return;
    }

  if (g_logInterval == 1 || (g_logInterval > 1 && (g_currentStep % g_logInterval == 0)))
    {
      NS_LOG_UNCOND (message);
    }
}

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
                                               << g_sumoTrajectory.size () << " timesteps from "
                                               << filePath);

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

  uint32_t effectiveSize = static_cast<uint32_t> (g_sumoTrajectory.size ());
  uint32_t safeIndex = std::min (timestep, effectiveSize - 1);
  if (g_loopSumoTrajectory && effectiveSize > 0)
    {
      safeIndex = timestep % effectiveSize;
    }

  const auto& states = g_sumoTrajectory[safeIndex];

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
  float low = -10000.0f;
  float high = 10000.0f;
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
  float low = 0.0f;
  float high = 10.0f;
  std::vector<uint32_t> shape = {actionNum};
  std::string dtype = TypeNameGet<float> ();
  Ptr<OpenGymBoxSpace> space = CreateObject<OpenGymBoxSpace> (low, high, shape, dtype);
  NS_LOG_UNCOND ("MyGetActionSpace: " << space);
  return space;
}

bool
MyGetGameOver (void)
{
  bool stepLimitReached = (g_maxSteps > 0 && g_currentStep >= g_maxSteps);
  bool timeLimitReached = (g_simulationTimeLimit > 0.0 && Simulator::Now ().GetSeconds () >= g_simulationTimeLimit);
  bool isGameOver = stepLimitReached || timeLimitReached;
  LogStepMessage ("MyGetGameOver: " + std::to_string (isGameOver), false);
  return isGameOver;
}

Ptr<OpenGymDataContainer>
MyGetObservation (void)
{
  LogStepMessage ("MyGetObservation: step=" + std::to_string (g_currentStep), false);

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

  float reward = static_cast<float> (activeNodes * 0.1);
  LogStepMessage ("MyGetReward: " + std::to_string (reward), false);
  return reward;
}

std::string
MyGetExtraInfo (void)
{
  std::ostringstream info;
  info << "step:" << g_currentStep << ";vehicles:" << g_vehicleMetrics.size ();
  LogStepMessage ("MyGetExtraInfo: " + info.str (), false);
  return info.str ();
}

bool
MyExecuteActions (Ptr<OpenGymDataContainer> action)
{
  if (g_maxSteps > 0 && g_currentStep >= g_maxSteps)
    {
      LogStepMessage ("MyExecuteActions: Step limit reached, ignoring actions", true);
      return false;
    }

  LogStepMessage ("MyExecuteActions: step=" + std::to_string (g_currentStep)
                     + " sim=" + std::to_string (Simulator::Now ().GetSeconds ()), false);

  g_currentStep++;
  return true;
}

void
ScheduleNextStateRead (double envStepTime, Ptr<OpenGymInterface> openGym)
{
  if (g_useSumoMobility)
    {
      ApplySumoMobility (g_currentStep);
    }
  else
    {
      UpdateVehicleMetrics ();
    }

  openGym->NotifyCurrentState ();

  if (g_maxSteps > 0 && g_currentStep >= g_maxSteps)
    {
      LogStepMessage ("ScheduleNextStateRead: Reached max steps, stopping schedule", true);
      return;
    }

  Simulator::Schedule (Seconds (envStepTime), &ScheduleNextStateRead, envStepTime, openGym);
}

int
main (int argc, char* argv[])
{
  double simulationTime = 120.0; // seconds
  uint32_t openGymPort = 5556;
  double envStepTime = 0.1; // seconds
  std::string sumoTracePath = "";
  uint32_t vehicleCount = 40;
  bool loopSumo = true;
  uint32_t maxSteps = 0; // unlimited by default
  uint32_t logInterval = 10;
  double areaMin = 0.0;
  double areaMax = 800.0;
  double minSpeed = 5.0;
  double maxSpeedValue = 25.0;
  uint32_t rngSeed = 1;
  uint32_t rngRun = 1;

  CommandLine cmd;
  cmd.AddValue ("openGymPort", "Port number for OpenGym env. Default: 5556", openGymPort);
  cmd.AddValue ("simTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("envStep", "Environment step time", envStepTime);
  cmd.AddValue ("sumoTrace", "Path to SUMO FCD mobility trace (.xml)", sumoTracePath);
  cmd.AddValue ("vehicleCount", "Number of vehicles when SUMO is not used", vehicleCount);
  cmd.AddValue ("loopSumo", "Loop SUMO trajectory when simulation exceeds trace length", loopSumo);
  cmd.AddValue ("maxSteps", "Maximum OpenGym steps before terminating (0=unbounded)", maxSteps);
  cmd.AddValue ("logInterval", "Log every N steps (0 disables per-step logging, 1 logs every step)",
                logInterval);
  cmd.AddValue ("areaMin", "Minimum coordinate for default random area", areaMin);
  cmd.AddValue ("areaMax", "Maximum coordinate for default random area", areaMax);
  cmd.AddValue ("minSpeed", "Minimum speed for random-walk mobility", minSpeed);
  cmd.AddValue ("maxSpeed", "Maximum speed for random-walk mobility", maxSpeedValue);
  cmd.AddValue ("seed", "RNG seed", rngSeed);
  cmd.AddValue ("run", "RNG run number", rngRun);
  cmd.Parse (argc, argv);

  g_envStepTime = envStepTime;
  g_maxSteps = maxSteps;
  g_loopSumoTrajectory = loopSumo;
  g_logInterval = logInterval;

  g_simulationTimeLimit = simulationTime;

  if (g_maxSteps > 0)
    {
      double minimumSim = envStepTime * (g_maxSteps + 5);
      if (simulationTime < minimumSim)
        {
          NS_LOG_UNCOND ("[Config] Extending simulation time to " << minimumSim
                                                                   << "s to cover requested steps");
          simulationTime = minimumSim;
          g_simulationTimeLimit = simulationTime;
        }
    }

  RngSeedManager::SetSeed (rngSeed);
  RngSeedManager::SetRun (rngRun);

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
      if (vehicleCount == 0)
        {
          vehicleCount = 10;
        }
      g_nodeNum = vehicleCount;
    }

  NS_LOG_UNCOND ("=== Training V2X Dataset Simulation ===");
  NS_LOG_UNCOND ("Simulation time: " << simulationTime << "s");
  NS_LOG_UNCOND ("OpenGym port: " << openGymPort);
  NS_LOG_UNCOND ("Environment step time: " << envStepTime << "s");
  NS_LOG_UNCOND ("SUMO mobility enabled: " << (g_useSumoMobility ? "yes" : "no"));
  if (g_useSumoMobility)
    {
      NS_LOG_UNCOND ("Loop SUMO: " << (g_loopSumoTrajectory ? "yes" : "no"));
    }
  NS_LOG_UNCOND ("Vehicle count: " << g_nodeNum);
  NS_LOG_UNCOND ("Max steps: " << (g_maxSteps > 0 ? std::to_string (g_maxSteps) : std::string ("unbounded")));
  NS_LOG_UNCOND ("Log interval: " << logInterval);

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
      std::ostringstream speedStr;
      speedStr << "ns3::UniformRandomVariable[Min=" << minSpeed << "|Max=" << maxSpeedValue << "]";

      MobilityHelper mobility;
      mobility.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator",
                                     "X", StringValue ("ns3::UniformRandomVariable[Min="
                                                              + std::to_string (areaMin) + "|Max="
                                                              + std::to_string (areaMax) + "]"),
                                     "Y", StringValue ("ns3::UniformRandomVariable[Min="
                                                              + std::to_string (areaMin) + "|Max="
                                                              + std::to_string (areaMax) + "]"));

      mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                                 "Bounds", RectangleValue (Rectangle (areaMin, areaMax, areaMin, areaMax)),
                                 "Speed", StringValue (speedStr.str ()));
      mobility.Install (g_nodes);
      NS_LOG_UNCOND ("Installed random walk mobility models within [" << areaMin << ", " << areaMax
                                                                       << "]");

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

  Simulator::Schedule (Seconds (envStepTime), &ScheduleNextStateRead, envStepTime, openGym);

  NS_LOG_UNCOND ("=== Starting Training V2X Simulation ===");
  openGym->NotifyCurrentState ();

  Simulator::Stop (Seconds (simulationTime));
  Simulator::Run ();

  NS_LOG_UNCOND ("=== Simulation Complete ===");
  openGym->NotifySimulationEnd ();
  Simulator::Destroy ();

  return 0;
}
