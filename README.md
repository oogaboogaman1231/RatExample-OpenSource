# Remote Access Tool V1.2.0A 🚨SECURE🚨

## Overview
This project is a remote access tool that allows you to control a remote computer. It supports various features such as remote desktop viewing, password recovery, and supports dynamic DNS services like ngrok and No-IP.

## Features
- Remote Desktop (Live screen footage)
- Password Recovery (Retrieve stored passwords)
- TCP and UDP server support
- Dynamic DNS support with ngrok and No-IP
- Customizable build options for the agent
- Options to start on startup, hide in specific directories, and use mutexes
- Secure communication using OpenSSL for encryption

## Requirements
- Windows operating system
- Visual Studio or any compatible C++ development environment
- [Msftedit.dll](https://docs.microsoft.com/en-us/windows/win32/api/richedit/) for RichEdit control (part of Windows OS)
- [OpenSSL](https://www.openssl.org/) for secure communication

## Installation
- First [download the installer](installer.bat), then run it
- Wait for it to do its thing

## Updating
- We recommend for the user to always keep the installer file since it will download and install the most recent version of the RAT when executed.

## Running the Application
- After running the install.bat script, a shortcut named "RAT Server" will be created on your desktop. You can use this shortcut to run the server.

## Ngrok Setup
- Sign up for an ngrok account and get your auth token.
- Enter your ngrok auth token in the respective field in the GUI.

## No-IP Setup
- Sign up for a No-IP account.
- Create a hostname and get your No-IP credentials.
- Enter your No-IP username, password, and hostname in the respective fields in the GUI.

## Usage

### Configure the Server
1. **Enter Server IP and Port:**
   - In the GUI, enter the IP address and port number for the server.
2. **Start the Server:**
   - Click the button to start the TCP server.

### Build and Deploy the Agent
1. **Enter Agent IP, Port, and Password:**
   - In the GUI, enter the IP address, port number, and password for the agent.
2. **Generate RSA Keys for Secure Communication:**
   - Use OpenSSL to generate RSA keys:
     ```sh
     openssl genrsa -out private.pem 2048
     openssl rsa -in private.pem -outform PEM -pubout -out public.pem
     ```
   - Ensure `private.pem` is kept secure and `public.pem` is distributed with the agent.
3. **Build the Agent:**
   - Click the button to build the agent. The build process will include the public key for encryption.

### Deploying the Agent
- Deploy the generated agent executable to the target machine.
- Ensure the agent has access to `public.pem` for encryption.

### Controlling the Agent
- Use the server GUI to control the agent, view the remote desktop, and recover passwords.
- All communication between the server and agent will be encrypted using the provided keys.

## Changelog
- **V1.2.0A:** Added support for secure communication using OpenSSL encryption.
- **V1.1.2A:** Fixed various bugs and improved stability.
- **V1.1.0A:** Added dynamic DNS support with ngrok and No-IP.

## Troubleshooting

If you encounter any issues while using the Remote Access Tool, you can refer to the following troubleshooting steps:

### 1. Compilation Errors
- **Issue:** If you encounter compilation errors when running `install.bat`, ensure that you have the necessary dependencies installed and that your compiler path is correctly set.
- **Solution:** Double-check your compiler setup and ensure that all required libraries are properly linked.

### 2. Missing Dependencies
- **Issue:** If the application fails to run due to missing dependencies, such as `ws2_32.lib` or `Comctl32.lib`, make sure these libraries are included in your project and properly linked during compilation.
- **Solution:** Verify that the necessary libraries are present in your project and correctly configured in your compiler settings.

### 3. Desktop Shortcut Not Created
- **Issue:** If the desktop shortcut is not created after running `install.bat`, check if the script encountered any errors during execution.
- **Solution:** Review the output of the `install.bat` script for any error messages. Ensure that the script has sufficient permissions to create shortcuts on the desktop.

### 4. GUI Not Displayed
- **Issue:** If the GUI fails to display after running the server, check if there are any issues with the graphical interface libraries or system configurations.
- **Solution:** Verify that your system meets the requirements for running the Remote Access Tool and ensure that all necessary dependencies are properly installed.

If you are unable to resolve your issue using the troubleshooting steps provided above, please feel free to open an [issue](https://github.com/oogaboogaman1231/RatExample-OpenSource/issues) on the GitHub repository, providing detailed information about the problem you encountered. Our team will assist you in resolving the issue as soon as possible.

## Legal Disclaimer
This tool is intended for legal use only. Unauthorized access to computer systems is illegal and punishable by law. Always obtain explicit permission before accessing or controlling a computer that you do not own. The author is not responsible for any misuse of this tool.

## License
This project is licensed under the MIT License.
