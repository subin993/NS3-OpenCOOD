# Model Download Instructions

Due to GitHub file size limitations, the trained model checkpoint files (.pth) are hosted externally.

## 📥 Download Links

### Option 1: Direct Download (Recommended)

Download the models and place them in the corresponding directories:

```bash
cd /path/to/ns3_opencood/models

# Create directories
mkdir -p snahcp v2vnet attentive_fusion cobevt

# Download models (links will be added after upload)
# SNA-HCP model (~39 MB)
wget -O snahcp/net_epoch5.pth [DOWNLOAD_LINK]

# V2VNet model (~56 MB)
wget -O v2vnet/net_epoch83.pth [DOWNLOAD_LINK]

# AttentiveFusion model (~26 MB)
wget -O attentive_fusion/latest.pth [DOWNLOAD_LINK]

# CoBEVT model (~43 MB)
wget -O cobevt/net_epoch33.pth [DOWNLOAD_LINK]
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
| SNA-HCP | ~39 MB | 5 |
| V2VNet | ~56 MB | 83 |
| AttentiveFusion | ~26 MB | latest |
| CoBEVT | ~43 MB | 33 |

**Total:** ~164 MB

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
│   └── net_epoch5.pth
├── v2vnet/
│   └── net_epoch83.pth
├── attentive_fusion/
│   └── latest.pth
└── cobevt/
    └── net_epoch33.pth
```

Check file sizes:
```bash
du -sh models/*/
```

Expected output:
```
39M     models/snahcp/
56M     models/v2vnet/
26M     models/attentive_fusion/
43M     models/cobevt/
```

## 📧 Support

If you encounter download issues:
- Open an issue on GitHub
- Contact: subin993@korea.ac.kr
