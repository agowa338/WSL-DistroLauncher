# WSL Alpine DistroLauncher
## Introduction 
  This is the Alpine Launcher implementation for a Windows Subsystem for Linux (WSL) distribution.
  
  Note: This project is written in C++.

  ### Goals
  The goal of this project is to enable:
  * Allow devolopers to use Alpine on their Windows machines for simple tasks (basically everything except kernel modules)
  * Allow developers to use the WSL with a very low footprint (compared to debian/ubuntu)
  * Use the official tar.gz userland release without building a custom userland tar.gz
  * Make the project "self maintained", i.e. don't require to ship any dependencies that require being updated and automatically pull the latest stable userland from the official source (WIP)

### Project Status
This project is an active repo maintained by agowa338.

### Contents
  The sample provides the following functionality:

  * `Alpine.exe`
    - Launches the user's default shell in the user's home directory.

  * `Alpine.exe install [--root]`
    - Install the distribution and do not launch the shell when complete.
      - `--root`: Do not create a user account and leave the default user set to root.

  * `Alpine.exe run <command line>`
    - Run the provided command line in the current working directory. If no command line is provided, the default shell is launched.
    - Everything after `run` is passed to WslLaunchInteractive.

  * `Alpine.exe config [setting [value]]`
    - Configure settings for this distribution.
    - Settings:
      - `--default-user <username>`: Sets the default user to <username>. This must be an existing user.

  * `Alpine.exe help`
    - Print usage information.

## Getting Started (Project Participants)
  0. Clone this repository and open it with Visual Studio ( >= 2017 )
  1. Generate a test certificate.
  2. Import it to your Cert:\CurrentUser\My and Cert:\LocalMachine\TrustedPublisher as well as Cert:\LocalMachine\Root. If you have a dedicated testing VM, import the certificate only to Cert:\CurrentUser\My on your production system, or you may make yourself vulnerable to all kinds of attacks. That is the reason why you would want to protect this certificate as nearly well as a bought code signing certificate.
  3. Open `DistroLauncher-Appx/MyDistro.appxmanifest`, select the Packaging tab, select Choose Certificate, click the Configure Certificate drop down and select Create test certificate.
  4. Edit your distribution-specific information in `DistroSpecial.h`. **NOTE: The `DistroSpecial::Name` variable must uniquely identify your distribution and cannot change from one version of your app to the next. Otherwise it will register as another distribution within WSL. If you want to have one distro twice, you would change this name.**
  5.  Update `Alpine.appxmanifest`. There are several properties that are in the manifest that will need to be updated with your specific values.
      - Make sure to note the `Identity Publisher` value (by default, `"CN=DistroOwner"`). We'll need that for testing the application.
      - Make sure that `<desktop:ExecutionAlias Alias="mydistro.exe" />` is set to something that ends in ".exe". This is the command that will be used to launch your distro from the command line.

## Build and Test
  To help building and testing the Alpine Launcher project, there are the following scripts to automate some tasks. You can either choose to use these scripts from the command line, or work directly in Visual Studio, whatever your preference is. 

  **Please Note** some sideloading/deployment steps don't work if you mix and match Visual Studio and the command line for development. If you run into errors while trying to deploy your app after already deploying it once, the easiest step is usually just to uninstall the previously sideloaded version and try again. By right-clicking the App and selecting "Uninstall", if WSL is still installed, but the app is not, just run `wslconfig.exe /u Alpine`.

### Setting up your Windows Environment
You will need a Windows environment to test that your app installs and works as expected. To set up a Windows environment for testing you can follow the steps from the [Windows Dev Center](https://developer.microsoft.com/en-us/windows/downloads/virtual-machines).

Note: If you are using Hyper-V you can use the new VM gallery to easily spin up a Windows instance.

### Building Project (Command line):
  To compile the project, you can simply type `.\build.bat` in the root of the project to use MSBuild to build the solution. This is useful for verifying that your application compiles. It will also build an appx for you to sideload on your dev machine for testing.
  
  `build.bat` assumes that MSBuild is installed at one of the following paths:
  `%ProgramFiles*%\MSBuild\14.0\bin\msbuild.exe` or
  `%ProgramFiles*%\Microsoft Visual Studio\2017\Enterprise\MSBuild\15.0\Bin\msbuild.exe` or
  `%ProgramFiles*%\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\msbuild.exe`.

  If that's not the case, then you will need to modify that script. 

  Once you've completed the build, the packaged appx should be placed in the directory `Alpine-DistroLauncher\x64\Debug\DistroLauncher-Appx` and should be named something like `DistroLauncher-Appx_1.0.0.0_x64_Debug.appx`. Simply double click that appx file to open the sideloading dialog. 

  You can also use the PowerShell cmdlet `Add-AppxPackage` to register your appx:
  ``` powershell
  powershell Add-AppxPackage x64\Debug\DistroLauncher-Appx\DistroLauncher-Appx_1.0.0.0_x64_Debug.appx
  ```

### Building Project (Visual Studio):

  You can also easily build and deploy the distro launcher from Visual Studio. To sideload your appx on your machine for testing, all you need to do is right-click on the "Solution (DistroLauncher)" in the Solution Explorer and click "Deploy Solution". This should build the project and sideload it automatically for testing.

  **NOTE: In order run your solution under the Visual Studio debugger, you will need to manually unregister it via `wslconfig.exe /u Alpine`.**

### Testing
  You should now have a finished appx sideloaded on your machine for testing. 

  But before you do that, make sure you've enabled Developer Mode in the Settings app (sideloading won't work without it). Then double click on the signed appx and click "Install" to install it. Note that this only installed the appx on your system, but it doesn't unzip the tar.gz or register the distro yet. 

  You can then begin the distro registration by launching the app from the Start Menu or executing `Alpine` from the command line. Or just select "Start after installation" while installing. 

### Publishing
  Once you are ready to upload your appx to the store, you will need to change a few small things to prepare the appx for the store.  

  1. In the appxmanifest, you will need to change the values of the Identity to match the value given to you by the store. This should look like the following:

  ``` xml
  <Identity Name="1234YourCompanyName.YourAppName"
            Version="1.0.1.0"
            Publisher="CN=12345678-045C-ABCD-1234-ABCDEF987654"
            ProcessorArchitecture="x64" />
  ```

  **NOTE**: Visual Studio can update this for you. You can do that by right-clicking on "DistroLauncher-Appx (Universal Windows)" in the solution explorer and clicking on "Store... Associate App with the Store..." and following the wizard. 

  2. You will either need to run `.\build.bat rel` from the command line to generate the Release version of your appx or use Visual Studio directly to upload your package to the store. You can do this by right-clicking on "DistroLauncher-Appx (Universal Windows)" in the solution explorer and clicking on "Store... Create App Packages..." and following the wizard. 

  Also, make sure to check out the [Notes for uploading to the Store](https://github.com/Microsoft/WSL-DistroLauncher/wiki/Notes-for-uploading-to-the-Store) page on our wiki for more information.

# Issues & Contact
Any bugs or problems discovered with the Launcher should be filed in this project's Issues list. 

# Contributing
## Base Project
This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.
