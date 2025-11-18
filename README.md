# crux:25
stager - red teaming tool

Work-in-Progress: CRUX:25
So README describes what will be at some point...

Let’s get to the crux of it…


What’s it? Why bother?
educational research project designed to simulate the lifecycle of a multi-stage red teaming exercise. The goal of this project is to study offensive techniques and develop corresponding detection rules for Blue Teams. It serves as a modular platform for understanding malware behavior, (C2) architecture, and post-exploitation activities in a controlled, isolated environment.


Disclaimer: The project in nature is for skill building and educational pupososed. It is intended to be used only in authorized, isolated sandbox environments. Also it is probably flawed so the authors are not responsible for any misuse of this software.


tl;dr:Data Exfiltration & Impact Simulation


Simulates the automated collection of telemetry and clipboard data to demonstrate how stealers operate.
Upon completion of the attack chain, the framework executes a non-destructive payload designed to visualize a successful compromise.
Behavior: The desktop wallpaper is altered to signal the breach, and a dialog box is presented to the user.
File Locking Simulation: Targeted files are archived into a password-protected ZIP container to simulate the availability loss caused by ransomware.
A coffin.txt file is placed on the desktop, containing the decryption key for the archive. It signals success in testing crux and allows defenders to unlock locked files after reading coffin’s message.


Overview
The tool purpose it to simulates an attack chain to test if EDR and network security controls are resilient to TA advances. It’s stages are representing different phases of a potential intrusion.


Key Modules & Capabilities
1. Dropper


Simulates the delivery of payloads via common file formats (e.g., PDF).


Objective: Analyze how initial access vectors are flagged by static analysis tools.


2. footprinting


Performs safe system fingerprinting to determine OS patch levels and environment details.


Technique: Implements environmental awareness logic to execute payloads only in specific, vulnerable conditions (Anti-Sandbox/Anti-Analysis techniques).


2. C2 Comms:


Setups channels using intermediary services (simulating generic public platforms like Pastebin) and proxy chains.


Objective: Study traffic patterns and develop signatures for C2 beaconing detection.


3. Lateral Movement Simulation


Demonstrates propagation techniques across local network shares.


Objective: Understand how threats pivot through SMB shares and how to detect unauthorized file replication logs.


End: Demonstrates "ransomware-like" behavior (e.g., file locking mechanisms and desktop background changes) to visualize the impact of a successful compromise without destructive encryption.


Learning Objectives
This project aims to provide hands-on experience with:


MITRE ATT&CK Framework: Mapping code execution to specific tactics (T1105 Ingress Tool Transfer, T1082 System Information Discovery, T1021 Remote Services).


Network Defense: Analyzing C2 traffic and lateral movement artifacts.


Win32 API Programming: Understanding low-level system interactions used in security tools.
