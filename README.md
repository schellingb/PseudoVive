# PseudoVive

This driver for SteamVR/OpenVR forces any loaded device driver to return Vive model names for HMD and controllers so that any application with an explicit check for it runs regardless of the used VR headset or motion controller.

## Install

1. Download the latest version [here](https://github.com/Bluscream/PseudoVive/releases).

   - If you download the '2vive_toggle' instead of '2vive' you will have a 'PV' icon next to the SteamVR icon in the windows system icon tray. This 'PV' icon allows you to activate and deactivate PseudoVive at runtime.

2. Right click SteamVR in the Tools or VR section in the Steam library. Click on 'Local Files' and then 'Browse Local Files...'.

3. Extract the ZIP you downloaded into the 'drivers' folder of SteamVR.

4. Restart SteamVR

You can check if the installation was successful by opening the 'Create System Report' window with the SteamVR menu or by right-clicking the SteamVR systray icon. Then under the 'Devices' tab it should list your devices with a different model name.

## Toggle Variation

Make sure not to extract both '2vive' and '2vive_toggle' into the 'drivers' folder of SteamVR.

## Notes

SteamVR loads its drivers in alphabetical order so our driver is named in a way that makes sure it is loaded first (with a leading number).

Even though the PseudoVive driver itself does not offer any devices, it makes sure that all drivers loaded after it will return the forced device model names.

So far it only changes the values for 'ManufacturerName' and 'ModelNumber' which seems enough for the applications that are out there at the moment. In theory a more elaborate check could be made that checks the format of the serial number or other fields.

## Dependencies

- [MinHook by Tsuda Kageyu](https://github.com/TsudaKageyu/minhook) (included in this repository)
- [OpenVR SDK by Valve](https://github.com/ValveSoftware/openvr) (included in this repository)

## License

PseudoVive is available under the [zlib license](http://www.gzip.org/zlib/zlib_license.html).
