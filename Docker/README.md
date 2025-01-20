# Conceal Desktop Docker Setup for Windows

## Prerequisites

1. Install Docker Desktop:
   - Download from: https://www.docker.com/products/docker-desktop
   - Run the installer (Docker Desktop Installer.exe)
   - During installation:
     - Enable WSL 2 if prompted
     - Keep "Use WSL 2 instead of Hyper-V" checked
   - Start Docker Desktop from the Start menu
   - Wait for Docker Desktop to start (whale icon in taskbar turns solid)

2. Install VcXsrv Windows X Server:
   - Download from: https://sourceforge.net/projects/vcxsrv/
   - Run XLaunch
   - Select "Multiple windows"
   - Display number: -1
   - Select "Start no client"
   - In Extra settings:
     - Check "Disable access control"
     - Check "Native opengl"
   - Click Finish
   - Note: VcXsrv must be running before starting the container

## Building and Running

1. Make sure VcXsrv is running (check system tray for X icon)

2. Open PowerShell and set the display variable:
```powershell
$env:DISPLAY = "host.docker.internal:0"
```

3. Navigate to the Docker/Docker-Ubuntu directory in the repository and build:  
*It's named -Ubuntu because it emulates ubuntu in your windows*   
```powershell
cd Docker/Docker-Ubuntu
docker-compose build
```

4. Start the container:
```powershell
docker-compose up
```

The Conceal Desktop GUI should appear in a window. The blockchain data will be stored in a Docker volume that automatically expands as needed.

:Warning: When importing and saving your wallet, use the volume created in docker for this purpose: `/home/conceal/wallets`. It's highly recommended to encrypt the wallet file.

To check storage usage:
```powershell
docker system df -v
```

## Troubleshooting

If you encounter display issues:
1. Verify VcXsrv is running
2. Make sure the DISPLAY environment variable is set correctly
3. Try restarting VcXsrv and the container

If the container fails to start:
1. Ensure Docker Desktop is running
2. Check Docker Desktop logs for any errors
3. Try rebuilding the container with:
```powershell
docker-compose build --no-cache
```
