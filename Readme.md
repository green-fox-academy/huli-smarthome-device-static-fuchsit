Hello Smart Home!

# Smarthome devices

## Description

This project is about Smart Home, which runs together with the **[Smarthome Server](smarhome-server.md)** project and will be implemented on B-L475E-IOT01x boards.

 - Smart light
    - turn on / turn off / DIM / RGB support (first on one LED, later we connect this to an RGB LED stripe)
 - Indoor weather station
    - measures and pushes the data to Cloud
 - Coffee maker control
    - heat up / make a coffee / schedule a coffee (the mug, water and coffee beans should be prepared manually)
 - Smart plug
    - turn on / turn off / schedule

The devices will communicate through the Google IoT Core infrastructure, which unlocks a significant future potential in this project:

 - The solution can be securely integrated to any online app
 - Cloud database, big data
 - Data analysis
 - Machine learning/AI
 - Google assistant integration (control with speech through dialogues)

Depending on the progress, we may start dig inside these areas as well.

## Technologies

 - C
 - UPnP/SSDP
 - MQTT
 - Google IoT Core
 - Secure communication
 - Board sensors

## Main features

 - Discoverability on WLAN
 - Light control (on/off/dim/color)
 - Warmup and start a coffee maker
 - Reporting indoor weather data
 - Wall plug control

## Components

 - B-L475E-IOT01x board
 - [Goolge IoT Core](https://goo.gl/khLTXc)
 - [UPnP / SSDP](https://goo.gl/PpJPNh)
 - [REST](https://goo.gl/ZyaDz)
 - [MQTT](https://goo.gl/OqNDSO)
 - [SSL security](https://goo.gl/ybM9n)

## Repositories

 - [smarthome devices](#)

## Milestones

Week #  | Milestone
-------:|:-----------------------
Week 1  | Environment setup, tech. assessment
Week 2  | Try all necessary APIs of Google IoT Core
Week 3  | SSDP discovery
Week 4  | Device properly cloud-connected
Week 5  | Implement the device-specific features
Week 6  | Integration
