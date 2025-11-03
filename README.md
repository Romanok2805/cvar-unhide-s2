# cvar-unhide-s2

![CS2 Console](./assets/console.png)

A Source 2 plugin to reveal all console variables and commands that are marked as hidden or development-only in Counter-Strike 2.

> [!IMPORTANT]
> You must add `-insecure` to Counter-Strike's launch options for this plugin to load.

## Supported games

- Counter-Strike 2

## Installation

1. **Download the latest release of the plugin**: \
   https://github.com/saul/cvar-unhide-s2/releases/latest
1. **Extract the contents of the ZIP to the game's mod folder.**

   - 📂 `$STEAM\steamapps\common\Counter-Strike Global Offensive\game\csgo`

   After extraction there should be an `addons` folder in the game folder, e.g. `Counter-Strike Global Offensive\game\csgo\addons\...`

1. **Update the `game\csgo\gameinfo.gi` file**: \
   Around line 22, add the `Game csgo/addons` search path. This tells the engine to load the plugin before loading Counter-Strike.

   ```diff
   FileSystem
   {
   	SearchPaths
   	{
   		Game_LowViolence	csgo_lv // Perfect World content override

   +		Game	csgo/addons
   		Game	csgo
   		Game	csgo_imported
   		Game	csgo_core
   		Game	core
   ```

1. **Start the game from Steam.**
   > [!WARNING]
   > Counter-Strike must be launched with `-insecure` in the launch options. If you don't know how to do this, take a look this [Steam Community guide](https://steamcommunity.com/sharedfiles/filedetails/?id=379782151).

If you want to disable cvar-unhide-s2:

- Remove the `Game	csgo/addons` line from the gameinfo.gi file.
- Remove `-insecure` from the game's launch options.

## Building

The project is automatically built using GitHub Actions on every push and pull request.

### Automated Builds

- **Build workflow**: Automatically builds Debug and Release configurations on push/PR to main branch
- **Release workflow**: Automatically creates releases when version tags (e.g., `v1.0.0`) are pushed

Download pre-built binaries from the [Releases](../../releases) page.

### Manual Build

Requirements:
- Visual Studio 2022 with C++ development tools
- Git (for submodules)

Steps:
1. Clone the repository with submodules:
   ```bash
   git clone --recurse-submodules https://github.com/Romanok2805/cvar-unhide-s2.git
   ```
2. Open `cvar-unhide-s2.sln` in Visual Studio
3. Build the solution (Configuration: Release, Platform: x64)
4. The output will be in `addons/bin/win64/server.dll`

## Available commands

If you installed the plugin correctly, you should now be able to use the following commands in the console:

- **cvar_unhide**: Reveal all hidden/development-only convars/concommands.
- **cvarlist_md**: Write all concmds/cvars to a `cvarlist.md` file in the `csgo` game directory. See [cvarlist.md](./cvarlist.md) for example output.
