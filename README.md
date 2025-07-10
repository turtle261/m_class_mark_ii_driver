# M-Class Mark II CUPS Driver for macOS

Pre-built driver for **Datamax-O'Neil M-Class Mark II** label printers on macOS.

## Installation

1. **Download the driver**  
   Go to [Releases](../../releases) and download `m_class_mark_ii_driver_macos.tar.gz`

2. **Extract and install**  
   Open Terminal and run:
   ```bash
   cd ~/Downloads
   tar -xzf m_class_mark_ii_driver_macos.tar.gz
   sudo cp filter/rastertodpl /usr/libexec/cups/filter/ # Install CUPS Driver
   sudo cp dmx_mii.ppd /Library/Printers/PPDs/Contents/Resources/ # Install CUPS Printer Config
   sudo launchctl kickstart -kp system/org.cups.cupsd # Restart CUPS Printing Service
   ```

3. **Add your printer**  
   • Open **System Settings** → **Printers & Scanners**  
   • Click **"+"** to add a printer  
   • Select your printer or enter its IP address  
   • Choose **"Datamax-Oneil M-Class Mark II"** from the driver list  
   • Click **Add**

## Uninstall

```bash
sudo rm /usr/libexec/cups/filter/rastertodpl 
sudo rm /Library/Printers/PPDs/Contents/Resources/dmx_mii.ppd
sudo launchctl kickstart -kp system/org.cups.cupsd # Restart CUPS Printing Service
```
