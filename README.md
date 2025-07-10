# M-Class Mark II CUPS Driver for macOS

A lightweight CUPS driver that enables Datamax‐O’Neil **M-Class Mark II** label printers to work on macOS.

---

## 1. Prerequisites

1. **Homebrew (recommended)** – If you do not already have it, open Terminal and install Homebrew:
   ```bash
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   ```
2. **CUPS developer tools** – Required for the `cups-config` and `ppdc` utilities that the build uses:
   ```bash
   brew install cups
   ```
3. **Xcode Command-Line Tools** – Provides the compiler (`clang`) and build utilities:
   ```bash
   xcode-select --install
   ```
   If they are already installed, this command will report so and do nothing.

> After installation Homebrew may suggest commands to add CUPS to your `PATH`. Follow those instructions (usually adding `/opt/homebrew/sbin` for Apple Silicon or `/usr/local/sbin` for Intel Macs).

---

## 2. Download the driver

In Terminal, choose a convenient folder and clone this repository:
```bash
cd ~/Downloads   # or wherever you like
git clone https://github.com/your-org/m_class_mark_ii_cups_driver.git
cd m_class_mark_ii_cups_driver
```
(If you downloaded a ZIP file instead, simply unzip it and `cd` into the extracted folder.)

---

## 3. Build & install

Run the following two commands. The first builds the filter binary and PPD file; the second copies them into the correct CUPS folders (you will be prompted for your macOS password):
```bash
make            # compile
sudo make install  # copy driver files into CUPS
```
That is all – the driver is now available to macOS.

---

## 4. Add your printer

1. Open **System Settings ▸ Printers & Scanners** (or **System Preferences** on older macOS).
2. Click **“+”** to add a printer.
3. Select your **Datamax M-Class Mark II** (or choose *IP* and enter its address).
4. When asked for a driver, pick **“Select Software…”**, then choose **`Datamax › M-Class Mark II (DPL)`** (it appears under *Datamax* after installation).
5. Click **Add**.

Your printer should now work with macOS applications.

---

## 5. Uninstall

If you ever need to remove the driver:
```bash
cd path/to/m_class_mark_ii_cups_driver
sudo make uninstall
```

---

## 6. Troubleshooting

• After installation, you may need to **restart** CUPS for macOS to see the new driver:
```bash
sudo launchctl stop org.cups.cupsd
sudo launchctl start org.cups.cupsd
```
• Ensure the Homebrew `sbin` directory is in your `PATH` so that `cups-config` is found during `make`.

---

*Enjoy printing!* 