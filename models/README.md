# Trained Models for SNA-HCP

This directory contains the trained models used in the SNA-HCP paper experiments.

## 📦 Model Files

### Baseline Models

#### 1. V2VNet
- **Path:** `v2vnet/net_epoch83.pth`
- **Size:** ~56 MB
- **Description:** PointPillar with V2VNet intermediate fusion
- **Epochs:** 83
- **Performance:** 83.5% AP@0.7 at α=1.0, 63.5 KB/agent bandwidth

#### 2. AttentiveFusion
- **Path:** `attentive_fusion/latest.pth`
- **Size:** ~26 MB
- **Description:** PointPillar with Attentive Fusion mechanism
- **Performance:** 83.2% AP@0.7 at α=1.0, 63.5 KB/agent bandwidth

#### 3. CoBEVT
- **Path:** `cobevt/net_epoch33.pth`
- **Size:** ~43 MB
- **Description:** PointPillar with CoBEVT compression
- **Epochs:** 33
- **Performance:** 87.0% AP@0.7 at α=1.0, 35.5 KB/agent bandwidth

### SNA-HCP Model

#### SNA-HCP (Ours)
- **Path:** `snahcp/net_epoch5.pth`
- **Size:** ~39 MB
- **Description:** Hierarchical fusion with SNAC clustering module
- **Epochs:** 5
- **Performance:** 87.6% AP@0.7 at α=1.0, 28.7 KB/agent bandwidth
- **Key Features:**
  - Semantic- and Network-Aware Clustering
  - Hierarchical collaborative perception
  - Adaptive bandwidth management
  - Best accuracy-bandwidth trade-off (3.05 AP@0.7 per KB)

## 🚀 Usage

### Loading Models in OpenCOOD

```python
import torch
from opencood.tools import train_utils

# Load SNA-HCP model
model_path = "models/snahcp/net_epoch5.pth"
checkpoint = torch.load(model_path)

# Load baseline model (e.g., V2VNet)
model_path = "models/v2vnet/net_epoch83.pth"
checkpoint = torch.load(model_path)
```

### Running Inference

```bash
# SNA-HCP inference
cd /path/to/OpenCOOD
python -m opencood.tools.inference \
    --model_dir /path/to/ns3_opencood/models/snahcp \
    --fusion_method intermediate

# V2VNet inference
python -m opencood.tools.inference \
    --model_dir /path/to/ns3_opencood/models/v2vnet \
    --fusion_method intermediate
```

## 📊 Performance Comparison

| Model | AP@0.7 (α=1.0) | Bandwidth (KB) | Efficiency (AP/KB) |
|-------|----------------|----------------|-------------------|
| **SNA-HCP** | **87.6%** | **28.7** | **3.05** |
| CoBEVT | 87.0% | 35.5 | 2.45 |
| V2VNet | 83.5% | 63.5 | 1.31 |
| AttentiveFusion | 83.2% | 63.5 | 1.31 |

## 📝 Citation

If you use these models in your research, please cite:

```bibtex
@article{snahcp2025,
  title={Semantic- and Network-Aware Hierarchical Collaborative Perception in Vehicular Networks},
  author={Subin Han, Dusit Niyato, and Sangheon Pack},
  journal={T.B.D.},
  year={2025},
  note={Implementation and models available at \url{https://github.com/subin993/NS3-OpenCOOD}}
}
```

## ⚠️ Notes

- All models are trained on the OPV2V dataset
- Models use PointPillar backbone with different fusion strategies
- SNA-HCP includes the SNAC clustering module for network-aware communication
- For training details, see the main repository README
- Model files are provided under MIT License

## 📧 Contact

For questions about the models or training procedures:
- Email: subin993@korea.ac.kr
- GitHub: https://github.com/subin993/NS3-OpenCOOD
