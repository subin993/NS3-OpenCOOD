# Model Download Instructions

Due to GitHub file size limitations, the trained model checkpoint files (.pth) are hosted externally.

## 📥 Download Links

### Option 1: Direct Download (Recommended)

Download the models and place them in the corresponding directories:

```bash
cd /path/to/ns3_opencood/models

# Create directories
mkdir -p snahcp v2vnet attentive_fusion cobevt cobevt_nocompression early_fusion late_fusion fcooper v2xvit

# Download models (links will be added after upload)
# SNA-HCP model (~39 MB)
wget -O snahcp/latest.pth [DOWNLOAD_LINK]

# V2VNet model (~56 MB)
wget -O v2vnet/net_epoch83.pth [DOWNLOAD_LINK]

# AttentiveFusion model (~26 MB)
wget -O attentive_fusion/latest.pth [DOWNLOAD_LINK]

# CoBEVT model (~43 MB)
wget -O cobevt/net_epoch33.pth [DOWNLOAD_LINK]

# CoBEVT (no compression) model (~41 MB)
wget -O cobevt_nocompression/net_epoch19.pth [DOWNLOAD_LINK]

# Early Fusion model (~26 MB)
wget -O early_fusion/latest.pth [DOWNLOAD_LINK]

# Late Fusion model (~26 MB)
wget -O late_fusion/net_epoch30.pth [DOWNLOAD_LINK]

# F-Cooper model (~28 MB)
wget -O fcooper/latest.pth [DOWNLOAD_LINK]

# V2X-ViT model (~55 MB)
wget -O v2xvit/net_epoch60.pth [DOWNLOAD_LINK]
```

### Option 2: Download Script

```bash
cd /path/to/ns3_opencood
chmod +x scripts/download_models.sh
./scripts/download_models.sh
```

## 📦 Model Sizes

| Model | File Size | Epochs |
|-------|-----------|--------|
| SNA-HCP | ~39 MB | latest |
| V2VNet | ~56 MB | 83 |
| V2X-ViT | ~55 MB | 60 |
| CoBEVT | ~43 MB | 33 |
| CoBEVT (no comp) | ~41 MB | 19 |
| F-Cooper | ~28 MB | latest |
| AttentiveFusion | ~26 MB | latest |
| Early Fusion | ~26 MB | latest |
| Late Fusion | ~26 MB | 30 |

**Total:** ~340 MB

## 🔗 External Hosting Options

Models will be available from:
- [ ] Google Drive
- [ ] GitHub Releases
- [ ] Zenodo (permanent DOI)
- [ ] Hugging Face Model Hub

## ✅ Verification

After downloading, verify the file structure:

```bash
models/
├── README.md
├── DOWNLOAD.md
├── snahcp/
│   └── latest.pth
├── v2vnet/
│   └── net_epoch83.pth
├── attentive_fusion/
│   └── latest.pth
├── cobevt/
│   └── net_epoch33.pth
├── cobevt_nocompression/
│   └── net_epoch19.pth
├── early_fusion/
│   └── latest.pth
├── late_fusion/
│   └── net_epoch30.pth
├── fcooper/
│   └── latest.pth
└── v2xvit/
    └── net_epoch60.pth
```

Check file sizes:
```bash
du -sh models/*/
```

Expected output:
```
26M     models/attentive_fusion/
26M     models/early_fusion/
28M     models/fcooper/
26M     models/late_fusion/
43M     models/cobevt/
41M     models/cobevt_nocompression/
39M     models/snahcp/
56M     models/v2vnet/
55M     models/v2xvit/
```

## 📧 Support

If you encounter download issues:
- Open an issue on GitHub
- Contact: subin993@korea.ac.kr
