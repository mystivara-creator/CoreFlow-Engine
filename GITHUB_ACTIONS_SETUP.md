# GitHub Actions for CoreFlow v3.0 - Complete Setup Guide

**Compile, build, dan release CoreFlow OTOMATIS tanpa terminal/PC!**

---

## 🎯 Apa Yang Anda Dapatkan

Dengan GitHub Actions, saat Anda:

- **Push code** → Workflow otomatis build binary
- **Create tag** → Workflow otomatis create release
- **Trigger manual** → Build on-demand kapan saja

**Semua terjadi di cloud - tidak perlu PC/terminal lokal!**

---

## 📋 Setup (Hanya 5 Langkah)

### Step 1: Create GitHub Repository

1. Go to https://github.com/new
2. Create repository:
   - **Name**: `CoreFlow` atau `coreflow-module`
   - **Public** atau **Private** (terserah)
   - **Add README** ✓
   - Click **Create repository**

### Step 2: Upload All Files

**Option A: Web UI (Recommended - Tanpa Git)**

1. Open repository GitHub Anda
2. Click **Add file** → **Upload files**
3. Drag-and-drop semua file dari package CoreFlow
4. Atau select individually:
   - coreflow_engine.cpp
   - *.sh files
   - module.prop
   - Makefile
   - README.md, etc.
5. Add commit message: "Initial commit - CoreFlow v3.0"
6. Click **Commit changes**

**Option B: Git CLI (Kalau sudah punya)**

```bash
git clone https://github.com/YOUR_USERNAME/CoreFlow.git
cd CoreFlow
cp /path/to/coreflow/files/* .
git add .
git commit -m "Initial commit - CoreFlow v3.0"
git push origin main
```

### Step 3: Create Workflows Directory

1. In repository, click **Add file** → **Create new file**
2. Path: `.github/workflows/build.yml`
3. Copy entire content dari file `.github_workflows_build.yml`
4. Click **Commit new file**

### Step 4: Create Release Workflow

1. Click **Add file** → **Create new file**
2. Path: `.github/workflows/release.yml`
3. Copy entire content dari file `.github_workflows_release.yml`
4. Click **Commit new file**

### Step 5: Create .gitignore (Optional but Recommended)

1. Click **Add file** → **Create new file**
2. Path: `.gitignore`
3. Paste ini:

```
# Build artifacts
bin/
build/
dist/
*.o
*.a
*.so

# IDE
.vscode/
.idea/
*.swp
*.swo

# OS
.DS_Store
Thumbs.db

# Temp
.temp/
*.tmp
```

4. Click **Commit new file**

---

## ✅ Done! Setup Selesai

GitHub Anda sekarang punya automatic CI/CD untuk CoreFlow!

---

## 🚀 Cara Menggunakan

### Method 1: Push Code (Auto Build)

Setiap kali Anda push changes:

1. Go to repository
2. Click **Add file** → **Upload files** atau Edit file
3. Add/modify files:
   - coreflow_engine.cpp
   - *.sh scripts
   - etc.
4. Click **Commit changes**
5. ✅ Build workflow otomatis jalan!

**Lihat progress:**
1. Go to repository
2. Click **Actions** tab
3. Lihat workflow running

### Method 2: Create Release (Auto Package & Release)

Untuk publish official release:

1. Go to repository
2. Click **Releases** (di sidebar kanan)
3. Click **Create a new release**
4. Fill in:
   - **Tag version**: `v3.0` (atau `v3.0.1`, etc.)
   - **Release title**: `CoreFlow v3.0`
   - **Description**: Copy dari RELEASE_NOTES.md
5. Click **Publish release**
6. ✅ Release workflow jalan otomatis!
7. ✅ Binary & ZIP di-attach automatically!

**Hasilnya:**
- Flashable ZIP automatically created
- Checksums generated
- Release notes included
- Download link ready

### Method 3: Manual Trigger (Anytime)

Untuk build kapan saja tanpa commit:

1. Go to repository
2. Click **Actions** tab
3. Select **Build CoreFlow Module** workflow
4. Click **Run workflow** button
5. Click **Run workflow** confirm
6. ✅ Build starts otomatis!

**Download hasil:**
1. Wait for workflow complete (5-10 minutes)
2. Click workflow name
3. Scroll ke **Artifacts**
4. Click **CoreFlow-Build** 
5. Download ZIP file!

---

## 📊 Workflow Explained

### Build Workflow (build.yml)

**Triggers:**
- Every push to main/develop branches
- Every pull request
- Manual trigger via Actions tab

**Does:**
1. ✅ Checkout code
2. ✅ Setup Android NDK
3. ✅ Compile daemon (C++ → ARM64 binary)
4. ✅ Strip binary (reduce size)
5. ✅ Create module structure
6. ✅ Package flashable ZIP
7. ✅ Generate checksums
8. ✅ Upload as artifacts

**Output:**
- CoreFlow_v3.0.zip
- SHA256 checksum
- MD5 checksum
- Build info

**Time:** 5-10 minutes

---

### Release Workflow (release.yml)

**Triggers:**
- When you create a git tag (v3.0, v3.0.1, etc.)
- Manual trigger from Actions tab

**Does:**
1. ✅ All steps dari Build workflow
2. ✅ Create release notes
3. ✅ Create GitHub Release
4. ✅ Attach all artifacts
5. ✅ Make downloadable

**Output:**
- GitHub Release page
- Downloadable ZIP
- Checksums
- Release notes

**Time:** 5-10 minutes

---

## 💡 Use Cases

### Case 1: Development & Testing

```
You modify code → Push to GitHub → Auto build → Download artifact → Test on device
```

1. Edit file (e.g., `coreflow_engine.cpp`)
2. Click **Commit changes**
3. Go to **Actions** tab
4. Wait for build complete
5. Download artifact
6. Flash to device & test

### Case 2: Official Release

```
Final code ready → Create tag → Auto build & release → Download from GitHub Releases
```

1. Code finalized
2. Go to **Releases**
3. Click **Create release**
4. Enter tag: `v3.0`
5. Click **Publish**
6. Wait for workflow
7. Release page auto-populated with downloads!

### Case 3: Quick Build (No Git)

```
Just want to build → Manual trigger → Wait → Download
```

1. Go to **Actions** tab
2. Select **Build CoreFlow Module**
3. Click **Run workflow**
4. Wait ~10 minutes
5. Download artifact

---

## 📥 Downloading Artifacts

### After Build Workflow

1. Go to repository
2. Click **Actions** tab
3. Click latest **Build CoreFlow Module** workflow
4. Scroll down ke **Artifacts**
5. Click **CoreFlow-Build** 
6. Download ZIP file
7. Extract: `CoreFlow_v3.0.zip`

### After Release Workflow

1. Go to repository
2. Click **Releases** (sidebar)
3. Click latest release (e.g., v3.0)
4. Click **CoreFlow_v3.0.zip** to download
5. Or right-click → Save as

**Advantages:**
- No need to build locally
- Always have latest version
- Checksums for verification
- Release notes included

---

## 🔧 Configuration

### Modify Workflows

Kalau mau customize workflows:

1. Go to `.github/workflows/build.yml`
2. Click **Edit this file** (pencil icon)
3. Modify:
   - NDK version: `ndk-version: r25b`
   - Build flags: `-O3 -march=armv8-a`
   - etc.
4. Click **Commit changes**

### Use Different NDK Version

Edit `.github/workflows/build.yml`:

```yaml
- name: Setup Android NDK
  uses: nttld/setup-ndk@v1
  with:
    ndk-version: r26  # Change ini
```

### Add More Optimization Flags

Edit compile step:

```bash
$CXX -O3 -march=armv8-a -mtune=cortex-a75 \
  -fPIE -fPIC -Wall -Wextra -std=c++17 ...
```

---

## 🐛 Troubleshooting

### "Workflow failed"

1. Go to **Actions**
2. Click failed workflow
3. Click **Build CoreFlow Module** step
4. Scroll dan baca error message
5. Common issues:
   - NDK setup: check ndk-version
   - Compilation error: check coreflow_engine.cpp syntax
   - Permission: check file permissions

### "Artifact not available"

1. Workflow must complete successfully (green ✓)
2. Takes 5-10 minutes
3. Check **Artifacts** section exists
4. If missing, workflow might failed

### "Can't download from releases"

1. Release must be created (not draft)
2. Workflow must complete
3. Artifacts automatically attached
4. If still missing, workflow failed - check logs

---

## 📱 Installation from GitHub

### Download CoreFlow_v3.0.zip

**From Artifacts (After Build):**
1. Go to Actions → Latest workflow
2. Download from Artifacts

**From Releases (After Release):**
1. Go to Releases
2. Click latest release
3. Click CoreFlow_v3.0.zip

### Flash to Device

```bash
# Push to device
adb push CoreFlow_v3.0.zip /sdcard/

# Extract to modules (via Magisk Manager)
# Or via ADB:
adb shell su -c "cd /data/adb/modules && unzip /sdcard/CoreFlow_v3.0.zip"
adb reboot
```

---

## ✨ Advanced Features

### Auto Release Drafts

Kalau mau auto-create draft releases:

Edit release.yml:
```yaml
draft: true  # Change to true untuk drafts
```

### Custom Build Names

Kalau mau custom ZIP names:

Edit build.yml:
```bash
# Change nama di sini
OUTPUT_ZIP="dist/CoreFlow_v3.0.zip"
```

### Add More Workflows

Bisa add:
- Testing workflow
- Security scan
- Documentation build
- Performance benchmarks

---

## 📊 Workflow Status Dashboard

GitHub automatically shows:

1. **Build Status Badge**
   ```markdown
   [![Build Status](https://github.com/YOUR_USER/CoreFlow/actions/workflows/build.yml/badge.svg)](https://github.com/YOUR_USER/CoreFlow/actions)
   ```

2. **Release Downloads**
   - Visible on Releases page
   - Download counts tracked
   - Version history maintained

3. **Action History**
   - All builds logged
   - Success/failure tracked
   - Runtimes recorded

---

## 🎯 Best Practices

✅ **DO:**
- Keep README.md updated
- Use meaningful commit messages
- Create releases for major versions
- Test locally before pushing
- Document changes in commits

❌ **DON'T:**
- Push untested code
- Delete successful workflow runs
- Modify workflows without understanding
- Commit large files (use .gitignore)
- Ignore workflow failures

---

## 📚 Quick Reference

| Task | Steps |
|------|-------|
| **Build** | Push code → Auto build → Download |
| **Release** | Create tag → Auto release → Download |
| **Manual Build** | Actions → Run workflow → Wait → Download |
| **Modify** | Edit file → Commit → Auto build |
| **Check Status** | Go to Actions tab → See running/completed |

---

## 🔐 Security Notes

✅ **Safe:**
- Workflows run on GitHub's servers
- No credentials needed
- No sensitive data exposed
- All logs public (unless private repo)

❌ **Be careful with:**
- Private repositories (if sensitive)
- GitHub token access (not needed for basic workflow)
- Publishing releases (once public, hard to delete)

---

## 💰 Free GitHub Actions

**GitHub provides FREE:**
- 2,000 minutes/month actions runtime
- Enough for ~100 builds
- Public repositories unlimited
- Private repositories limited

**CoreFlow builds:**
- ~5-10 minutes per build
- ~100 minutes/month with weekly builds
- **Plenty within free limits!**

---

## 🎉 You're Done!

Now you have:

✅ Fully automated build system  
✅ Cloud-based compilation  
✅ One-click releases  
✅ No local terminal needed  
✅ GitHub-hosted artifacts  
✅ Public download links  

---

## 📞 Next Steps

1. **Setup repository** (follow Step 1-5 above)
2. **Test workflow**
   - Push a small change
   - Go to Actions
   - Watch build run
3. **Download artifact**
   - After build completes
   - Test on device
4. **Create release**
   - When ready to publish
   - Use proper version tag
   - Release automatically created!

---

## 🚀 Start Building on GitHub!

1. Create repository on github.com
2. Upload all CoreFlow files
3. Add workflows (.github/workflows/*.yml)
4. Push or trigger build
5. Download artifact
6. Flash to device
7. **Done! Completely automated!**

---

**No PC needed. No terminal required. Just GitHub + Browser.**

🎊 **Happy Building!** 🎊

---

## Useful Links

- **GitHub**: https://github.com
- **Actions Docs**: https://docs.github.com/en/actions
- **Releases**: https://docs.github.com/en/repositories/releasing-projects-on-github
- **Artifacts**: https://docs.github.com/en/actions/managing-workflow-runs/downloading-workflow-artifacts

---

*Last Updated: 2024*  
*CoreFlow v3.0 - Fully Automated*
