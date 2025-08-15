
# Milestone

- [x] Entity-Component System
- [x] Textures
    - [x] Render Ordering
- [x] Animations
    - [x] animation trigger
    - [-] refine animation control (moving forward/backward) [scrapped idea]
    - [x] chaining animations
- [x] Collisions (AABB)
- [x] Interactions
- [ ] Maps
    - [ ] Implement tilemapping: non-entity data that informs how part of the
                                 world works.
                                 Dynamic Entities have soft collision with
                                 tilemaps, while static entities have a hard
                                 collision, and live within the map.
                                 Tiles within tilemaps are non-entity and
                                 provide proper collision to all entities.
- [ ] Loading/Saving Game Data (with versioning)
- [ ] Behaviors
    - [ ] dynamic behaviors (based on conditions, decision tree maybe..?)

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
