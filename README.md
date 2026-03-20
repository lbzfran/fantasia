
# Build

For development on Windows, a posix shell is required. `w64devkit` is recommended.

The Makefile will target `mingw64` and use the mingw64 toolchain.
Linux builds will use the native gcc compiler available.

```sh
./setup.sh && make
```

OS is set to `windows` by default.
The `PLATFORM` variable can be set explicitly as shown.

This feature is exposed primarily for cross-compilation.

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

# References

Occassionally, I will find brilliant solutions to specific problems that
I simply have to credit the original from. Below is that list.

Helped with separation of physics timestep to the rendering timestep (framestep).
- https://gafferongames.com/post/fix_your_timestep/

