# BetterSpades Flatpak manifest
For building universal flatpak image you need to install `flatpak-builder` and Freedesktop SDK 20.08:
`flatpak install flathub org.freedesktop.Sdk/x86_64/20.08`
### Build
In the repo root dir:
```
git submodule update --init --recursive
flatpak-builder flatpak/build flatpak/org.xtreme8000.BetterSpades.yml --install --user
```
### Run
`flatpak run org.xtreme8000.BetterSpades`
