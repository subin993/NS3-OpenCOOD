#!/usr/bin/env python3
"""
NS-3 Guided Proven Data Augmentation
NS-3 시뮬레이션 환경의 실시간 텔레메트리를 받아
성능이 검증된 원본 데이터(2021_09_11_00_33_16_temp)를 증강

기존 파이프라인 구조 유지:
1. NS-3 simple-v2x-sim을 먼저 실행 (SUMO trace 기반)
2. Python이 ns3gym으로 연결
3. 매 타임스텝마다 NS-3에서 차량 상태 수신
4. 원본 데이터에 NS-3 델타를 적용하여 증강
"""

import os
import sys
import yaml
import time
import logging
import numpy as np
import open3d as o3d
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Optional
import shutil

# Fix numpy compatibility
if not hasattr(np, 'float'):
    np.float = np.float64
if not hasattr(np, 'int'):
    np.int = np.int_
if not hasattr(np, 'bool'):
    np.bool = np.bool_

# NS3-Gym import
try:
    from ns3gym.ns3env import Ns3Env
    NS3_AVAILABLE = True
except ImportError:
    NS3_AVAILABLE = False
    logging.warning("ns3gym not available")


class NS3GuidedProvenDataAugmentor:
    """NS-3 텔레메트리 기반 proven 데이터 증강기"""
    
    def __init__(
        self,
        source_scenario: str,
        output_dir: str,
        scenario_name: str,
        ns3_port: int = 5555,
        ns3_sim_time: float = 120.0,  # NS-3 시뮬레이션 시간 (초)
        ns3_step_time: float = 0.1,   # NS-3 step 간격 (초)
        lidar_noise_std: float = 0.03,
        copy_images: bool = True
    ):
        self.source_scenario = Path(source_scenario)
        self.output_dir = Path(output_dir)
        self.scenario_name = scenario_name
        self.ns3_port = ns3_port
        self.ns3_sim_time = ns3_sim_time
        self.ns3_step_time = ns3_step_time
        self.lidar_noise_std = lidar_noise_std
        self.copy_images = copy_images
        
        self.output_path = self.output_dir / scenario_name
        self.setup_logging()
        
        # NS-3 연결
        self.ns3_env: Optional[Ns3Env] = None
        self.connected = False
        
        # 차량 IDs (원본 시나리오에서 추출)
        self.vehicle_ids: List[str] = []
        
        # NS-3 초기 기준점 (첫 프레임 상태 저장)
        self.ns3_initial_states: Dict[str, Dict[str, float]] = {}
        
        # NS-3 시뮬레이션 기반 예상 프레임 수 계산
        self.expected_ns3_frames = int(ns3_sim_time / ns3_step_time)
        
        self.logger.info(f"NS-3 Guided Augmentor 초기화: {scenario_name}")
        self.logger.info(f"  Source: {self.source_scenario}")
        self.logger.info(f"  Output: {self.output_path}")
        self.logger.info(f"  NS-3 Port: {self.ns3_port}")
        self.logger.info(f"  NS-3 Simulation: {ns3_sim_time}s @ {ns3_step_time}s/step = {self.expected_ns3_frames} frames")
    
    def setup_logging(self):
        """로깅 설정"""
        log_file = f"ns3_proven_aug_{datetime.now().strftime('%Y%m%d_%H%M%S')}.log"
        logging.basicConfig(
            level=logging.INFO,
            format='%(asctime)s - %(levelname)s - %(message)s',
            handlers=[
                logging.FileHandler(log_file),
                logging.StreamHandler()
            ]
        )
        self.logger = logging.getLogger(__name__)
    
    def connect_to_ns3(self) -> bool:
        """NS-3 시뮬레이션에 연결 (기존 working 방식과 동일)"""
        if not NS3_AVAILABLE:
            self.logger.warning("ns3gym not available, running in static mode")
            return False
        
        max_retries = 15
        for attempt in range(max_retries):
            try:
                self.logger.info(f"NS-3 연결 시도 ({attempt + 1}/{max_retries})...")
                
                self.ns3_env = Ns3Env(
                    port=self.ns3_port,
                    stepTime=0.1,
                    startSim=False,
                    debug=False
                )
                
                obs = self.ns3_env.reset()
                if obs is not None:
                    self.connected = True
                    self.logger.info(f"✅ NS-3 연결 성공! (port {self.ns3_port})")
                    self.logger.info(f"   Observation shape: {obs.shape}")
                    
                    # ✅ 수정: reset() 직후의 observation을 초기 상태로 저장
                    self._store_initial_ns3_states(obs)
                    
                    return True
                    
            except Exception as e:
                if attempt < max_retries - 1:
                    self.logger.info(f"   연결 실패, 2초 후 재시도... ({e})")
                    time.sleep(2)
                else:
                    self.logger.warning(f"NS-3 연결 실패 (정적 모드로 전환): {e}")
                    self.connected = False
                    return False
        
        return False
    
    def _store_initial_ns3_states(self, obs):
        """NS-3 초기 상태를 저장 (reset 직후 호출)"""
        ns3_states = self._extract_ns3_vehicle_states(obs)
        if ns3_states:
            self.ns3_initial_states = {
                vid: {k: v for k, v in state.items()}
                for vid, state in ns3_states.items()
            }
            self.logger.info(f"🔍 NS-3 초기 기준점 저장 (reset 직후):")
            for vid in sorted(list(ns3_states.keys()))[:3]:
                s = ns3_states[vid]
                self.logger.info(f"   차량 {vid}: pos=({s['x']:.2f}, {s['y']:.2f}), speed={s['speed']:.2f} m/s")
    
    def _extract_ns3_vehicle_states(self, obs) -> Dict[str, Dict[str, float]]:
        """NS-3 observation에서 차량 상태 추출"""
        states = {}
        
        if obs is None or len(obs) < 4:
            return states
        
        try:
            num_vehicles = int(obs[0])
            
            # ✅ 수정: NS-3의 SUMO FCD 순서에 맞춤
            # SUMO FCD 순서: veh_962, veh_971, veh_980, veh_989, veh_998, veh_1007, veh_1016
            ns3_vehicle_order = ['962', '971', '980', '989', '998', '1007', '1016']
            
            for i in range(min(num_vehicles, len(ns3_vehicle_order))):
                vid = ns3_vehicle_order[i]
                base_idx = 4 + (i * 3)
                if base_idx + 2 < len(obs):
                    states[vid] = {
                        'x': float(obs[base_idx]),
                        'y': float(obs[base_idx + 1]),
                        'speed': float(obs[base_idx + 2]),
                        'heading': 0.0
                    }
        except Exception as e:
            self.logger.warning(f"NS-3 상태 추출 실패: {e}")
        
        return states
    
    def _load_yaml(self, path: Path) -> dict:
        """YAML 파일 로드"""
        with open(path, 'r', encoding='utf-8') as f:
            return yaml.safe_load(f)
    
    def _save_yaml(self, data: dict, path: Path):
        """YAML 파일 저장"""
        path.parent.mkdir(parents=True, exist_ok=True)
        with open(path, 'w', encoding='utf-8') as f:
            yaml.safe_dump(data, f, allow_unicode=True, sort_keys=False)
    
    def _apply_ns3_transform(
        self,
        yaml_data: dict,
        ns3_states: Dict[str, Dict[str, float]],
        base_states: Dict[str, Dict[str, float]]
    ) -> None:
        """NS-3 델타를 YAML 데이터에 적용 (NS-3 초기 기준점으로부터의 상대 변화량 사용)"""
        
        if not ns3_states or not base_states or not self.ns3_initial_states:
            return
        
        # NS-3 초기 기준점으로부터의 상대 변화량 계산
        deltas = []
        speed_changes = []
        for vid in ns3_states:
            if vid in self.ns3_initial_states:
                # NS-3에서의 이동량 (초기 위치 대비)
                dx = ns3_states[vid]['x'] - self.ns3_initial_states[vid]['x']
                dy = ns3_states[vid]['y'] - self.ns3_initial_states[vid]['y']
                deltas.append((dx, dy))
                
                # 속도 변화 (초기 속도 대비)
                ns3_speed = ns3_states[vid]['speed']
                initial_speed = self.ns3_initial_states[vid]['speed']
                if initial_speed > 0.01:
                    speed_ratio = ns3_speed / initial_speed
                    speed_changes.append((vid, initial_speed, ns3_speed, speed_ratio))
        
        if not deltas:
            return
        
        avg_dx = sum(d[0] for d in deltas) / len(deltas)
        avg_dy = sum(d[1] for d in deltas) / len(deltas)
        
        # 위치 변화량 제한 (lidar_range 범위 내 유지)
        # OpenCOOD lidar_range: [-140.8, -38.4, -3, 140.8, 38.4, ?]
        # 안전한 범위 유지를 위해 최대 변화량을 20m로 제한
        MAX_DELTA = 20.0
        if abs(avg_dx) > MAX_DELTA:
            avg_dx = MAX_DELTA if avg_dx > 0 else -MAX_DELTA
        if abs(avg_dy) > MAX_DELTA:
            avg_dy = MAX_DELTA if avg_dy > 0 else -MAX_DELTA
        
        # 속도 변화 디버깅 로그 (첫 프레임만)
        if speed_changes and hasattr(self, '_first_frame_logged') == False:
            self._first_frame_logged = True
            self.logger.info("🔍 첫 프레임 속도 분석:")
            for vid, base_spd, ns3_spd, ratio in speed_changes[:3]:
                self.logger.info(f"   차량 {vid}: {base_spd:.2f}m/s → {ns3_spd:.2f}m/s (ratio: {ratio:.2f}x)")
            self.logger.info(f"   평균 위치 delta: dx={avg_dx:.2f}m, dy={avg_dy:.2f}m (제한 적용)")
        
        # 델타 계산 디버깅 (몇 프레임만)
        if not hasattr(self, '_delta_log_count'):
            self._delta_log_count = 0
        if self._delta_log_count < 5:
            self._delta_log_count += 1
            self.logger.info(f"🔍 델타 계산 (로그 {self._delta_log_count}/5):")
            self.logger.info(f"   NS-3 states 개수: {len(ns3_states)}")
            self.logger.info(f"   Initial states 개수: {len(self.ns3_initial_states)}")
            if ns3_states and self.ns3_initial_states:
                vid = list(ns3_states.keys())[0]
                self.logger.info(f"   샘플 차량 {vid}:")
                self.logger.info(f"     현재: x={ns3_states[vid]['x']:.2f}, y={ns3_states[vid]['y']:.2f}")
                if vid in self.ns3_initial_states:
                    self.logger.info(f"     초기: x={self.ns3_initial_states[vid]['x']:.2f}, y={self.ns3_initial_states[vid]['y']:.2f}")
                    self.logger.info(f"     델타: dx={ns3_states[vid]['x'] - self.ns3_initial_states[vid]['x']:.2f}, dy={ns3_states[vid]['y'] - self.ns3_initial_states[vid]['y']:.2f}")
            self.logger.info(f"   계산된 평균 델타: dx={avg_dx:.3f}, dy={avg_dy:.3f}")
        
        # Ego positions 업데이트
        for key in ['lidar_pose', 'predicted_ego_pos', 'true_ego_pos']:
            if key in yaml_data and isinstance(yaml_data[key], list):
                if len(yaml_data[key]) >= 2:
                    yaml_data[key][0] = float(yaml_data[key][0]) + avg_dx
                    yaml_data[key][1] = float(yaml_data[key][1]) + avg_dy
        
        # Camera positions 업데이트
        for cam in ['camera0', 'camera1', 'camera2', 'camera3']:
            if cam in yaml_data and 'cords' in yaml_data[cam]:
                cords = yaml_data[cam]['cords']
                if isinstance(cords, list) and len(cords) >= 2:
                    cords[0] = float(cords[0]) + avg_dx
                    cords[1] = float(cords[1]) + avg_dy
        
        # 차량별 위치 업데이트 (NS-3 초기 기준점 대비 변화량 적용)
        if 'vehicles' in yaml_data:
            for vid, vehicle_data in yaml_data['vehicles'].items():
                if vid in ns3_states and vid in self.ns3_initial_states:
                    # NS-3에서의 상대 이동량
                    dx = ns3_states[vid]['x'] - self.ns3_initial_states[vid]['x']
                    dy = ns3_states[vid]['y'] - self.ns3_initial_states[vid]['y']
                    
                    # 개별 차량 위치 변화량도 제한
                    if abs(dx) > MAX_DELTA:
                        dx = MAX_DELTA if dx > 0 else -MAX_DELTA
                    if abs(dy) > MAX_DELTA:
                        dy = MAX_DELTA if dy > 0 else -MAX_DELTA
                    
                    if 'location' in vehicle_data:
                        loc = vehicle_data['location']
                        if isinstance(loc, list) and len(loc) >= 2:
                            loc[0] = float(loc[0]) + dx
                            loc[1] = float(loc[1]) + dy
                    
                    # 속도: NS-3의 현재 속도를 직접 사용 (원본 데이터의 속도가 0이므로)
                    if 'speed' in vehicle_data:
                        vehicle_data['speed'] = float(ns3_states[vid]['speed'])
    
    def _augment_pcd(self, src_pcd: Path, dst_pcd: Path, dx: float, dy: float):
        """Point cloud 변환 및 노이즈 추가"""
        pcd = o3d.io.read_point_cloud(str(src_pcd))
        
        if len(pcd.points) > 0:
            points = np.asarray(pcd.points)
            
            # 위치 이동
            points[:, 0] += dx
            points[:, 1] += dy
            
            # LiDAR 노이즈 추가
            if self.lidar_noise_std > 0:
                noise = np.random.normal(0, self.lidar_noise_std, size=points.shape)
                points = points + noise
            
            pcd.points = o3d.utility.Vector3dVector(points)
        
        dst_pcd.parent.mkdir(parents=True, exist_ok=True)
        o3d.io.write_point_cloud(str(dst_pcd), pcd)
    
    def augment(self) -> bool:
        """메인 증강 프로세스 - NS-3 시뮬레이션 step 수에 맞춰 증강 데이터 생성"""
        try:
            # 원본 시나리오 확인
            if not self.source_scenario.exists():
                self.logger.error(f"Source scenario not found: {self.source_scenario}")
                return False
            
            # 차량 IDs 추출
            self.vehicle_ids = sorted([
                d.name for d in self.source_scenario.iterdir() 
                if d.is_dir() and d.name.isdigit()
            ])
            
            if not self.vehicle_ids:
                self.logger.error("No vehicle directories found in source")
                return False
            
            self.logger.info(f"Found {len(self.vehicle_ids)} vehicles: {', '.join(self.vehicle_ids)}")
            
            # 원본 데이터 프레임 로드 (템플릿으로 사용)
            first_vehicle_dir = self.source_scenario / self.vehicle_ids[0]
            source_frames = sorted([f.stem for f in first_vehicle_dir.glob('*.yaml')])
            source_frame_count = len(source_frames)
            
            if source_frame_count == 0:
                self.logger.error("No frames found in source scenario")
                return False
            
            self.logger.info(f"Source template frames: {source_frame_count}")
            
            # NS-3 연결
            self.connect_to_ns3()
            
            # 실제 생성할 프레임 수 결정
            if self.connected:
                # NS-3 연결됨: 시뮬레이션이 끝날 때까지 (최대 expected_ns3_frames)
                total_frames = self.expected_ns3_frames
                self.logger.info(f"🎯 NS-3 connected: Will generate up to {total_frames} frames (based on {self.ns3_sim_time}s simulation)")
            else:
                # NS-3 미연결: 원본 프레임 수만큼만 생성
                total_frames = source_frame_count
                self.logger.warning(f"⚠️ NS-3 not connected: Will generate {total_frames} frames (source frame count)")
            
            self.logger.info(f"📊 Frame generation plan:")
            self.logger.info(f"   Source template: {source_frame_count} frames")
            self.logger.info(f"   Target output: {total_frames} frames")
            if total_frames > source_frame_count:
                self.logger.info(f"   Strategy: Cycle through source frames {total_frames // source_frame_count + 1} times")
            
            # 출력 디렉토리 생성
            self.output_path.mkdir(parents=True, exist_ok=True)
            
            # NS-3 시뮬레이션 step 수에 맞춰 프레임 생성
            for frame_idx in range(total_frames):
                # 원본 데이터에서 템플릿 프레임 선택 (순환)
                source_frame_idx = frame_idx % source_frame_count
                source_frame_name = source_frames[source_frame_idx]
                
                # 출력 프레임 이름 (0부터 연속)
                output_frame_name = f"{frame_idx:06d}"
                
                # NS-3에서 현재 타임스텝 데이터 획득
                ns3_states = {}
                if self.connected and self.ns3_env:
                    try:
                        action = self.ns3_env.action_space.sample()
                        obs, reward, done, info = self.ns3_env.step(action)
                        
                        if done:
                            self.logger.info(f"✅ NS-3 simulation completed at frame {frame_idx}/{total_frames}")
                            self.logger.info(f"   Generated {frame_idx} frames total")
                            # NS-3 시뮬레이션이 끝나면 여기서 증강 종료
                            total_frames = frame_idx  # 실제 생성된 프레임 수로 업데이트
                            break
                        else:
                            ns3_states = self._extract_ns3_vehicle_states(obs)
                            
                            # ✅ 수정: 진행 상황 로깅 (매 10 프레임마다)
                            if frame_idx % 10 == 0 and ns3_states:
                                self.logger.info(f"📍 Frame {frame_idx}/{total_frames}: NS-3 state check")
                                for vid in sorted(list(ns3_states.keys()))[:2]:
                                    s = ns3_states[vid]
                                    init_s = self.ns3_initial_states.get(vid, {})
                                    dx = s['x'] - init_s.get('x', 0)
                                    dy = s['y'] - init_s.get('y', 0)
                                    self.logger.info(
                                        f"  차량 {vid}: pos=({s['x']:.1f},{s['y']:.1f}), "
                                        f"delta=({dx:.1f},{dy:.1f}m), speed={s['speed']:.1f}m/s"
                                    )
                            
                    except Exception as e:
                        self.logger.warning(f"NS-3 step failed (frame {frame_idx}): {e}")
                        self.connected = False
                
                # 각 차량별 데이터 처리
                for vehicle_id in self.vehicle_ids:
                    src_vehicle_dir = self.source_scenario / vehicle_id
                    dst_vehicle_dir = self.output_path / vehicle_id
                    dst_vehicle_dir.mkdir(parents=True, exist_ok=True)
                    
                    # ✅ 수정: 이미지 symlink 생성 (첫 프레임만)
                    if self.copy_images and frame_idx == 0:
                        for png in src_vehicle_dir.glob('*.png'):
                            dst_png = dst_vehicle_dir / png.name
                            try:
                                # 상대 경로로 symlink 생성
                                rel_path = os.path.relpath(png, dst_vehicle_dir)
                                os.symlink(rel_path, dst_png)
                            except FileExistsError:
                                pass  # 이미 존재하면 무시
                            except OSError as e:
                                # Symlink 실패 시 복사로 fallback
                                self.logger.warning(f"Symlink failed for {png.name}, copying: {e}")
                                shutil.copy2(png, dst_png)
                    
                    # 원본 YAML 로드 (템플릿) 및 base states 추출
                    src_yaml_path = src_vehicle_dir / f"{source_frame_name}.yaml"
                    yaml_data = self._load_yaml(src_yaml_path)
                    
                    # Base states 추출 (이 프레임의 원본 차량 위치)
                    base_states = {}
                    if 'vehicles' in yaml_data:
                        for vid, vdata in yaml_data['vehicles'].items():
                            if 'location' in vdata:
                                loc = vdata['location']
                                base_states[vid] = {
                                    'x': float(loc[0]) if len(loc) > 0 else 0.0,
                                    'y': float(loc[1]) if len(loc) > 1 else 0.0,
                                    'speed': float(vdata.get('speed', 0.0)),
                                    'heading': 0.0
                                }
                    
                    # 디버깅: 첫 프레임 base states 로깅
                    if frame_idx == 0 and vehicle_id == self.vehicle_ids[0] and base_states:
                        self.logger.info(f"🔍 원본 데이터 첫 프레임 base states:")
                        for vid in sorted(list(base_states.keys()))[:3]:
                            s = base_states[vid]
                            self.logger.info(f"   차량 {vid}: pos=({s['x']:.2f}, {s['y']:.2f}), speed={s['speed']:.2f} m/s")
                    
                    # NS-3 델타 적용
                    if ns3_states:
                        self._apply_ns3_transform(yaml_data, ns3_states, base_states)
                    
                    # 출력 YAML 저장 (새 프레임 이름으로)
                    dst_yaml_path = dst_vehicle_dir / f"{output_frame_name}.yaml"
                    self._save_yaml(yaml_data, dst_yaml_path)
                    
                    # PCD 처리 (템플릿에서 가져와서 변환 후 새 이름으로 저장)
                    src_pcd = src_vehicle_dir / f"{source_frame_name}.pcd"
                    dst_pcd = dst_vehicle_dir / f"{output_frame_name}.pcd"
                    
                    if src_pcd.exists():
                        # 평균 델타 계산 (NS-3 초기 기준점 대비)
                        if ns3_states and self.ns3_initial_states:
                            deltas = [
                                (ns3_states[vid]['x'] - self.ns3_initial_states[vid]['x'],
                                 ns3_states[vid]['y'] - self.ns3_initial_states[vid]['y'])
                                for vid in ns3_states if vid in self.ns3_initial_states
                            ]
                            avg_dx = sum(d[0] for d in deltas) / len(deltas) if deltas else 0.0
                            avg_dy = sum(d[1] for d in deltas) / len(deltas) if deltas else 0.0
                            
                            # PCD 변환에도 제한 적용
                            MAX_DELTA = 20.0
                            if abs(avg_dx) > MAX_DELTA:
                                avg_dx = MAX_DELTA if avg_dx > 0 else -MAX_DELTA
                            if abs(avg_dy) > MAX_DELTA:
                                avg_dy = MAX_DELTA if avg_dy > 0 else -MAX_DELTA
                        else:
                            avg_dx = avg_dy = 0.0
                        
                        self._augment_pcd(src_pcd, dst_pcd, avg_dx, avg_dy)
                
                # ✅ 수정: 진행 상황 로깅 개선
                if (frame_idx + 1) % 50 == 0 or frame_idx == 0:
                    progress = (frame_idx + 1) / total_frames * 100
                    self.logger.info(f"⏳ Progress: {frame_idx + 1}/{total_frames} frames ({progress:.1f}%)")
                    self.logger.info(f"   Source template: frame {source_frame_idx}/{source_frame_count} (cycling)")
                    if ns3_states:
                        self.logger.info(f"   NS-3 connected: ✅ Active")
                    else:
                        self.logger.info(f"   NS-3 connected: ⚠️ Disconnected or static mode")
            
            # 원본 data_protocol.yaml 복사 (OpenCOOD 호환성을 위해)
            source_protocol = self.source_scenario / 'data_protocol.yaml'
            if source_protocol.exists():
                shutil.copy2(source_protocol, self.output_path / 'data_protocol.yaml')
                self.logger.info("Copied original data_protocol.yaml for OpenCOOD compatibility")
            else:
                # Fallback: 메타데이터만 저장
                metadata = {
                    'augmentation': {
                        'type': 'ns3_guided_proven_data',
                        'source': str(self.source_scenario.name),
                        'source_frames': source_frame_count,
                        'generated_frames': total_frames,
                        'ns3_sim_time': self.ns3_sim_time,
                        'ns3_step_time': self.ns3_step_time,
                        'ns3_connected': self.connected,
                        'ns3_port': self.ns3_port,
                        'lidar_noise_std': self.lidar_noise_std,
                        'timestamp': datetime.now().isoformat()
                    }
                }
                self._save_yaml(metadata, self.output_path / 'data_protocol.yaml')
            
            self.logger.info(f"📊 Augmentation Summary:")
            self.logger.info(f"   Source template: {source_frame_count} frames")
            self.logger.info(f"   Generated output: {total_frames} frames")
            self.logger.info(f"   NS-3 simulation: {self.ns3_sim_time}s @ {self.ns3_step_time}s/step")
            if total_frames > source_frame_count:
                cycles = total_frames / source_frame_count
                self.logger.info(f"   Template cycled: {cycles:.1f}x")
            
            # NS-3 연결 종료
            if self.ns3_env:
                try:
                    self.ns3_env.close()
                except:
                    pass
            
            self.logger.info(f"✅ Augmentation complete: {self.output_path}")
            return True
            
        except Exception as e:
            self.logger.error(f"Augmentation failed: {e}", exc_info=True)
            return False


def main():
    """CLI Entry Point"""
    import argparse
    
    parser = argparse.ArgumentParser(
        description="NS-3 Guided Proven Data Augmentation"
    )
    parser.add_argument(
        '--source',
        type=str,
        required=True,
        help='Source proven data scenario path'
    )
    parser.add_argument(
        '--output-dir',
        type=str,
        required=True,
        help='Output directory'
    )
    parser.add_argument(
        '--scenario-name',
        type=str,
        required=True,
        help='Output scenario name'
    )
    parser.add_argument(
        '--ns3-port',
        type=int,
        default=5555,
        help='NS-3 OpenGym port (default: 5555)'
    )
    parser.add_argument(
        '--ns3-sim-time',
        type=float,
        default=120.0,
        help='NS-3 simulation time in seconds (default: 120.0)'
    )
    parser.add_argument(
        '--ns3-step-time',
        type=float,
        default=0.1,
        help='NS-3 step interval in seconds (default: 0.1)'
    )
    parser.add_argument(
        '--lidar-noise-std',
        type=float,
        default=0.03,
        help='LiDAR noise std-dev (default: 0.03)'
    )
    parser.add_argument(
        '--no-copy-images',
        action='store_true',
        help='Skip copying PNG images'
    )
    
    args = parser.parse_args()
    
    augmentor = NS3GuidedProvenDataAugmentor(
        source_scenario=args.source,
        output_dir=args.output_dir,
        scenario_name=args.scenario_name,
        ns3_port=args.ns3_port,
        ns3_sim_time=args.ns3_sim_time,
        ns3_step_time=args.ns3_step_time,
        lidar_noise_std=args.lidar_noise_std,
        copy_images=not args.no_copy_images
    )
    
    success = augmentor.augment()
    sys.exit(0 if success else 1)


if __name__ == '__main__':
    main()
