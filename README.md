![image](https://github.com/ConcealNetwork/conceal-imagery/blob/master/tqRGU34m_400x400.jpg)

# Conceal Desktop (GUI Wallet)

Latest Release: v6.4.8

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
-   GitHub: <https://github.com/ConcealNetwork/conceal-core>
-   Discord: <https://discord.gg/YbpHVSd>
-   Twitter: <https://twitter.com/ConcealNetwork>
-   Telegram Official (News Feed): <https://t.me/concealnetwork>
-   Telegram User Group (Chat Group): <https://t.me/concealnetworkusers>
-   Medium: <https://medium.com/@ConcealNetwork>
-   Reddit: <https://www.reddit.com/r/ConcealNetwork>
-   Bitcoin Talk: <https://bitcointalk.org/index.php?topic=4515873>
-   Paperwallet: <https://conceal.network/paperwallet>

## Compiling Conceal from source

### Linux / Ubuntu

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
mkdir bin && mv build/release/CONCEAL-GUI bin/
make clean
```

If the build is successful the binary will be in the `bin` folder.

### Windows 10

#### Prerequisites

-   Install [Visual Studio 2017 Community Edition](https://www.visualstudio.com/thank-you-downloading-visual-studio/?sku=Community&rel=15&page=inlineinstall)
-   When installing Visual Studio, you need to install **Desktop development with C++** and the **VC++ v140 toolchain** components. The option to install the v140 toolchain can be found by expanding the "Desktop development with C++" node on the right. You will need this for the project to build correctly.
-   Install [CMake](https://cmake.org/download/)
-   Install [Boost 1.67.0](https://boost.teeks99.com/bin/1.67.0/), ensuring you download the installer for MSVC 14.1.
-   Install [Qt 5.11.0](https://www.qt.io/download)

#### Building

-   From the start menu, open 'x64 Native Tools Command Prompt for vs2017' or run "C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Community\\Common7\\Tools\\VsMSBuildCmd.bat" from any command prompt.
-   Edit the CMakeLists.txt file and set the path to QT cmake folder. For example: set(CMAKE_PREFIX_PATH "C:\\Qt\\5.11.0\\msvc2017_64\\lib\\cmake\\").
-   `git clone https://github.com/ConcealNetwork/conceal-core`
-   `git clone https://github.com/ConcealNetwork/conceal-desktop`
-   Copy the contents of the conceal-core folder into conceal-wallet\\cryptonote
-   `cd conceal-desktop`
-   `mkdir build`
-   `cd build`
-   `cmake -G "Visual Studio 15 2017 Win64" -DBOOST_LIBRARYDIR:PATH=c:/local/boost_1_67_0 ..` (Or your boost installed dir.)
-   `msbuild CONCEAL-GUI.sln /p:Configuration=Release`

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

If the build is successful the binary will be `build/release/CONCEAL-GUI.app`

It is also possible to deploy the application as a `.dmg` by using these commands after the build:

```bash
cd build/release
macdeployqt CONCEAL-GUI.app
cpack
```

## Special Thanks

Special thanks goes out to the developers from Cryptonote, Bytecoin, Monero, Forknote, TurtleCoin, and Masari.
