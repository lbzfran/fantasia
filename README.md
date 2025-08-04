
# Milestone

[x] Entity-Component System
[ ] Textures
[ ] Animations
[ ] Collisions
[ ] Interactions
[ ] Maps

# Build

```sh
./setup.sh && make
```

OS is inferred automatically, but can also be specified explicitly.
This feature is primarily for cross-compilation.
(Valid values: `linux`, `windows`)
```sh
make PLATFORM=linux
```

To specify the platform for the setup script:
(Valid values: `linux`, `windows`)
```sh
./setup.sh linux
```

Additionally, the setup script will ask for input on certain behavior(s).
Simply answer as necessary.
