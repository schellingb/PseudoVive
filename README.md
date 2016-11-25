PseudoVive
==========

This driver for SteamVR/OpenVR forces any loaded device driver to return Vive model names for HMD and controllers so that any application with an explicit check for it runs regardless of the used VR headset or motion controller.

## Install

1. Download the [archive from here](https://github.com/schellingb/PseudoVive/archive/master.zip).

2. Right click SteamVR in the Tools or VR section in the Steam library. Click on 'Local Files' and then 'Browse Local Files...'.

3. Extract the '2vive' folder from the PseudoVive master ZIP into the 'drivers' folder of SteamVR.

4. Restart SteamVR

You can check if the installation was successful by opening the 'Create System Report' window with the SteamVR menu or by right-clicking the SteamVR systray icon. Then under the 'Devices' tab it should list your devices with a different model name.

## Notes

SteamVR loads its drivers in alphabetical order so our driver is named in a way that makes sure it is loaded first (with a leading number).

Even though the PseudoVive driver itself does not offer any devices, it makes sure that all drivers loaded after it will return the forced device model names.

So far it only changes the values for 'ManufacturerName' and 'ModelNumber' which seems enough for the applications that are out there at the moment. In theory a more elaborate check could be made that checks the format of the serial number or other fields.

## Dependencies

- [MinHook by Tsuda Kageyu](https://github.com/TsudaKageyu/minhook) (included in this repository)
- [OpenVR SDK by Valve](https://github.com/ValveSoftware/openvr) (included in this repository)

## License

PseudoVive is available under the [zlib license](http://www.gzip.org/zlib/zlib_license.html).