# Audio Sensor Motor Control (EECE 5520 Project)

Arduino-based motor control system using audio sensors and FFT analysis.  
Motor speed and direction adjust dynamically based on detected sound frequencies (C4 ~262 Hz, A4 ~440 Hz).  

## Features
- Implemented FFT algorithm in C++ for frequency detection.  
- Used H-bridge (L293D) for bi-directional motor control.  
- Applied Object-Oriented Design (OOD) to modularize motor, LCD, RTC, and IR modules.  
- Displayed real-time fan parameters (speed, direction, clock) on LCD.  

## My Contributions
- Developed LCD integration and RTC display code.  
- Applied OOD structure for modular design.  
- Debugged IR remote integration.  

## Hardware
- Arduino Mega 2560  
- Audio Sensor Module  
- L293D H-bridge  
- 16x2 LCD Display  
- DS1307 RTC Module  
- IR Receiver  

## Repository
Forked from [slouisy/Lab3](https://github.com/slouisy/Lab3).
