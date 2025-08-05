
# Milestone

- [x] Entity-Component System
- [x] Textures
    - [x] Render Ordering
- [ ] Animations
- [ ] Collisions
- [ ] Interactions
- [ ] Maps

# Build

For Windows builds, the Makefile will target `mingw64` and use the mingw64 toolchain.
Linux builds will use the native gcc compiler available.

```sh
./setup.sh && make
```

OS is set to `windows` by default within the Makefile.
The `PLATFORM` variable can be set explicitly as shown.
This feature is primarily for cross-compilation from Linux to Windows (and not necessarily
the other way around)

Valid values: `linux`, `windows`
```sh
make PLATFORM=linux
```

To specify the platform for the setup script:

Valid values: `linux`, `windows`
```sh
./setup.sh linux
```

Additionally, the setup script will ask for input on certain behavior(s).
Simply answer as necessary.


