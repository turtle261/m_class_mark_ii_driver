# M-Class Mark II CUPS Driver for macOS - Compilation Instructions

Instructions for **compiling** the Datamax-O'Neil M-Class Mark II CUPS driver from source on macOS.

## Prerequisites

Ensure you have:
- Xcode Command Line Tools installed
- Homebrew installed
- CUPS development headers available

## 1. Get the source code

```bash
cd ~/Downloads   # or wherever you like
git clone https://github.com/your-org/m_class_mark_ii_cups_driver.git
cd m_class_mark_ii_cups_driver
```
(If you downloaded a ZIP file instead, simply unzip it and `cd` into the extracted folder.)

---

## 2. Build & install

Run the following two commands. The first builds the filter binary and PPD file; the second copies them into the correct CUPS folders (you will be prompted for your macOS password):
```bash
make            # compile
sudo make install  # copy driver files into CUPS
```
That is all – the driver is now available to macOS.

---

## 3. Add your printer

1. Open **System Settings ▸ Printers & Scanners** (or **System Preferences** on older macOS).
2. Click **"+"** to add a printer.
3. Select your **Datamax M-Class Mark II** (or choose *IP* and enter its address).
4. When asked for a driver, pick **"Select Software…"**, then choose **`Datamax › M-Class Mark II (DPL)`** (it appears under *Datamax* after installation).
5. Click **Add**.

Your printer should now work with macOS applications.

---

## 4. Uninstall

If you ever need to remove the driver:
```bash
cd path/to/m_class_mark_ii_cups_driver
sudo make uninstall
```

---

## 5. Troubleshooting

• After installation, you may need to **restart** CUPS for macOS to see the new driver:
```bash
sudo launchctl stop org.cups.cupsd
sudo launchctl start org.cups.cupsd
```
• Ensure the Homebrew `sbin` directory is in your `PATH` so that `cups-config` is found during `make`.

---

*Enjoy printing!*
