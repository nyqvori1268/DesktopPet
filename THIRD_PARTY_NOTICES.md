# Components and artwork

DesktopPet uses native Windows Win32, GDI, GDI+, WinHTTP, CNG, DPAPI and the
Microsoft C++ Runtime. These Microsoft components remain governed by their
respective Microsoft/Windows/Visual Studio terms; the project's MIT license
relicenses none of them. No Electron, WebView, Unity or third-party runtime
framework is bundled.

The built-in procedural pixel pets, effects, original geometric test fixtures
and bundled dialogue are original project assets covered by this project's
MIT license. No third-party character artwork is included in the public
release. Imported custom artwork belongs to its respective rights holders;
users must have permission for their intended use and redistribution. The
project's MIT license grants no rights to those imports.

CMake, GitHub Actions and optional development/test tools are not runtime
components and are not bundled in the executable distribution.

The six ocean pets in v1.3.0 use the original GIF artwork explicitly supplied
for this project by the user (DesktopPet-v1.3.0-OceanPets-GIF). No web artwork
was downloaded. Cropped, losslessly indexed frames are embedded in the main
executable; source hashes and crop coordinates are recorded in
assets/ocean/provenance.json. Existing procedural pet artwork is unchanged.
Pillow is an optional developer-only conversion tool, not a runtime dependency
and not bundled in either distribution. See docs/OCEAN_PETS.md.

The renewed cat/dog/budgie/cockatiel/panda atlases are original project artwork generated for this task with the built-in image generation tool; no web or licensed character artwork was imported. They remain covered by the project MIT license. Retained source atlases and prompt specifications are in assets/renewed/source; Pillow remains a developer-only conversion dependency.
