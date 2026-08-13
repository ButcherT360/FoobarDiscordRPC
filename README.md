# FoobarDiscordRPC
Displays your currently playing song in Discord Rich Presence, including the artist, track progress, and album cover.

# FoobarRPC

Shows your currently playing song in Discord Rich Presence with album artwork and playback progress.

FoobarRPC reads the currently playing track from [foobar2000](https://www.foobar2000.org/) and displays it in Discord using the Discord Social SDK.

## Features

* 🎵 Currently playing artist and song
* 💿 Album information
* 🖼️ Automatic album artwork
* ▶️ Playing / paused status
* ⏱️ Playback progress
* 🔄 Automatically updates when the song changes
* 🎧 Works with foobar2000

## How it works

```text
foobar2000
    ↓
FoobarRPC
    ↓
MusicBrainz
    ↓
Cover Art Archive
    ↓
Discord Rich Presence
```

MusicBrainz is used to identify the release and obtain the release ID. Cover Art Archive is then used to retrieve the album artwork.

## Download

Download the latest version from the [GitHub Releases](../../releases) page.

Extract the downloaded ZIP and run:

```text
StartFoobar.bat
```

This starts both FoobarRPC and foobar2000.

## Requirements

* Windows
* foobar2000
* Discord

## Building

This project is written in C++ and uses Visual Studio.

Dependencies include:

* Discord Social SDK
* nlohmann/json
* Windows WinHTTP

## Credits

Developed by ButcherT & ChatGPT.

This project was developed with assistance from ChatGPT for C++ implementation, API integration and debugging.

## License

This project is licensed under the MIT License.

See [LICENSE](LICENSE) for details.

## Third-party services

This project uses:

* [MusicBrainz](https://musicbrainz.org/) for music release information
* [Cover Art Archive](https://coverartarchive.org/) for album artwork
* [Discord Social SDK](https://discord.com/developers/docs/social-sdk) for Discord Rich Presence
* [foobar2000](https://www.foobar2000.org/) as the music player
