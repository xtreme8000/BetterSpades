# BetterSpades Flatpak manifest
For building the universal flatpak application you need to install `flatpak-builder` and Freedesktop SDK 20.08:
`flatpak install flathub org.freedesktop.Sdk/x86_64/20.08`
### Build
In the repo root dir:
```
git submodule update --init --recursive
flatpak-builder flatpak/build flatpak/party.aos.betterspades.yml --install --user
```
### Run
`flatpak run party.aos.betterspades`
