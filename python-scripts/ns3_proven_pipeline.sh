#!/bin/bash
# NS-3 Guided Proven Data Augmentation + Inference Pipeline
# 기존 v2x_to_inference_pipeline.sh 구조를 유지하면서
# proven-good 데이터를 NS-3 텔레메트리로 증강

set -e

echo "🚀 NS-3 Guided Proven Data Augmentation + Inference Pipeline"
echo "============================================================"

# 환경 설정
source ~/miniconda3/etc/profile.d/conda.sh
conda activate opencood

# 시작 시간 기록
START_TIME=$(date +%s)
echo "⏰ 파이프라인 시작 시간: $(date)"
echo ""

# =============================================================================
# Step 1: NS-3 시뮬레이션 상태 확인
# =============================================================================
echo "📊 Step 1: NS-3 시뮬레이션 상태 확인"
echo "=================================="
cd /home/aibox-kou-a/subin

echo "🔍 NS-3 Simple V2X 시뮬레이션 상태 확인..."
if pgrep -f "simple-v2x-sim" > /dev/null; then
    echo "✅ NS-3 Simple V2X 시뮬레이션이 실행 중입니다"
    echo "   (SUMO trace 사용 여부는 NS-3 로그에서 확인 가능)"
else
    echo "❌ NS-3 Simple V2X 시뮬레이션이 실행되지 않았습니다!"
    echo ""
    echo "   먼저 다른 터미널에서 NS-3를 실행하세요:"
    echo ""
    echo "   # SUMO trace 사용 (권장):"
    echo "   cd /home/aibox-kou-a/subin/ns-allinone-3.40/ns-3.40"
    echo "   ./ns3 run 'simple-v2x-sim --sumoTrace=/home/aibox-kou-a/subin/sumo_traces/highway_7_vehicles_fcd.xml --simTime=120'"
    echo ""
    echo "   # 또는 Random Walk 사용:"
    echo "   ./ns3 run 'simple-v2x-sim --simTime=120'"
    echo ""
    exit 1
fi

# =============================================================================
# Step 2: NS-3 Guided Proven Data Augmentation
# =============================================================================
echo ""
echo "📊 Step 2: NS-3 Guided Proven Data 증강"
echo "=================================="

SOURCE_DATA="/home/aibox-kou-a/subin/mobility_aware_cooperative_perception/data/opv2v_real_scenarios/arxiv/2021_09_11_00_33_16_temp"
VALIDATE_ROOT="/home/aibox-kou-a/subin/mobility_aware_cooperative_perception/data/opv2v_real_scenarios/validate"
SCENARIO_NAME="ns3_proven_$(date +%Y_%m_%d_%H_%M_%S)"

echo "🔄 NS-3 Guided 증강 실행..."
echo "   Source (proven data): $SOURCE_DATA"
echo "   Output: $VALIDATE_ROOT/$SCENARIO_NAME"
echo ""

python3 ns3_guided_proven_data_augmentation.py \
    --source "$SOURCE_DATA" \
    --output-dir "$VALIDATE_ROOT" \
    --scenario-name "$SCENARIO_NAME" \
    --ns3-port 5555 \
    --ns3-sim-time 120.0 \
    --ns3-step-time 0.1 \
    --lidar-noise-std 0.03

AUG_EXIT_CODE=$?

if [ $AUG_EXIT_CODE -ne 0 ]; then
    echo "❌ 증강 데이터 생성 실패 (Exit Code: $AUG_EXIT_CODE)"
    exit 1
fi

echo "✅ NS-3 Guided Proven Data 증강 완료!"
V2X_DATA_PATH="$VALIDATE_ROOT/$SCENARIO_NAME"

# 데이터 요약 정보
if [ -d "$V2X_DATA_PATH" ]; then
    VEHICLE_COUNT=$(ls -1 "$V2X_DATA_PATH" | grep -E "^[0-9]+$" | wc -l)
    echo "🚗 생성된 차량 수: $VEHICLE_COUNT"
    
    FIRST_VEHICLE=$(ls "$V2X_DATA_PATH" | grep -E "^[0-9]+$" | head -n1)
    if [ -d "$V2X_DATA_PATH/$FIRST_VEHICLE" ]; then
        FRAME_COUNT=$(ls -1 "$V2X_DATA_PATH/$FIRST_VEHICLE" | grep "\.yaml$" | wc -l)
        echo "📸 생성된 프레임 수: $FRAME_COUNT"
    fi
fi

# =============================================================================
# Step 3: Config 파일 업데이트 및 Inference 실행
# =============================================================================
echo ""
echo "📊 Step 3: 모델 추론 실행"
echo "=================================="

MODEL_DIR="/home/aibox-kou-a/subin/mobility_aware_cooperative_perception/models/pretrained/pointpillar_cobevt"
CONFIG_PATH="$MODEL_DIR/config.yaml"

# Config 백업
echo "💾 Config 파일 백업..."
BACKUP_CONFIG="${CONFIG_PATH}.backup_$(date +%Y%m%d_%H%M%S)"
cp "$CONFIG_PATH" "$BACKUP_CONFIG"
echo "   Backup: $BACKUP_CONFIG"

# Validate 경로 업데이트 (OpenCOOD는 validate_dir 아래에서 시나리오를 찾음)
echo "🔧 Config 파일 업데이트 (validate path)..."
sed -i "s|validate_dir:.*|validate_dir: '$VALIDATE_ROOT'|g" "$CONFIG_PATH"
echo "   Validate dir: $VALIDATE_ROOT"
echo "   Scenario: $SCENARIO_NAME"

echo "🚀 추론 실행..."
echo "   Model: PointPillar CoBEVT"
echo "   Model Dir: $MODEL_DIR"
echo "   Fusion: intermediate"
echo ""

INFERENCE_LOG="inference_ns3_proven_$(date +%Y%m%d_%H%M%S).log"

cd /home/aibox-kou-a/subin/OpenCOOD
python -m opencood.tools.inference \
    --model_dir "$MODEL_DIR" \
    --fusion_method intermediate 2>&1 | tee "/home/aibox-kou-a/subin/$INFERENCE_LOG"

INFERENCE_EXIT_CODE=$?

# Config 복원
echo ""
echo "🔄 Config 파일 복원..."
mv "$BACKUP_CONFIG" "$CONFIG_PATH"

# =============================================================================
# Step 4: 결과 요약
# =============================================================================
echo ""
echo "=========================================="
echo "파이프라인 실행 완료!"
echo "=========================================="

END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))
echo "⏱️  총 실행 시간: ${DURATION}초"
echo ""

if [ $INFERENCE_EXIT_CODE -eq 0 ]; then
    echo "✅ 추론 성공!"
    echo ""
    echo "📊 결과 파일:"
    echo "   - 증강 데이터: $V2X_DATA_PATH"
    echo "   - 추론 로그: /home/aibox-kou-a/subin/$INFERENCE_LOG"
    echo ""
    echo "📈 성능 확인:"
    echo "   grep 'AP@' /home/aibox-kou-a/subin/$INFERENCE_LOG"
else
    echo "❌ 추론 실패 (Exit Code: $INFERENCE_EXIT_CODE)"
    echo "   로그 확인: /home/aibox-kou-a/subin/$INFERENCE_LOG"
    exit 1
fi

echo ""
echo "🎉 파이프라인 완료!"
echo "=========================================="
