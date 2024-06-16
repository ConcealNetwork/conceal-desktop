![image](https://github.com/ConcealNetwork/conceal-imagery/blob/master/logos/splash.png)

# Conceal Desktop (GUI Wallet)

Latest Release: v6.7.3

Maintained by Conceal Devs.

## Information

Conceal.Network is a decentralized blockchain bank, with deposits and investments paying interest rates, without involvement of financial institutions, powered by 100% open source code.

Conceal.Network enables untraceable and anonymous messaging, and a secure way to transfer funds. Using a distributed public ledger, the sender and receiver are kept anonymous, a key concern in a post Snowden world. Hackers cannot trace money or messages when the messages are sent across public networks.

Conceal Cryptocurrency (₡CCX) is based on the Cryptonote protocol and runs on a secure peer-to-peer network technology to operate with no central authority. You control the private keys to your funds.

Conceal is accessible by anyone in the world regardless of their geographic location or status. Our blockchain is resistant to any kind of analysis. All your CCX transactions and messages are anonymous. Conceal avoids many concerns, e.g. technological, environment impact, reputational and security, of Bitcoin, and provides a glimpse of the future.

Conceal is open-source, community driven and truly decentralized.

No one owns Conceal, everyone can take part.

## Resources

-   Web: <https://conceal.network>
-   GitHub: <https://github.com/ConcealNetwork>
-   Wiki: <https://conceal.network/wiki>
-   Explorer: <https://explorer.conceal.network>
-   Discord: <https://discord.gg/YbpHVSd>
-   Twitter: <https://twitter.com/ConcealNetwork>
-   Telegram Official (News Feed): <https://t.me/concealnetwork>
-   Telegram User Group (Chat Group): <https://t.me/concealnetworkusers>
-   Medium: <https://medium.com/@ConcealNetwork>
-   Reddit: <https://www.reddit.com/r/ConcealNetwork>
-   Bitcoin Talk: <https://bitcointalk.org/index.php?topic=4515873>
-   Paperwallet: <https://conceal.network/paperwallet>

## Compiling Conceal from source

### Linux / Ubuntu / Debian

#### Prerequisites

Dependencies: GCC 4.7.3 or later, CMake 2.8.6 or later, Boost 1.55 or later, and Qt 5.9 or later.
You may download them from:

-   <https://gcc.gnu.org/>
-   <https://www.cmake.org/>
-   <https://www.boost.org/>
-   <https://www.qt.io>

On Ubuntu it is possible to install them using apt:

```bash
sudo apt install git gcc make cmake libboost-all-dev qt5-default
```

#### Building

To acquire the source via git and build the release version, run the following commands:

```bash
git clone https://github.com/ConcealNetwork/conceal-desktop
cd conceal-desktop
rm -rf cryptonote
git clone https://github.com/ConcealNetwork/conceal-core cryptonote
make build-release
mkdir bin && mv build/release/conceal-desktop bin/
make clean
```

If the build is successful the binary will be in the `bin` folder.

### Windows 10

#### Prerequisites

-   Install [Visual Studio 2019 Community Edition](https://visualstudio.microsoft.com/thank-you-downloading-visual-studio/?sku=Community&rel=16)
    > When installing Visual Studio, you need to install **Desktop development with C++** and the **MSVC v142 - VS 2019 C++ x64/x86 build tools** components. The option to install the v142 build tools can be found by expanding the "**Desktop development with C++**" node on the right. You will need this for the project to build correctly.
-   Install [CMake](https://cmake.org/download/)
-   Install [Boost](https://sourceforge.net/projects/boost/files/boost-binaries/1.78.0/boost_1_78_0-msvc-14.2-64.exe/download), **ensuring** you download the installer for **MSVC 14.2**. 
    > The instructions will be given for Boost 1.78.0. Using a different version should be supported but you will have to change the version where required.
-   Install [Qt 5](https://www.qt.io/download)
    > The instructions will be given for Qt 5.15.2. Using a different version should be supported but you will have to change the version where required.

#### Building

-   From the start menu, open 'x64 Native Tools Command Prompt for vs2019' or run "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\Common7\\Tools\\VsMSBuildCmd.bat" from any command prompt.
-   Edit the CMakeLists.txt file and set the path to QT cmake folder.
    Change the line `set(CMAKE_PREFIX_PATH "C:\\Qt\\5.11.0\\msvc2017_64\\lib\\cmake\\")` to `set(CMAKE_PREFIX_PATH "C:\\Qt\\5.15.2\\msvc2019_64\\lib\\cmake\\")`

```ps
git clone https://github.com/ConcealNetwork/conceal-desktop
cd conceal-desktop
rmdir /S /Q cryptonote
git clone https://github.com/ConcealNetwork/conceal-core cryptonote
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A x64 -DBOOST_ROOT="c:\local\boost_1_78_0"
msbuild conceal-desktop.sln /p:Configuration=Release /m
```

If the build is successful the binaries will be in the `Release` folder.

### macOS

#### Prerequisites

First, we need to install the same dependencies as [conceal-core](https://github.com/ConcealNetwork/conceal-core#macos).

Once conceal-core dependencies are installed, we need to install Qt5, open a Terminal and run the following commands:

```bash
brew install qt5
export PATH="/usr/local/opt/qt/bin:$PATH"
```

#### Building

When all dependencies are installed, build Conceal Desktop with the following commands: 

```bash
git clone https://github.com/ConcealNetwork/conceal-desktop
cd conceal-desktop
rm -rf cryptonote
git clone https://github.com/ConcealNetwork/conceal-core cryptonote
make build-release
```

If the build is successful the binary will be `build/release/conceal-desktop.app`

It is also possible to deploy the application as a `.dmg` by using these commands after the build:

```bash
cd build/release
macdeployqt conceal-desktop.app
cpack
```

## Special Thanks

Special thanks goes out to the developers from Cryptonote, Bytecoin, Monero, Forknote, TurtleCoin, and Masari.

## Copyright

Copyright (c) 2017-2024 Conceal Community, Conceal Network & Conceal Devs