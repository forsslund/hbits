# Haptic Bits Lab Repository
here you find all the necessary files to be able to eithe build your own or repair a Haptic Bit. If you want to know more what a haptic bit is or the Haptic Bits Lab, have a look here: (https://hbits.forsslundsystems.com)

## Software Recommendation for MIDI Recording
The are myriads of choices of MIDI recording software. Here's a list of some that work great with the bits:
- Chataigne: A multipurpose sequencer that also supports MIDI. You'll find a template in the folder "MIDI". It's configured to record all bits in one kit. https://benjamin.kuperberg.fr/chataigne/en
- Signal: Online MIDI sequencer that runs great on Chrome (note: doesn't work with Safari): https://signalmidi.app/edit
- MIDI Bitslab: Our own attempt of a recording environment that runs great on Chrome (note: doesn't work with Safari). It emphasises sketching and direct recording and has some features to make dynamic patches. : https://github.com/somaBits/Midi_BitLab/


## Firmware
The bits are driven by an ESP32. If you would like to make changes to the firmware you are free to download this repository and open it in VS code with the platform.io plugin.

This can be usefull if for example you wish to customise the haptic patterns for the vibration bits.

## Do it Yourself
The Haptic Bits Lab is designed to be built by anyone that has access to a 3D printer and laser cutter. The bill of materials is in [foldername]. The carrier PCB are produced by PCBway and can be order semi-populated with SMD parts via this link: (https://www.pcbway.com/project/shareproject/Haptic_Bits_Lab_Carrier_PCB_semi_populated_7a38576c.html)

The 3D model of the enclosure are modelled in Fusion 360 and can be downloaded here:


|Name | Link | Image |
| ------------- | ------------- | ------------- |
| AIR enclosure | https://a360.co/3LH06NW or [3MF file for printing ](hardware/3d_models/HBL_AIR.3mf)| ![AIR](https://github.com/user-attachments/assets/d55b6fac-87f3-424a-9efa-90fedc94c571) |
| VIBE / HEAT enclosure | https://a360.co/4b2XGnf or [3MF file for printing ](hardware/3d_models/HBL_VIBEHEAT.3mf) | ![VIBE:HEAT](https://github.com/user-attachments/assets/b21250de-ba8c-46b2-b5a4-6822bc7ba87f) |
| Rotary Knob | https://a360.co/4jEkBaE or [3MF file for printing ](hardware/3d_models/HBL KNOBS_6x.3mf) | ![Knob](https://github.com/user-attachments/assets/d07edb3d-1eb6-47a5-bff6-354c7bf92140) |
| LED Ring Support | https://a360.co/4jEkBaE or [3MF file for printing ](hardware/3d_models/HBL_RING_SUPPORT.3mf) | ![LED support](https://github.com/user-attachments/assets/95b652a9-eae0-4722-9c73-2f53bbcf477d) |
| USB-C Support | https://a360.co/4qu6h79  or [3MF file for printing ](hardware/3d_models/HBL_USB_SUPPORT.3mf) | ![USB-C Support](https://github.com/user-attachments/assets/8fe35e68-db01-4e86-b39b-f433a2df9841) |
| VIBE Puck | https://a360.co/3NGZx7i  or [3MF file for printing ](hardware/3d_models/HBL_PUCK.3mf) | ![VIBE Puck](https://github.com/user-attachments/assets/28ec015c-1ade-4de0-803e-54617480d0c3) |

