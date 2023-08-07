# Ukaton Mission Firmware

How to load to your Ukaton Mission motion module:
0. Remove the battery from the motion module and plug in via USB
1. Open VSCode
2. Install PlatformIO on VSCode
3. Open this workspace
5. [Change the name of your device in definitions.h](https://github.com/zakaton/ukaton-mission-firmware/blob/main/src/definitions.h#L15)
6. Click the upload button on the bottom (looks like "->")
7. After uploading, unplug the USB and plug it back in
8. Test [here](https://ukaton-side-mission.glitch.me/visualization) to see if it works
9. If it works, add the battery
